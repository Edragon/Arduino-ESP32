// run 


// Capturing image...
// Image captured: 12396 bytes
// Image saved successfully as /image_20817.jpg (12396 bytes)
// SPIFFS Usage: 69778/1318001 bytes (5.3% full)
// Recent files in SPIFFS:
//   image_303.jpg (14050 bytes)
//   image_10572.jpg (14050 bytes)
//   image_342.jpg (13965 bytes)
//   image_10519.jpg (13734 bytes)
//   image_20817.jpg (12396 bytes)
//   ... (showing only recent 5 files)
// Waiting 10 seconds before next capture...



#include "esp_camera.h"
#include "FS.h"
#include "LittleFS.h"
#include <WiFi.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

// ===========================
// WiFi credentials
// ===========================
const char* ssid = "111";
const char* password = "electrodragon";

// ===========================
// Simple ESP32-CAM Image Capture
// ===========================

// ===================
// Select camera model
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

WebServer server(80);
bool captureRequested = false; // Flag for manual capture requests
bool gpioTriggered = false;    // Flag for GPIO3 trigger

// GPIO pin for external trigger
#define TRIGGER_GPIO_PIN 3

// FreeRTOS handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
SemaphoreHandle_t fileMutex = NULL;

unsigned long lastCaptureTime = 0;
// const long captureInterval = 60000; // Auto-capture interval in milliseconds (60 seconds)

// GPIO interrupt handler
void IRAM_ATTR gpio3InterruptHandler() {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  gpioTriggered = true; // Set flag for GPIO trigger
  // Yield to higher priority task if needed
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void captureAndSaveImage() {
  Serial.println("\nCapturing image...");
  
  // Take mutex to protect file operations
  if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
    // Capture image
    camera_fb_t * fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      xSemaphoreGive(fileMutex);
      return;
    }
    Serial.printf("Image captured: %d bytes\n", fb->len);
    
    // Save image to LittleFS
    String filename = "/image_" + String(millis()) + ".jpg";
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open file for writing");
      esp_camera_fb_return(fb);
      xSemaphoreGive(fileMutex);
      return;
    }
    
    // Write image data to file in chunks
    size_t written = 0;
    size_t chunk_size = 512; // Smaller chunks for no PSRAM (512 bytes)
    uint8_t* buf_ptr = fb->buf;
    size_t remaining = fb->len;
    while (remaining > 0) {
      size_t to_write = (remaining > chunk_size) ? chunk_size : remaining;
      size_t chunk_written = file.write(buf_ptr, to_write);
      written += chunk_written;
      buf_ptr += chunk_written;
      remaining -= chunk_written;
      // Allow other tasks to run more frequently
      vTaskDelay(pdMS_TO_TICKS(2));
      if (chunk_written != to_write) {
        Serial.printf("Write error: expected %d, wrote %d\n", to_write, chunk_written);
        break;
      }
    }
    file.close();
    
    if (written == fb->len) {
      Serial.printf("Image saved successfully as %s (%d bytes)\n", filename.c_str(), written);
    } else {
      Serial.printf("Failed to save complete image. Written: %d, Expected: %d\n", written, fb->len);
    }
    esp_camera_fb_return(fb);
    xSemaphoreGive(fileMutex);
  } else {
    Serial.println("Failed to take file mutex, skipping capture");
  }
}

void handleRoot() {
  // Take mutex to protect file operations
  if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    String html = "<html><head><title>ESP32-CAM LittleFS Images</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:sans-serif; background-color:#222; color: #fff; padding:20px;} ";
    html += "table{border-collapse:collapse; width:100%;} th,td{padding:8px;text-align:left;border-bottom:1px solid #444;} ";
    html += "tr:hover{background-color:#333;} a{text-decoration:none;color:#00abff;} ";
    html += ".button{background-color:#00abff; color:white; padding:10px 20px; text-align:center; display:inline-block; font-size:16px; margin:10px 5px; border:none; cursor:pointer; border-radius:5px;} ";
    html += "img{max-width:200px; border:1px solid #666;}</style>";
    html += "</head><body><h1>ESP32-CAM LittleFS Images (FreeRTOS)</h1>";
    html += "<p><a href='/capture' class='button'>Take New Picture</a> ";
    html += "<a href='/' class='button'>Refresh</a></p>";
    html += "<table><tr><th>Image</th><th>File</th><th>Size</th><th>Actions</th></tr>";

    File root = LittleFS.open("/");
    File file = root.openNextFile();
    int fileCount = 0;
    while(file){
        String fileName = String(file.name());
        Serial.println("Found file: " + fileName);
        if(fileName.endsWith(".jpg")) {
            fileCount++;
            html += "<tr>";
            html += "<td><img src='/download?file=" + fileName + "' alt='" + fileName + "'></td>";
            html += "<td>" + fileName + "</td>";
            html += "<td>" + String(file.size()) + " bytes</td>";
            html += "<td><a href='/download?file=" + fileName + "' target='_blank'>View</a></td>";
            html += "</tr>";
        }
        file = root.openNextFile();
        // Allow other tasks to run during file listing
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    root.close();
    
    if (fileCount == 0) {
      html += "<tr><td colspan='4'>No images found. Take a picture first!</td></tr>";
    }
    
    html += "</table>";
    html += "<p>Total images: " + String(fileCount) + "</p>";
    html += "<p>Free heap: " + String(ESP.getFreeHeap()) + " bytes</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
    xSemaphoreGive(fileMutex);
  } else {
    server.send(503, "text/plain", "Service temporarily unavailable - file system busy");
  }
}

