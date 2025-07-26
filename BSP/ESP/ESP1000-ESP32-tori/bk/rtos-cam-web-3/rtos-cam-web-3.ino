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
#include <Wire.h>
#include "SSD1306Wire.h"
#include <BMx280I2C.h>
#include <driver/i2s.h>

// Camera sensor definitions
#define OV2640_PID 0x2642

// ===========================
// I2S Microphone Configuration (INMP441)
// ===========================
#define I2S_WS 2        // Word Select (LRCLK)
#define I2S_SD 14       // Serial Data
#define I2S_SCK 12      // Serial Clock (BCLK)
#define I2S_PORT I2S_NUM_0
#define I2S_BUFFER_LEN 64
#define RECORDING_DURATION_SEC 10
#define SAMPLE_RATE 22050  // Lower sample rate to reduce file size

// ===========================
// WiFi credentials
// ===========================
const char* ssid = "111";
const char* password = "electrodragon";

// ===========================
// OLED Display Configuration
// ===========================
// SDA = GPIO 15, SCL = GPIO 13 (using same pins as reference code)
SSD1306Wire display(0x3c, 15, 13);

// ===========================
// BMP280 Sensor Configuration
// ===========================
BMx280I2C bmx280(0x76);

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

// LED pins for capture indication
#define LED1_GPIO_PIN 33
#define LED2_GPIO_PIN 4

// FreeRTOS handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t audioTaskHandle = NULL;
SemaphoreHandle_t fileMutex = NULL;

unsigned long lastCaptureTime = 0;
// Removed captureInterval - only manual and GPIO trigger capture

// Audio recording control
bool audioRecordRequested = false;
bool isRecording = false;

// BMP280 sensor readings (shared between tasks)
float currentTemperature = 0.0;
double currentPressure = 0.0;
bool sensorDataValid = false;

// GPIO interrupt handler - minimal and safe to prevent cache issues
void IRAM_ATTR gpio3InterruptHandler() {
  // Minimal ISR - just set flags, no complex operations or timer calls
  static volatile uint32_t last_interrupt_time = 0;
  uint32_t current_time = millis(); // Use millis() instead of esp_timer
  
  // Simple debounce - ignore interrupts within 500ms
  if (current_time - last_interrupt_time > 500) {
    gpioTriggered = true; // Set flag for GPIO trigger
    audioRecordRequested = true; // Also trigger audio recording
    last_interrupt_time = current_time;
  }
}

void blinkCaptureLEDs() {
  // Blink both LEDs twice quickly
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1_GPIO_PIN, HIGH);
    digitalWrite(LED2_GPIO_PIN, HIGH);
    delay(100); // LED on for 100ms
    digitalWrite(LED1_GPIO_PIN, LOW);
    digitalWrite(LED2_GPIO_PIN, LOW);
    delay(100); // LED off for 100ms
  }
}

// Global variable to track I2S initialization status
bool i2s_initialized = false;

void i2s_install() {
  Serial.println("=== I2S Microphone Initialization ===");
  
  // Set up I2S Processor configuration with different interrupt allocation
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,  // Use Level 1 interrupt instead of default
    .dma_buf_count = 4,                        // Reduced from 8 to save memory
    .dma_buf_len = I2S_BUFFER_LEN,
    .use_apll = false
  };
  
  Serial.printf("I2S Config:\n");
  Serial.printf("  Sample Rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("  Bits per Sample: 16\n");
  Serial.printf("  DMA Buffer Count: 4 (reduced for ESP32-CAM)\n");
  Serial.printf("  DMA Buffer Length: %d\n", I2S_BUFFER_LEN);
  Serial.printf("  Mode: I2S_MODE_MASTER | I2S_MODE_RX\n");
  Serial.printf("  Interrupt Flags: ESP_INTR_FLAG_LEVEL1\n");
  
  esp_err_t result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  if (result == ESP_OK) {
    Serial.println("✅ I2S driver installed successfully");
    i2s_initialized = true;
  } else {
    Serial.printf("❌ I2S driver install failed with error: 0x%x\n", result);
    Serial.printf("Error details: %s\n", esp_err_to_name(result));
    
    // If installation fails, we'll try to install it later when needed
    Serial.println("⚠️  I2S installation deferred - will try again during recording");
    i2s_initialized = false;
  }
}

