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

unsigned long lastCaptureTime = 0;
const long captureInterval = 10000; // 10 seconds

void captureAndSaveImage() {
  Serial.println("\nCapturing image...");
  // Capture image
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(5000);
    return;
  }
  Serial.printf("Image captured: %d bytes\n", fb->len);
  // Save image to LittleFS
  String filename = "/image_" + String(millis()) + ".jpg";
  File file = LittleFS.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    esp_camera_fb_return(fb);
    delay(5000);
    return;
  }
  // Write image data to file in chunks
  size_t written = 0;
  size_t chunk_size = 1024; // 1KB chunks
  uint8_t* buf_ptr = fb->buf;
  size_t remaining = fb->len;
  while (remaining > 0) {
    size_t to_write = (remaining > chunk_size) ? chunk_size : remaining;
    size_t chunk_written = file.write(buf_ptr, to_write);
    written += chunk_written;
    buf_ptr += chunk_written;
    remaining -= chunk_written;
    // Removed yield() to prevent watchdog errors
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
}

void setup() {
  delay(1000); // Delay for stability
  Serial.begin(115200);
  Serial.println("ESP32-CAM Simple Image Capture to LittleFS");
  
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
  config.frame_size = FRAMESIZE_UXGA;  // High resolution
  config.jpeg_quality = 10;            // High quality (lower number = better quality)
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  // Check if PSRAM is available and adjust settings
  if (psramFound()) {
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
    Serial.println("PSRAM found - using high quality settings");
  } else {
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    Serial.println("No PSRAM - using standard quality settings");
  }
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
}

void loop() {
  if (millis() - lastCaptureTime >= captureInterval) {
    captureAndSaveImage();
    lastCaptureTime = millis();
  }
  delay(100); // Small delay
}