void handleFileDownload() {
  if (server.hasArg("file")) {
    String path = server.arg("file");
    // Ensure path starts with /
    if (!path.startsWith("/")) {
      path = "/" + path;
    }
    Serial.println("Downloading file: " + path);
    
    // Take mutex to protect file operations
    if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
      if (LittleFS.exists(path)) {
        File file = LittleFS.open(path, "r");
        server.streamFile(file, "image/jpeg");
        file.close();
      } else {
        Serial.println("File not found: " + path);
        server.send(404, "text/plain", "File not found: " + path);
      }
      xSemaphoreGive(fileMutex);
    } else {
      server.send(503, "text/plain", "Service temporarily unavailable - file system busy");
    }
  } else {
    server.send(400, "text/plain", "Bad Request: file argument missing");
  }
}

void handleCapture() {
  captureRequested = true; // Set flag instead of calling function directly
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "Redirecting...");
}

// FreeRTOS Task Functions
void cameraTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(captureInterval);
  
  for(;;) {
    // Check for GPIO3 trigger (highest priority)
    if (gpioTriggered) {
      Serial.println("GPIO3 trigger detected - capturing image");
      captureAndSaveImage();
      gpioTriggered = false;
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    // Check for manual capture request
    else if (captureRequested) {
      captureAndSaveImage();
      captureRequested = false;
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    
    // Auto capture every interval
    if (xTaskGetTickCount() - xLastWakeTime >= xFrequency) {
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount();
    }
    
    // Task delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void webServerTask(void *pvParameters) {
  for(;;) {
    // Only handle server if WiFi is connected
    if (WiFi.status() == WL_CONNECTED) {
      server.handleClient();
    }
    // Small delay to allow other tasks to run
    vTaskDelay(pdMS_TO_TICKS(50)); // Increased delay
  }
}

void setup() {
  delay(2000); // Increased delay for stability
  Serial.begin(115200);
  Serial.println("ESP32-CAM FreeRTOS Image Capture to LittleFS");
  
  // Print initial memory info
  Serial.printf("Initial free heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM available: %s\n", psramFound() ? "Yes" : "No");
  
  // Create file mutex
  fileMutex = xSemaphoreCreateMutex();
  if (fileMutex == NULL) {
    Serial.println("Failed to create file mutex");
    return;
  }
  
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed - formatting...");
    if (!LittleFS.format()) {
      Serial.println("LittleFS Format Failed");
      return;
    }
    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS Mount Failed after format");
      return;
    }
  }
  Serial.println("LittleFS mounted successfully");
  
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
  config.frame_size = FRAMESIZE_VGA;   // Reduced from UXGA for no PSRAM
  config.jpeg_quality = 15;            // Slightly lower quality for smaller files
  config.fb_count = 1;                 // Single frame buffer for no PSRAM
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  // Optimized settings for boards without PSRAM
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.frame_size = FRAMESIZE_VGA;   // VGA resolution (640x480)
  config.jpeg_quality = 15;            // Quality 15 for reasonable file size
  config.fb_count = 1;                 // Single buffer only
  Serial.println("No PSRAM detected - using optimized settings for DRAM only");
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");
  // Get camera sensor and adjust settings
  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // Flip vertically
    s->set_brightness(s, 1);   // Increase brightness
    s->set_saturation(s, -2);  // Decrease saturation
  }
  Serial.println("Setup complete. Camera ready for capture.");
  
  // Setup GPIO3 as input with pull-down for external trigger
  pinMode(TRIGGER_GPIO_PIN, INPUT_PULLDOWN);
  // Small delay before setting up interrupt
  delay(100);
  attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
  Serial.println("GPIO3 interrupt configured for rising edge trigger");
  
  // Connect to WiFi with error handling
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 20) {
    delay(500);
    Serial.print(".");
    wifi_retry++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Continuing without WiFi...");
  }
  
  // Setup server routes only if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    server.on("/", handleRoot);
    server.on("/download", handleFileDownload);
    server.on("/capture", handleCapture);
    server.begin();
    Serial.println("HTTP server started");
  } else {
    Serial.println("WiFi not connected - skipping web server setup");
  }
  
  // Create FreeRTOS tasks with conservative stack sizes for no PSRAM
  xTaskCreatePinnedToCore(
    cameraTask,           // Task function
    "CameraTask",         // Name of task
    10240,                // Stack size (bytes) - reduced for no PSRAM
    NULL,                 // Parameter to pass
    2,                    // Task priority (higher number = higher priority)
    &cameraTaskHandle,    // Task handle
    1                     // Core 1 (core 0 is for WiFi/Bluetooth)
  );
  
  // Only create web server task if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    xTaskCreatePinnedToCore(
      webServerTask,        // Task function
      "WebServerTask",      // Name of task
      6144,                 // Stack size (bytes) - reduced for no PSRAM
      NULL,                 // Parameter to pass
      1,                    // Task priority
      &webServerTaskHandle, // Task handle
      0                     // Core 0
    );
    Serial.println("FreeRTOS tasks created successfully (including web server)");
  } else {
    Serial.println("FreeRTOS camera task created successfully (web server disabled)");
  }
  Serial.printf("Free heap after setup: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
  // Main loop now just monitors tasks and prints status
  static unsigned long lastStatusPrint = 0;
  
  if (millis() - lastStatusPrint >= 30000) { // Print status every 30 seconds
    Serial.printf("=== System Status ===\n");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("Camera task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(cameraTaskHandle));
    if (webServerTaskHandle != NULL) {
      Serial.printf("Web server task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(webServerTaskHandle));
    }
    Serial.printf("WiFi status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    lastStatusPrint = millis();
  }
  
  // Small delay to prevent the loop from consuming too much CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}