void i2s_setpin() {
  // Only try to set pins if I2S driver was successfully installed
  if (!i2s_initialized) {
    Serial.println("❌ Skipping I2S pin configuration - driver not installed");
    return;
  }
  
  Serial.println("=== I2S Pin Configuration ===");
  
  // Set I2S pin configuration
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = -1,
    .data_in_num = I2S_SD
  };
  
  Serial.printf("I2S Pin Mapping:\n");
  Serial.printf("  BCK (Serial Clock): GPIO %d\n", I2S_SCK);
  Serial.printf("  WS (Word Select/LRCLK): GPIO %d\n", I2S_WS);
  Serial.printf("  SD (Serial Data): GPIO %d\n", I2S_SD);
  Serial.printf("  Data Out: Not used (-1)\n");
  
  esp_err_t result = i2s_set_pin(I2S_PORT, &pin_config);
  if (result == ESP_OK) {
    Serial.println("✅ I2S pins configured successfully");
  } else {
    Serial.printf("❌ I2S pin configuration failed with error: 0x%x\n", result);
    Serial.printf("Error details: %s\n", esp_err_to_name(result));
  }
}

bool ensureI2SReady() {
  // If I2S was never initialized, try to initialize it now
  if (!i2s_initialized) {
    Serial.println("I2S not initialized, attempting to install...");
    
    // Try multiple fallback configurations to work around interrupt allocation issues
    const i2s_config_t fallback_configs[] = {
      // Config 1: Shared interrupt with minimal resources
      {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 16000,  // Lower sample rate
        .bits_per_sample = i2s_bits_per_sample_t(16),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_LEVEL3,  // Shared interrupt
        .dma_buf_count = 2,
        .dma_buf_len = 32,
        .use_apll = false
      },
      // Config 2: No specific interrupt flags (let system choose)
      {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 8000,   // Even lower sample rate
        .bits_per_sample = i2s_bits_per_sample_t(16),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,  // No specific flags - let ESP-IDF decide
        .dma_buf_count = 2,
        .dma_buf_len = 16,      // Very small buffers
        .use_apll = false
      },
      // Config 3: IRAM allocation flag to avoid cache issues
      {
        .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = 8000,
        .bits_per_sample = i2s_bits_per_sample_t(16),
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LEVEL3,  // IRAM allocation
        .dma_buf_count = 2,
        .dma_buf_len = 16,
        .use_apll = false
      }
    };
    
    for (int config_idx = 0; config_idx < 3; config_idx++) {
      Serial.printf("Trying I2S config %d (sample rate: %d Hz)...\n", config_idx + 1, fallback_configs[config_idx].sample_rate);
      
      esp_err_t install_result = i2s_driver_install(I2S_PORT, &fallback_configs[config_idx], 0, NULL);
      if (install_result == ESP_OK) {
        Serial.printf("✅ I2S driver installed with config %d\n", config_idx + 1);
        
        // Set pins
        const i2s_pin_config_t pin_config = {
          .bck_io_num = I2S_SCK,
          .ws_io_num = I2S_WS,
          .data_out_num = -1,
          .data_in_num = I2S_SD
        };
        
        esp_err_t pin_result = i2s_set_pin(I2S_PORT, &pin_config);
        if (pin_result == ESP_OK) {
          i2s_initialized = true;
          Serial.println("✅ I2S driver installed successfully on demand");
          break;  // Success, exit loop
        } else {
          Serial.printf("Pin config failed: 0x%x (%s)\n", pin_result, esp_err_to_name(pin_result));
          i2s_driver_uninstall(I2S_PORT);  // Clean up
        }
      } else {
        Serial.printf("Config %d failed: 0x%x (%s)\n", config_idx + 1, install_result, esp_err_to_name(install_result));
        if (install_result == ESP_ERR_NOT_FOUND) {
          Serial.println("  -> No free interrupts available");
        }
      }
      
      // Small delay between attempts
      delay(100);
    }
    
    if (!i2s_initialized) {
      Serial.println("❌ All I2S configurations failed - audio permanently disabled");
      Serial.println("   This is expected on ESP32-CAM due to limited interrupt resources");
      return false;
    }
  }
  
  // Try to start I2S to test if it's working
  esp_err_t test_result = i2s_start(I2S_PORT);
  if (test_result == ESP_OK) {
    i2s_stop(I2S_PORT);  // Stop it immediately
    return true;
  }
  
  Serial.printf("I2S start test failed: 0x%x (%s)\n", test_result, esp_err_to_name(test_result));
  return false;
}

