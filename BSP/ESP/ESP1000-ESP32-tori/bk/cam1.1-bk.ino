#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ESP32-CAM OV2640 Simple Camera Server with Single Capture Button

// ===================
// Camera Model - AI-Thinker ESP32-CAM with OV2640
// ===================
#define CAMERA_MODEL_AI_THINKER

// Camera pin definitions for AI_THINKER model
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_GPIO_NUM       4  // Flash LED
#define TRIGGER_GPIO_PIN   3  // External trigger input

// ===========================
// WiFi credentials
// ===========================
const char* ssid = "111";
const char* password = "electrodragon";

WebServer server(80);

// GPIO trigger variables
volatile bool triggerPressed = false;
camera_fb_t* lastCapturedImage = nullptr;
bool imageAvailableFromGPIO = false;

// GPIO interrupt handler
void IRAM_ATTR gpio3InterruptHandler() {
  triggerPressed = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-CAM Simple Web Camera Server");
  
  // Camera configuration
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // VGA resolution for OV2640
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  // Check for PSRAM and configure accordingly
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("PSRAM found - using optimized settings");
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_SVGA;
    Serial.println("No PSRAM - using DRAM settings");
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");

  // Configure camera sensor
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA); // Start with lower resolution
  
  // Setup LED flash
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  // Setup GPIO3 as input trigger with pull-down
  pinMode(TRIGGER_GPIO_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
  Serial.println("GPIO3 configured as trigger input with pull-down");

  // Connect to WiFi
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("Camera server IP: http://");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/last", handleLastImage);
  
  server.begin();
  Serial.println("Web server started");
}



void loop() {
  // Check for GPIO3 trigger
  if (triggerPressed) {
    triggerPressed = false; // Reset flag
    Serial.println("GPIO3 trigger detected - capturing image");
    
    // Call the same capture function used by web interface
    captureImageAndRespond();
    
    // Small delay to avoid multiple triggers
    delay(500);
  }
  
  server.handleClient();
  delay(10);
}

// Main page with capture button
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<title>ESP32-CAM Simple Capture</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; margin-bottom: 30px; }";
  html += ".capture-btn { background-color: #007bff; color: white; border: none; padding: 15px 30px; font-size: 18px; border-radius: 5px; cursor: pointer; margin: 20px; }";
  html += ".capture-btn:hover { background-color: #0056b3; }";
  html += ".image-container { margin-top: 30px; border: 2px dashed #ccc; padding: 20px; min-height: 200px; }";
  html += ".status { margin: 15px 0; font-weight: bold; }";
  html += "img { max-width: 100%; height: auto; border: 1px solid #ddd; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>📷 ESP32-CAM Simple Capture</h1>";
  html += "<p>Click the button below to capture an image, or view the last GPIO-triggered image</p>";
  html += "<button class='capture-btn' onclick='captureImage()'>📸 Capture Image</button>";
  html += "<button class='capture-btn' onclick='viewLastGPIO()' style='background-color: #28a745;'>🔄 View Last GPIO Image</button>";
  html += "<div class='status' id='status'>Ready to capture</div>";
  html += "<div class='image-container' id='imageContainer'>";
  html += "<p style='color: #666;'>Captured image will appear here</p>";
  html += "</div>";
  html += "</div>";
  
  html += "<script>";
  html += "function captureImage() {";
  html += "  document.getElementById('status').innerText = 'Capturing image...';";
  html += "  document.getElementById('status').style.color = 'orange';";
  html += "  fetch('/capture')";
  html += "    .then(response => response.blob())";
  html += "    .then(blob => {";
  html += "      const img = document.createElement('img');";
  html += "      img.src = URL.createObjectURL(blob);";
  html += "      document.getElementById('imageContainer').innerHTML = '';";
  html += "      document.getElementById('imageContainer').appendChild(img);";
  html += "      document.getElementById('status').innerText = 'Image captured successfully!';";
  html += "      document.getElementById('status').style.color = 'green';";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerText = 'Error capturing image';";
  html += "      document.getElementById('status').style.color = 'red';";
  html += "      console.error('Error:', error);";
  html += "    });";
  html += "}";
  html += "function viewLastGPIO() {";
  html += "  document.getElementById('status').innerText = 'Loading last GPIO image...';";
  html += "  document.getElementById('status').style.color = 'blue';";
  html += "  fetch('/last')";
  html += "    .then(response => {";
  html += "      if (!response.ok) throw new Error('No GPIO image available');";
  html += "      return response.blob();";
  html += "    })";
  html += "    .then(blob => {";
  html += "      const img = document.createElement('img');";
  html += "      img.src = URL.createObjectURL(blob);";
  html += "      document.getElementById('imageContainer').innerHTML = '';";
  html += "      document.getElementById('imageContainer').appendChild(img);";
  html += "      document.getElementById('status').innerText = 'Last GPIO image loaded!';";
  html += "      document.getElementById('status').style.color = 'green';";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerText = 'No GPIO image available';";
  html += "      document.getElementById('status').style.color = 'orange';";
  html += "      console.error('Error:', error);";
  html += "    });";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// Core image capture function
camera_fb_t* captureImage() {
  // Turn on flash LED
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(100); // Brief flash
  
  // Capture image
  camera_fb_t * fb = esp_camera_fb_get();
  
  // Turn off flash LED
  digitalWrite(LED_GPIO_NUM, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return nullptr;
  }
  
  Serial.printf("Image captured: %d bytes\n", fb->len);
  return fb;
}


// Function for GPIO trigger (captures and stores image)
void captureImageAndRespond() {
  // Free previous image if exists
  if (lastCapturedImage) {
    esp_camera_fb_return(lastCapturedImage);
    lastCapturedImage = nullptr;
  }
  
  // Capture new image
  lastCapturedImage = captureImage();
  if (lastCapturedImage) {
    Serial.printf("GPIO3 triggered capture: %d bytes (stored for web access)\n", lastCapturedImage->len);
    imageAvailableFromGPIO = true;
  }
}

// Web capture endpoint
void handleCapture() {
  camera_fb_t * fb = captureImage();
  if (!fb) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }
  
  // Send image as JPEG
  server.sendHeader("Content-Type", "image/jpeg");
  server.sendHeader("Content-Length", String(fb->len));
  server.sendHeader("Cache-Control", "no-cache");
  
  // Send the image data
  WiFiClient client = server.client();
  client.write(fb->buf, fb->len);
  
  // Return frame buffer
  esp_camera_fb_return(fb);
  
  Serial.println("Image sent to web client");
}

// Handle request for last GPIO-captured image
void handleLastImage() {
  if (lastCapturedImage && imageAvailableFromGPIO) {
    Serial.println("Serving last GPIO-captured image to web client");
    
    // Send image as JPEG
    server.sendHeader("Content-Type", "image/jpeg");
    server.sendHeader("Content-Length", String(lastCapturedImage->len));
    server.sendHeader("Cache-Control", "no-cache");
    
    // Send the image data
    WiFiClient client = server.client();
    client.write(lastCapturedImage->buf, lastCapturedImage->len);
    
    Serial.println("Last GPIO image sent to web client");
  } else {
    server.send(404, "text/plain", "No GPIO-captured image available");
    Serial.println("No GPIO-captured image available");
  }
}