void recordAudio() {
  if (isRecording) {
    Serial.println("Already recording, skipping...");
    return;
  }
  
  isRecording = true;
  Serial.println("\n=== Starting Audio Recording ===");
  Serial.printf("Recording Duration: %d seconds\n", RECORDING_DURATION_SEC);
  Serial.printf("Sample Rate: %d Hz\n", SAMPLE_RATE);
  Serial.printf("Buffer Size: %d samples\n", I2S_BUFFER_LEN);
  
  // Check if I2S is ready, try to fix if not
  if (!ensureI2SReady()) {
    Serial.println("❌ I2S initialization failed - cannot record audio");
    Serial.println("   This is normal on ESP32-CAM due to limited interrupt resources");
    Serial.println("   Camera functionality remains unaffected");
    isRecording = false;
    return;
  }
  
  // Take mutex to protect file operations with shorter timeout to prevent blocking
  if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    String filename = "/audio_" + String(millis()) + ".raw";
    File file = LittleFS.open(filename, FILE_WRITE);
    if (!file) {
      Serial.println("Failed to open audio file for writing");
      isRecording = false;
      xSemaphoreGive(fileMutex);
      return;
    }
    
    int16_t sBuffer[I2S_BUFFER_LEN];
    size_t totalBytesWritten = 0;
    unsigned long recordingStartTime = millis();
    unsigned long recordingDuration = RECORDING_DURATION_SEC * 1000; // Convert to ms
    
    // Start I2S with error handling
    Serial.println("Starting I2S interface...");
    esp_err_t i2s_start_result = i2s_start(I2S_PORT);
    if (i2s_start_result == ESP_OK) {
      Serial.println("✅ I2S started successfully");
    } else {
      Serial.printf("❌ I2S start failed: 0x%x (%s)\n", i2s_start_result, esp_err_to_name(i2s_start_result));
      file.close();
      isRecording = false;
      xSemaphoreGive(fileMutex);
      return;
    }
    
    Serial.println("Beginning audio capture loop...");
    unsigned long loopCount = 0;
    int consecutive_errors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 10;
    
    while ((millis() - recordingStartTime) < recordingDuration && consecutive_errors < MAX_CONSECUTIVE_ERRORS) {
      size_t bytesIn = 0;
      esp_err_t result = i2s_read(I2S_PORT, &sBuffer, I2S_BUFFER_LEN * sizeof(int16_t), &bytesIn, pdMS_TO_TICKS(100));
      loopCount++;
      
      if (result == ESP_OK && bytesIn > 0) {
        size_t bytesWritten = file.write((uint8_t*)sBuffer, bytesIn);
        totalBytesWritten += bytesWritten;
        consecutive_errors = 0;  // Reset error counter
        
        // Print progress every 100 loops
        if (loopCount % 100 == 0) {
          Serial.printf("Recording progress: %lu bytes written, loop %lu\n", totalBytesWritten, loopCount);
        }
        
        if (bytesWritten != bytesIn) {
          Serial.printf("Audio write error: expected %d, wrote %d\n", bytesIn, bytesWritten);
          consecutive_errors++;
        }
      } else {
        consecutive_errors++;
        if (result != ESP_OK) {
          Serial.printf("I2S read error: 0x%x (%s), bytesIn: %d\n", result, esp_err_to_name(result), bytesIn);
        }
      }
      
      // Allow other tasks to run and feed watchdog
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
      Serial.println("⚠️  Too many I2S errors, stopping recording early");
    }
    
    // Stop I2S
    Serial.println("Stopping I2S interface...");
    esp_err_t i2s_stop_result = i2s_stop(I2S_PORT);
    if (i2s_stop_result == ESP_OK) {
      Serial.println("✅ I2S stopped successfully");
    } else {
      Serial.printf("❌ I2S stop failed: 0x%x (%s)\n", i2s_stop_result, esp_err_to_name(i2s_stop_result));
    }
    
    file.close();
    Serial.printf("=== Audio Recording Complete ===\n");
    Serial.printf("File: %s\n", filename.c_str());
    Serial.printf("Size: %d bytes\n", totalBytesWritten);
    Serial.printf("Duration: %d seconds\n", RECORDING_DURATION_SEC);
    Serial.printf("Loops executed: %lu\n", loopCount);
    Serial.printf("Errors encountered: %d\n", consecutive_errors);
    
    // Only verify file if we actually wrote something
    if (totalBytesWritten > 0) {
      if (LittleFS.exists(filename)) {
        Serial.println("Audio file successfully created and exists in filesystem");
      } else {
        Serial.println("ERROR: Audio file was not created!");
      }
    } else {
      Serial.println("WARNING: No audio data was written (I2S may not be working)");
    }
    
    xSemaphoreGive(fileMutex);
  } else {
    Serial.println("Failed to take file mutex for audio recording");
  }
  
  isRecording = false;
}

void captureAndSaveImage() {
  Serial.println("\nCapturing image...");
  
  // Trigger LED blink to indicate capture
  blinkCaptureLEDs();
  
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
    String html = "<html><head><title>ESP32-CAM + BMP280 Sensor</title>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    html += "<style>body{font-family:sans-serif; background-color:#222; color: #fff; padding:20px;} ";
    html += "table{border-collapse:collapse; width:100%;} th,td{padding:8px;text-align:left;border-bottom:1px solid #444;} ";
    html += "tr:hover{background-color:#333;} a{text-decoration:none;color:#00abff;} ";
    html += ".button{background-color:#00abff; color:white; padding:10px 20px; text-align:center; display:inline-block; font-size:16px; margin:10px 5px; border:none; cursor:pointer; border-radius:5px;} ";
    html += "img{max-width:200px; border:1px solid #666;}</style>";
    html += "</head><body><h1>ESP32-CAM + BMP280 + I2S Mic (FreeRTOS)</h1>";
    html += "<p><a href='/capture' class='button'>Take New Picture</a> ";
    html += "<a href='/record' class='button'>Record Audio (10s)</a> ";
    html += "<a href='/' class='button'>Refresh</a></p>";
    if (isRecording) {
      html += "<p style='color: red; font-weight: bold;'>🔴 Recording in progress...</p>";
    }
    html += "<h2>Captured Images</h2>";
    html += "<table><tr><th>Image</th><th>File</th><th>Size</th><th>Actions</th></tr>";

    File root = LittleFS.open("/");
    File file = root.openNextFile();
    int imageCount = 0;
    int audioCount = 0;
    
    // Collect all files first
    String imageFiles[50];  // Array to store image filenames
    String audioFiles[50];  // Array to store audio filenames
    int imageFileSizes[50];
    int audioFileSizes[50];
    
    // Scan all files and categorize them
    while(file){
        String fileName = String(file.name());
        Serial.println("Found file: " + fileName);
        
        if(fileName.endsWith(".jpg") && imageCount < 50) {
            imageFiles[imageCount] = fileName;
            imageFileSizes[imageCount] = file.size();
            imageCount++;
        }
        else if(fileName.endsWith(".raw") && audioCount < 50) {
            audioFiles[audioCount] = fileName;
            audioFileSizes[audioCount] = file.size();
            audioCount++;
        }
        
        file = root.openNextFile();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
    root.close();
    
    // Display images
    for(int i = 0; i < imageCount; i++) {
        html += "<tr>";
        html += "<td><img src='/download?file=" + imageFiles[i] + "' alt='" + imageFiles[i] + "'></td>";
        html += "<td>" + imageFiles[i] + "</td>";
        html += "<td>" + String(imageFileSizes[i]) + " bytes</td>";
        html += "<td><a href='/download?file=" + imageFiles[i] + "' target='_blank'>View</a></td>";
        html += "</tr>";
    }
    
    if (imageCount == 0) {
      html += "<tr><td colspan='4'>No images found. Take a picture first!</td></tr>";
    }
    
    html += "</table>";
    
    // Add Audio Files section
    html += "<h2>Audio Recordings</h2>";
    html += "<table><tr><th>Type</th><th>File</th><th>Size</th><th>Actions</th></tr>";
    
    // Display audio files
    for(int i = 0; i < audioCount; i++) {
        html += "<tr>";
        html += "<td>Audio (RAW)</td>";
        html += "<td>" + audioFiles[i] + "</td>";
        html += "<td>" + String(audioFileSizes[i]) + " bytes</td>";
        html += "<td><a href='/download?file=" + audioFiles[i] + "' download>Download</a></td>";
        html += "</tr>";
    }
    
    if (audioCount == 0) {
      html += "<tr><td colspan='4'>No audio recordings found. Record audio first!</td></tr>";
    }
    
    html += "</table>";
    
    // Add BMP280 sensor data section
    html += "<h2>Sensor Data (BMP280)</h2>";
    html += "<div style='background-color:#333; padding:15px; border-radius:5px; margin:10px 0;'>";
    if (sensorDataValid) {
      html += "<p><strong>Temperature:</strong> " + String(currentTemperature, 1) + " °C</p>";
      html += "<p><strong>Pressure:</strong> " + String(currentPressure, 0) + " Pa</p>";
      html += "<p><strong>Pressure:</strong> " + String(currentPressure / 100.0, 2) + " hPa</p>";
    } else {
      html += "<p><strong>Sensor Status:</strong> Reading... or Not Connected</p>";
    }
    html += "</div>";
    
    html += "<p>Total images: " + String(imageCount) + " | Audio files: " + String(audioCount) + "</p>";
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
        
        // Determine MIME type based on file extension
        String contentType = "application/octet-stream"; // Default
        if (path.endsWith(".jpg") || path.endsWith(".jpeg")) {
          contentType = "image/jpeg";
        } else if (path.endsWith(".raw")) {
          contentType = "audio/raw";
        }
        
        server.streamFile(file, contentType);
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
void displayTask(void *pvParameters) {
  // Initialize display
  display.init();
  delay(50);
  display.clear();
  display.display();
  
  for(;;) {
    display.clear();
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    
    // Display title
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "ESP32-CAM + BMP280");
    
    // Display BMP280 sensor data
    if (sensorDataValid) {
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 12, "Temp: " + String(currentTemperature, 1) + " C");
      display.drawString(0, 22, "Press: " + String(currentPressure, 0) + " Pa");
    } else {
      display.drawString(0, 12, "Sensor: Reading...");
    }
    
    // Display WiFi status (compact)
    if (WiFi.status() == WL_CONNECTED) {
      display.drawString(0, 34, "WiFi: " + WiFi.localIP().toString());
    } else {
      display.drawString(0, 34, "WiFi: Disconnected");
    }
    
    // Display memory info
    display.drawString(0, 46, "Heap: " + String(ESP.getFreeHeap()));
    
    // Display capture count (if files exist)
    display.drawString(0, 56, "Images captured");
    
    display.display();
    
    // Update every 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void sensorTask(void *pvParameters) {
  for(;;) {
    // Start a measurement
    if (bmx280.measure()) {
      // Wait for the measurement to finish
      while (!bmx280.hasValue()) {
        vTaskDelay(pdMS_TO_TICKS(100));
      }
      
      // Read sensor values
      currentTemperature = bmx280.getTemperature();
      currentPressure = bmx280.getPressure64();
      sensorDataValid = true;
      
      Serial.printf("BMP280 - Temp: %.1f°C, Pressure: %.0f Pa\n", 
                   currentTemperature, currentPressure);
    } else {
      Serial.println("BMP280 measurement failed");
      sensorDataValid = false;
    }
    
    // Read sensor every 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void audioTask(void *pvParameters) {
  Serial.println("Audio task started");
  for(;;) {
    // Check for audio recording request
    if (audioRecordRequested && !isRecording) {
      Serial.println("Audio recording request detected in task");
      recordAudio();
      audioRecordRequested = false;
    }
    
    // Task delay
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void cameraTask(void *pvParameters) {
  for(;;) {
    // Check for GPIO3 trigger (highest priority)
    if (gpioTriggered) {
      Serial.println("GPIO3 trigger detected - capturing image");
      captureAndSaveImage();
      gpioTriggered = false;
    }
    // Check for manual capture request
    else if (captureRequested) {
      captureAndSaveImage();
      captureRequested = false;
    }
    
    // No auto capture - only manual and GPIO trigger
    
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
  delay(1000); // Reduced delay to prevent watchdog
  Serial.begin(115200);
  Serial.println("ESP32-CAM FreeRTOS Image Capture to LittleFS");
  
  // Allow system to stabilize early and often during setup
  delay(100);
  
  // Print initial memory info
  Serial.printf("Initial free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM available: %s\n", psramFound() ? "Yes" : "No");
  
  // Create file mutex
  fileMutex = xSemaphoreCreateMutex();
  if (fileMutex == NULL) {
    Serial.println("Failed to create file mutex");
    return;
  }
  
  // Allow system to process after mutex creation
  delay(50);
  
  // Initialize I2C for OLED display (SDA=15, SCL=13)
  Wire.begin(15, 13);
  Serial.println("I2C initialized for OLED display");
  
  // Allow system to process after I2C
  delay(50);
  
  // Initialize BMP280 sensor
  if (!bmx280.begin()) {
    Serial.println("BMP280 sensor initialization failed! Check connections.");
    // Continue without sensor (sensorDataValid will remain false)
  } else {
    if (bmx280.isBME280()) {
      Serial.println("BME280 sensor detected");
    } else {
      Serial.println("BMP280 sensor detected");
    }
    
    // Reset sensor to default parameters and configure
    bmx280.resetToDefaults();
    bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
    bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
    Serial.println("BMP280 sensor configured successfully");
  }
  
  // Allow system to process after sensor initialization
  delay(100);
  
  // Initialize LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed - formatting...");
    delay(100); // Allow system to process before formatting
    if (!LittleFS.format()) {
      Serial.println("LittleFS Format Failed");
      return;
    }
    delay(100); // Allow system to process after formatting
    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS Mount Failed after format");
      return;
    }
  }
  Serial.println("LittleFS mounted successfully");
  
  // Allow system to process after LittleFS
  delay(100);
  
  // Camera configuration (initialize camera BEFORE I2S to avoid interrupt conflicts)
  Serial.println("\n=== Starting Camera Initialization ===");
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
  
  // Allow system to process before camera initialization
  delay(100);
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return;
  }
  Serial.println("Camera initialized successfully");
  
  // Allow system to process after camera initialization
  delay(100);
  // Get camera sensor and adjust settings for OV2640
  sensor_t * s = esp_camera_sensor_get();
  if (s->id.PID == OV2640_PID) {
    s->set_vflip(s, 1);        // Flip vertically
    s->set_brightness(s, 1);   // Increase brightness
    s->set_saturation(s, -2);  // Decrease saturation
    Serial.println("OV2640 sensor configured");
  } else {
    Serial.println("Warning: Expected OV2640 sensor, found different sensor");
  }
  Serial.println("Camera initialization complete.");
  
  // Allow system to process after camera sensor configuration
  delay(100);
  
  // Initialize I2S microphone AFTER camera to avoid interrupt conflicts
  Serial.println("\n=== Starting I2S Microphone Setup ===");
  Serial.println("⚠️  Skipping I2S initialization during setup to avoid interrupt conflicts");
  Serial.println("   I2S will be initialized on-demand when audio recording is requested");
  Serial.println("   This is safer on ESP32-CAM with limited interrupt resources");
  
  // Don't try to initialize I2S during setup - it will fail due to interrupt conflicts
  // i2s_install();
  // i2s_setpin();
  
  // Allow system to process after I2S setup decision
  delay(100);
  
  Serial.println("=== I2S Microphone Setup Deferred ===\n");
  Serial.println("Setup complete. Camera ready, I2S microphone will initialize on first use.");
  Serial.println("Note: This approach prevents interrupt conflicts during startup.");
  
  // Setup GPIO3 as input with pull-down for external trigger
  pinMode(TRIGGER_GPIO_PIN, INPUT_PULLDOWN);
  
  // Allow system to process before GPIO interrupt setup
  delay(100);
  
  // Check if GPIO ISR service is already installed before trying to attach interrupt
  esp_err_t gpio_isr_result = gpio_install_isr_service(0);
  if (gpio_isr_result == ESP_ERR_INVALID_STATE) {
    Serial.println("GPIO ISR service already installed - continuing...");
  } else if (gpio_isr_result != ESP_OK) {
    Serial.printf("Failed to install GPIO ISR service: 0x%x\n", gpio_isr_result);
  }
  
  // Small delay before setting up interrupt
  delay(50); // Reduced delay
  
  // Try to attach interrupt with error handling
  if (digitalPinToInterrupt(TRIGGER_GPIO_PIN) != -1) {
    attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
    Serial.println("GPIO3 interrupt configured for rising edge trigger");
  } else {
    Serial.println("Warning: GPIO3 interrupt attachment failed - trigger may not work");
  }
  
  // Setup LED pins for capture indication
  pinMode(LED1_GPIO_PIN, OUTPUT);
  pinMode(LED2_GPIO_PIN, OUTPUT);
  digitalWrite(LED1_GPIO_PIN, LOW); // Start with LEDs off
  digitalWrite(LED2_GPIO_PIN, LOW);
  Serial.println("LED pins (GPIO33, GPIO4) configured for capture indication");
  
  // Allow system to process before WiFi connection
  delay(100);
  
  // Connect to WiFi with error handling and slower retries
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 15) { // Reduced retries
    delay(400); // Reduced delay
    Serial.print(".");
    wifi_retry++;
    
    // Allow system to process every few retries
    if (wifi_retry % 3 == 0) {
      delay(100);
    }
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: http://");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi. Continuing without WiFi...");
  }
  
  // Allow system to process after WiFi connection
  delay(100);
  
  // Setup server routes only if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("=== Setting up Web Server Routes ===");
    
    server.on("/", handleRoot);
    Serial.println("✅ Route registered: / (handleRoot)");
    
    server.on("/download", handleFileDownload);
    Serial.println("✅ Route registered: /download (handleFileDownload)");
    
    server.on("/capture", handleCapture);
    Serial.println("✅ Route registered: /capture (handleCapture)");
    
    server.on("/record", [](){
        Serial.println("'/record' endpoint called");
        if (!isRecording) {
            Serial.println("Setting audioRecordRequested = true");
            audioRecordRequested = true;
            server.send(200, "text/plain", "Audio recording started");
        } else {
            Serial.println("Recording already in progress");
            server.send(429, "text/plain", "Recording already in progress");
        }
    });
    Serial.println("✅ Route registered: /record (lambda function)");
    
    // Allow system to process before server start
    delay(100);
    
    server.begin();
    Serial.println("✅ HTTP server started successfully");
    Serial.printf("Server accessible at: http://%s/\n", WiFi.localIP().toString().c_str());
    Serial.println("Available endpoints:");
    Serial.println("  GET  /          - Main page");
    Serial.println("  GET  /capture   - Take photo");
    Serial.println("  GET  /record    - Record audio");
    Serial.println("  GET  /download  - Download files");
  } else {
    Serial.println("WiFi not connected - skipping web server setup");
  }
  
  // Allow system to process before task creation
  delay(100);
  
  // Create FreeRTOS tasks with conservative stack sizes for no PSRAM
  Serial.println("=== Creating FreeRTOS Tasks ===");
  
  // Create display task first
  xTaskCreatePinnedToCore(
    displayTask,          // Task function
    "DisplayTask",        // Name of task
    4096,                 // Stack size (bytes) - smaller for display
    NULL,                 // Parameter to pass
    1,                    // Task priority (lower priority)
    &displayTaskHandle,   // Task handle
    0                     // Core 0
  );
  Serial.println("✅ Display task created");
  
  // Allow system to process between task creations
  delay(50);
  
  // Create sensor task
  xTaskCreatePinnedToCore(
    sensorTask,           // Task function
    "SensorTask",         // Name of task
    3072,                 // Stack size (bytes) - small for sensor reading
    NULL,                 // Parameter to pass
    1,                    // Task priority (same as display)
    &sensorTaskHandle,    // Task handle
    0                     // Core 0
  );
  Serial.println("✅ Sensor task created");
  
  // Allow system to process
  delay(50);
  
  // Create audio task
  xTaskCreatePinnedToCore(
    audioTask,            // Task function
    "AudioTask",          // Name of task
    8192,                 // Stack size (bytes) - larger for audio processing
    NULL,                 // Parameter to pass
    2,                    // Task priority (high for audio)
    &audioTaskHandle,     // Task handle
    1                     // Core 1
  );
  Serial.println("✅ Audio task created");
  
  // Allow system to process
  delay(50);
  
  xTaskCreatePinnedToCore(
    cameraTask,           // Task function
    "CameraTask",         // Name of task
    10240,                // Stack size (bytes) - reduced for no PSRAM
    NULL,                 // Parameter to pass
    2,                    // Task priority (higher number = higher priority)
    &cameraTaskHandle,    // Task handle
    1                     // Core 1 (core 0 is for WiFi/Bluetooth)
  );
  Serial.println("✅ Camera task created");
  
  // Allow system to process
  delay(50);
  
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
    Serial.println("✅ Web server task created");
    Serial.println("🎯 FreeRTOS tasks created successfully (including web server, display, sensor, and audio)");
  } else {
    Serial.println("🎯 FreeRTOS camera, display, sensor, and audio tasks created successfully (web server disabled)");
  }
  
  // Final system processing delay and completion message
  delay(100);
  Serial.printf("Free heap after setup: %u bytes\n", ESP.getFreeHeap());
  Serial.println("=== Setup Complete - System Ready ===");
}

void loop() {
  // Main loop now just monitors tasks and prints status
  static unsigned long lastStatusPrint = 0;
  
  if (millis() - lastStatusPrint >= 30000) { // Print status every 30 seconds
    Serial.printf("=== System Status ===\n");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Camera task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(cameraTaskHandle));
    Serial.printf("Display task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(displayTaskHandle));
    Serial.printf("Sensor task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(sensorTaskHandle));
    Serial.printf("Audio task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(audioTaskHandle));
    if (webServerTaskHandle != NULL) {
      Serial.printf("Web server task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(webServerTaskHandle));
    }
    Serial.printf("WiFi status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (sensorDataValid) {
      Serial.printf("BMP280: %.1f°C, %.0f Pa\n", currentTemperature, currentPressure);
    }
    if (isRecording) {
      Serial.println("Audio: Currently recording...");
    }
    lastStatusPrint = millis();
  }
  
  // Small delay to prevent the loop from consuming too much CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}
