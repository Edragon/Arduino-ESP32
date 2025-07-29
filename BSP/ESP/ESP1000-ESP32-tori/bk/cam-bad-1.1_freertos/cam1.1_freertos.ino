#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"

// ESP32-CAM OV2640 Camera Server with FreeRTOS Task Management

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

// ===========================
// FreeRTOS Task Priorities and Stack Sizes
// ===========================
#define CAMERA_TASK_PRIORITY     2
#define WEB_SERVER_TASK_PRIORITY 1
#define GPIO_TASK_PRIORITY       3
#define WIFI_TASK_PRIORITY       1

// Increased stack sizes to prevent potential overflow
#define CAMERA_TASK_STACK_SIZE   8192  // Increased from 4096
#define WEB_SERVER_TASK_STACK_SIZE 4096  // Increased from 3072
#define GPIO_TASK_STACK_SIZE     2048  // Increased from 1024
#define WIFI_TASK_STACK_SIZE     4096  // Increased from 2048

// ===========================
// FreeRTOS Handles and Variables
// ===========================
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t gpioTaskHandle = NULL;
TaskHandle_t wifiTaskHandle = NULL;

QueueHandle_t captureQueue = NULL;
SemaphoreHandle_t imageBufferMutex = NULL;
EventGroupHandle_t wifiEventGroup = NULL;
const int WIFI_CONNECTED_BIT = BIT0;

WebServer server(80);

// Camera status tracking
bool cameraInitialized = false;

// Image buffer management
typedef struct {
  camera_fb_t* frameBuffer;
  uint32_t timestamp;
  bool isFromGPIO;
} ImageData_t;

ImageData_t lastCapturedImage = {nullptr, 0, false};

// Queue message types
typedef enum {
  CAPTURE_FROM_WEB,
  CAPTURE_FROM_GPIO,
  CAPTURE_PERIODIC
} CaptureSource_t;

typedef struct {
  CaptureSource_t source;
  uint32_t timestamp;
} CaptureMessage_t;

// GPIO interrupt handler - minimal processing
void IRAM_ATTR gpio3InterruptHandler() {
  // Safety check: ensure queue is created before using it
  if (captureQueue == NULL) {
    return; // Exit early if queue not ready
  }
  
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  CaptureMessage_t msg = {CAPTURE_FROM_GPIO, (uint32_t)xTaskGetTickCountFromISR()};
  
  // Send message to queue from ISR
  xQueueSendFromISR(captureQueue, &msg, &xHigherPriorityTaskWoken);
  
  // Request context switch if higher priority task was woken
  if (xHigherPriorityTaskWoken) {
    portYIELD_FROM_ISR();
  }
}

// ===========================
// WiFi Management Task
// ===========================
void wifiTask(void *pvParameters) {
  Serial.println("WiFi Task: Starting WiFi connection");
  
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  
  Serial.println();
  Serial.println("WiFi Task: WiFi connected!");
  Serial.print("Camera server IP: http://");
  Serial.println(WiFi.localIP());
  
  // Signal that WiFi is connected
  xEventGroupSetBits(wifiEventGroup, WIFI_CONNECTED_BIT);
  
  // Monitor WiFi connection
  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi Task: Connection lost, reconnecting...");
      WiFi.reconnect();
      
      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
      Serial.println("WiFi Task: Reconnected!");
    }
    
    vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
  }
}

// ===========================
// Camera Task - Handles all image capture operations
// ===========================
void cameraTask(void *pvParameters) {
  CaptureMessage_t msg;
  
  Serial.println("Camera Task: Ready to process capture requests");
  
  while (1) {
    // Wait for capture request
    if (xQueueReceive(captureQueue, &msg, portMAX_DELAY) == pdTRUE) {
      Serial.printf("Camera Task: Processing capture request from %s\n", 
                   msg.source == CAPTURE_FROM_GPIO ? "GPIO" : 
                   msg.source == CAPTURE_FROM_WEB ? "Web" : "Periodic");
      
      // Skip capture if camera not initialized
      if (!cameraInitialized) {
        Serial.println("Camera Task: Camera not available - skipping capture");
        vTaskDelay(pdMS_TO_TICKS(1000)); // Wait before next request
        continue;
      }
      
      // Capture image
      camera_fb_t* newFrameBuffer = captureImage();
      
      if (newFrameBuffer) {
        // Take mutex to protect shared image buffer
        if (xSemaphoreTake(imageBufferMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
          // Free previous image if exists
          if (lastCapturedImage.frameBuffer) {
            esp_camera_fb_return(lastCapturedImage.frameBuffer);
          }
          
          // Store new image
          lastCapturedImage.frameBuffer = newFrameBuffer;
          lastCapturedImage.timestamp = msg.timestamp;
          lastCapturedImage.isFromGPIO = (msg.source == CAPTURE_FROM_GPIO);
          
          Serial.printf("Camera Task: Image captured and stored (%d bytes)\n", newFrameBuffer->len);
          
          xSemaphoreGive(imageBufferMutex);
        } else {
          Serial.println("Camera Task: Failed to acquire mutex, releasing frame buffer");
          esp_camera_fb_return(newFrameBuffer);
        }
      } else {
        Serial.println("Camera Task: Image capture failed - will retry on next request");
        // Add delay when capture fails to prevent flooding
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    }
  }
}

// ===========================
// Web Server Task
// ===========================
void webServerTask(void *pvParameters) {
  // Wait for WiFi connection
  xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  
  Serial.println("Web Server Task: Starting web server");
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/capture", handleCapture);
  server.on("/last", handleLastImage);
  server.on("/status", handleStatus);
  server.on("/camera-test", handleCameraTest);  // New endpoint for camera testing
  server.on("/ping", []() { server.send(200, "text/plain", "pong"); }); // Simple ping test
  
  server.begin();
  Serial.println("Web Server Task: Web server started");
  
  while (1) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10)); // Small delay to prevent watchdog issues
  }
}

// ===========================
// GPIO Monitoring Task
// ===========================
void gpioTask(void *pvParameters) {
  Serial.println("GPIO Task: Waiting for WiFi connection to attach interrupt...");

  // Wait for the WiFi connected event
  xEventGroupWaitBits(wifiEventGroup, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

  Serial.println("GPIO Task: WiFi is connected. Attaching GPIO interrupt now.");
  
  // Attach interrupt here, safely after WiFi is up
  attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
  
  while (1) {
    // This task can handle additional GPIO operations or monitoring
    // The actual trigger is handled by the interrupt
    vTaskDelay(pdMS_TO_TICKS(1000)); // Can sleep for longer
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32-CAM FreeRTOS Camera Server Starting...");
  
  // Print memory information before initialization
  printMemoryInfo();
  
  // Create FreeRTOS objects FIRST (before GPIO setup)
  createFreeRTOSObjects();
  
  // Try to initialize camera - but continue even if it fails
  bool cameraWorking = false;
  if (initializeCamera()) {
    cameraWorking = true;
    Serial.println("✅ Camera initialization successful");
  } else {
    Serial.println("❌ Primary camera initialization failed!");
    Serial.println("Trying alternative camera settings...");
    
    // Try with even more conservative settings
    if (initializeCameraConservative()) {
      cameraWorking = true;
      Serial.println("✅ Conservative camera initialization successful");
    } else {
      Serial.println("❌ All camera initialization attempts failed!");
      Serial.println("🚀 System will continue with web server only");
    }
  }
  
  // Always setup GPIO (even without camera)
  setupGPIO();
  
  // Create tasks
  createTasks();
  
  Serial.println("FreeRTOS tasks created successfully");
  if (cameraWorking) {
    Serial.println("📷 System ready with camera functionality!");
  } else {
    Serial.println("🌐 System ready - web server only (no camera)");
  }
  
  // Print final memory status
  printMemoryInfo();
}

void loop() {
  // Empty - all work is done by FreeRTOS tasks
  // Add a small delay and print memory info to monitor system health
  vTaskDelay(pdMS_TO_TICKS(5000));
  Serial.println("--- System Health Check ---");
  printMemoryInfo();
  Serial.println("-------------------------");
}

// ===========================
// Initialization Functions
// ===========================
void printMemoryInfo() {
  Serial.println("=== Memory Status ===");
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Min Free Heap: %d bytes\n", ESP.getMinFreeHeap());
  Serial.printf("PSRAM Total: %d bytes\n", ESP.getPsramSize());
  Serial.printf("PSRAM Free: %d bytes\n", ESP.getFreePsram());
  Serial.printf("Flash Size: %d bytes\n", ESP.getFlashChipSize());
  Serial.println("===================");
}

bool initializeCameraConservative() {
  Serial.println("Attempting conservative camera initialization...");
  
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
  
  // Ultra-conservative settings for minimal memory usage
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.frame_size = FRAMESIZE_QQVGA; // Very small: 160x120
  config.jpeg_quality = 20; // Lower quality
  config.fb_count = 1; // Single buffer only
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  
  Serial.println("Conservative settings: QQVGA (160x120), Quality: 20, Single buffer");

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Conservative camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Configure camera sensor with minimal settings
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Failed to get camera sensor in conservative mode");
    return false;
  }
  
  // Set minimal frame size
  s->set_framesize(s, FRAMESIZE_QQVGA);
  s->set_quality(s, 20);
  
  // Test capture in conservative mode
  Serial.println("Testing conservative camera capture...");
  camera_fb_t * test_fb = esp_camera_fb_get();
  if (test_fb) {
    Serial.printf("Conservative test capture successful: %d bytes\n", test_fb->len);
    esp_camera_fb_return(test_fb);
    cameraInitialized = true;  // Mark camera as working
    Serial.println("Conservative camera initialization successful");
    return true;
  } else {
    Serial.println("Conservative test capture failed");
    cameraInitialized = false;
    return false;
  }
}

bool initializeCamera() {
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
  
  // Check for PSRAM and configure accordingly
  if (psramFound()) {
    // PSRAM available - use higher quality settings
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.frame_size = FRAMESIZE_UXGA; // Higher resolution possible with PSRAM
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
    Serial.println("PSRAM found - using high quality settings");
  } else {
    // No PSRAM - use conservative settings for DRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.frame_size = FRAMESIZE_CIF; // Much smaller frame size for DRAM
    config.jpeg_quality = 15; // Lower quality to reduce memory usage
    config.fb_count = 1; // Single buffer to save memory
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    Serial.println("No PSRAM - using DRAM-optimized settings");
    Serial.println("Frame size: CIF (400x296), Quality: 15, Single buffer");
  }

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  // Configure camera sensor with better error handling
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Failed to get camera sensor");
    return false;
  }
  
  // Apply sensor settings with error checking
  if (s->set_framesize(s, FRAMESIZE_CIF) != 0) {
    Serial.println("Failed to set frame size, trying QVGA");
    if (s->set_framesize(s, FRAMESIZE_QVGA) != 0) {
      Serial.println("Failed to set QVGA, trying QQVGA");
      s->set_framesize(s, FRAMESIZE_QQVGA);
    }
  }
  
  // Additional sensor optimizations for better stability
  s->set_quality(s, 15);        // Set quality
  s->set_brightness(s, 0);      // -2 to 2
  s->set_contrast(s, 0);        // -2 to 2
  s->set_saturation(s, 0);      // -2 to 2
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_colorbar(s, 0);        // 0 = disable, 1 = enable
  s->set_whitebal(s, 1);        // 0 = disable, 1 = enable
  s->set_gain_ctrl(s, 1);       // 0 = disable, 1 = enable
  s->set_exposure_ctrl(s, 1);   // 0 = disable, 1 = enable
  s->set_hmirror(s, 0);         // 0 = disable, 1 = enable
  s->set_vflip(s, 0);           // 0 = disable, 1 = enable
  
  Serial.println("Camera initialized and configured successfully");
  
  // Test capture to verify camera is working
  Serial.println("Testing camera capture...");
  camera_fb_t * test_fb = esp_camera_fb_get();
  if (test_fb) {
    Serial.printf("Test capture successful: %d bytes\n", test_fb->len);
    esp_camera_fb_return(test_fb);
    cameraInitialized = true;  // Mark camera as working
  } else {
    Serial.println("Test capture failed - camera may not be working properly");
    cameraInitialized = false;
    return false;
  }
  
  return true;
}

void setupGPIO() {
  // Setup LED flash
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);

  // Setup GPIO3 as input trigger with pull-down
  pinMode(TRIGGER_GPIO_PIN, INPUT_PULLDOWN);
  
  // Check if ISR service is already installed (ignore error if already installed)
  esp_err_t isr_result = gpio_install_isr_service(0);
  if (isr_result == ESP_OK) {
    Serial.println("GPIO ISR service installed");
  } else if (isr_result == ESP_ERR_INVALID_STATE) {
    Serial.println("GPIO ISR service already installed - continuing");
  } else {
    Serial.printf("GPIO ISR service installation failed: 0x%x\n", isr_result);
  }
  
  // Attach interrupt - THIS IS NOW DONE IN gpioTask AFTER WIFI CONNECTS
  // attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
  
  Serial.println("GPIO configured. Interrupt will be attached by GPIO task after WiFi connects.");
}

void createFreeRTOSObjects() {
  Serial.println("Creating FreeRTOS objects...");
  
  // Create queue for capture requests
  captureQueue = xQueueCreate(10, sizeof(CaptureMessage_t));
  if (!captureQueue) {
    Serial.println("CRITICAL: Failed to create capture queue!");
    return;
  }
  Serial.println("Capture queue created successfully");
  
  // Create mutex for image buffer protection
  imageBufferMutex = xSemaphoreCreateMutex();
  if (!imageBufferMutex) {
    Serial.println("CRITICAL: Failed to create image buffer mutex!");
    return;
  }
  Serial.println("Image buffer mutex created successfully");
  
  // Create event group for WiFi connection signaling
  wifiEventGroup = xEventGroupCreate();
  if (!wifiEventGroup) {
    Serial.println("CRITICAL: Failed to create WiFi event group!");
    return;
  }
  Serial.println("WiFi event group created successfully");
  
  Serial.println("All FreeRTOS objects created successfully");
}

void createTasks() {
  // Create WiFi task
  xTaskCreatePinnedToCore(
    wifiTask,
    "WiFi_Task",
    WIFI_TASK_STACK_SIZE,
    NULL,
    WIFI_TASK_PRIORITY,
    &wifiTaskHandle,
    1  // Run on core 1
  );
  
  // Create camera task
  xTaskCreatePinnedToCore(
    cameraTask,
    "Camera_Task",
    CAMERA_TASK_STACK_SIZE,
    NULL,
    CAMERA_TASK_PRIORITY,
    &cameraTaskHandle,
    0  // Run on core 0 (same as camera driver)
  );
  
  // Create web server task
  xTaskCreatePinnedToCore(
    webServerTask,
    "WebServer_Task",
    WEB_SERVER_TASK_STACK_SIZE,
    NULL,
    WEB_SERVER_TASK_PRIORITY,
    &webServerTaskHandle,
    1  // Run on core 1
  );
  
  // Create GPIO task
  xTaskCreatePinnedToCore(
    gpioTask,
    "GPIO_Task",
    GPIO_TASK_STACK_SIZE,
    NULL,
    GPIO_TASK_PRIORITY,
    &gpioTaskHandle,
    0  // Run on core 0
  );

  // Verify task creation
  if (wifiTaskHandle == NULL || cameraTaskHandle == NULL || webServerTaskHandle == NULL || gpioTaskHandle == NULL) {
    Serial.println("CRITICAL: Failed to create one or more tasks! System will likely fail.");
  } else {
    Serial.println("All tasks created successfully.");
  }
}

// ===========================
// Core Functions
// ===========================
camera_fb_t* captureImage() {
  // Check if camera was successfully initialized
  if (!cameraInitialized) {
    Serial.println("Camera not initialized - skipping capture");
    return nullptr;
  }
  
  // Check if camera sensor is available
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("Camera sensor not available for capture");
    cameraInitialized = false;  // Mark as failed
    return nullptr;
  }
  
  // Turn on flash LED
  digitalWrite(LED_GPIO_NUM, HIGH);
  vTaskDelay(pdMS_TO_TICKS(100)); // Brief flash
  
  // Capture image with retry mechanism
  camera_fb_t * fb = nullptr;
  int retry_count = 3;
  
  for (int i = 0; i < retry_count; i++) {
    fb = esp_camera_fb_get();
    if (fb != nullptr) {
      break; // Success
    }
    
    Serial.printf("Camera capture attempt %d failed, retrying...\n", i + 1);
    vTaskDelay(pdMS_TO_TICKS(100)); // Small delay before retry
  }
  
  // Turn off flash LED
  digitalWrite(LED_GPIO_NUM, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed after retries");
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
    // Try to get sensor status for debugging
    if (s) {
      Serial.printf("Sensor status - framesize: %d, quality: %d\n", 
                   s->status.framesize, s->status.quality);
    }
    return nullptr;
  }
  
  // Validate captured image
  if (fb->len == 0) {
    Serial.println("Captured image has zero length");
    esp_camera_fb_return(fb);
    return nullptr;
  }
  
  Serial.printf("Image captured successfully: %d bytes\n", fb->len);
  return fb;
}

// ===========================
// Web Server Handlers
// ===========================
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<title>ESP32-CAM FreeRTOS Server</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f0f0; margin: 0; padding: 20px; }";
  html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; margin-bottom: 30px; }";
  html += ".capture-btn { background-color: #007bff; color: white; border: none; padding: 15px 30px; font-size: 18px; border-radius: 5px; cursor: pointer; margin: 20px; }";
  html += ".capture-btn:hover { background-color: #0056b3; }";
  html += ".capture-btn:disabled { background-color: #6c757d; cursor: not-allowed; }";
  html += ".image-container { margin-top: 30px; border: 2px dashed #ccc; padding: 20px; min-height: 200px; }";
  html += ".status { margin: 15px 0; font-weight: bold; }";
  html += "img { max-width: 100%; height: auto; border: 1px solid #ddd; }";
  html += ".freertos-info { background-color: #e8f5e8; padding: 10px; border-radius: 5px; margin: 20px 0; }";
  
  // Add camera status styling
  if (cameraInitialized) {
    html += ".camera-status { background-color: #d4edda; color: #155724; padding: 10px; border-radius: 5px; margin: 20px 0; }";
  } else {
    html += ".camera-status { background-color: #f8d7da; color: #721c24; padding: 10px; border-radius: 5px; margin: 20px 0; }";
  }
  
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>📷 ESP32-CAM FreeRTOS Server</h1>";
  
  html += "<div class='freertos-info'>";
  html += "<p><strong>🚀 FreeRTOS Enhanced Version</strong><br>";
  html += "Multi-tasking • Better Performance • Thread-Safe</p>";
  html += "</div>";
  
  // Camera status indicator
  html += "<div class='camera-status'>";
  if (cameraInitialized) {
    html += "<p><strong>📷 Camera Status: ONLINE</strong><br>Camera is working and ready to capture images</p>";
  } else {
    html += "<p><strong>❌ Camera Status: OFFLINE</strong><br>Camera initialization failed - web server only mode</p>";
  }
  html += "</div>";
  
  html += "<p>System Status: WiFi Connected • FreeRTOS Running • Web Server Active</p>";
  
  // Only show camera buttons if camera is working
  if (cameraInitialized) {
    html += "<button class='capture-btn' onclick='captureImage()'>📸 Capture Image</button>";
    html += "<button class='capture-btn' onclick='viewLastGPIO()' style='background-color: #28a745;'>🔄 View Last GPIO Image</button>";
  } else {
    html += "<button class='capture-btn' disabled>📸 Camera Offline</button>";
    html += "<button class='capture-btn' disabled>🔄 Camera Offline</button>";
  }
  
  html += "<button class='capture-btn' onclick='getStatus()' style='background-color: #17a2b8;'>📊 System Status</button>";
  html += "<button class='capture-btn' onclick='testCamera()' style='background-color: #ffc107; color: #000;'>🔧 Camera Test</button>";
  html += "<div class='status' id='status'>System Ready</div>";
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
  
  html += "function getStatus() {";
  html += "  document.getElementById('status').innerText = 'Getting system status...';";
  html += "  document.getElementById('status').style.color = 'blue';";
  html += "  fetch('/status')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('imageContainer').innerHTML = '<pre style=\"text-align: left; font-size: 12px;\">' + data + '</pre>';";
  html += "      document.getElementById('status').innerText = 'System status loaded!';";
  html += "      document.getElementById('status').style.color = 'green';";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerText = 'Error getting status';";
  html += "      document.getElementById('status').style.color = 'red';";
  html += "    });";
  html += "}";
  
  html += "function testCamera() {";
  html += "  document.getElementById('status').innerText = 'Testing camera...';";
  html += "  document.getElementById('status').style.color = 'orange';";
  html += "  fetch('/camera-test')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      document.getElementById('imageContainer').innerHTML = '<pre style=\"text-align: left; font-size: 12px;\">' + data + '</pre>';";
  html += "      document.getElementById('status').innerText = 'Camera test completed!';";
  html += "      document.getElementById('status').style.color = 'green';";
  html += "    })";
  html += "    .catch(error => {";
  html += "      document.getElementById('status').innerText = 'Error testing camera';";
  html += "      document.getElementById('status').style.color = 'red';";
  html += "    });";
  html += "}";
  html += "</script>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleCapture() {
  // Check if camera is available
  if (!cameraInitialized) {
    server.send(503, "text/plain", "Camera not available - initialization failed");
    return;
  }
  
  // Send capture request to camera task
  CaptureMessage_t msg = {CAPTURE_FROM_WEB, (uint32_t)millis()};
  
  if (xQueueSend(captureQueue, &msg, pdMS_TO_TICKS(100)) != pdTRUE) {
    server.send(500, "text/plain", "Camera busy, try again");
    return;
  }
  
  // Wait a moment for capture to complete
  vTaskDelay(pdMS_TO_TICKS(1000));
  
  // Take mutex and send image
  if (xSemaphoreTake(imageBufferMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    if (lastCapturedImage.frameBuffer && !lastCapturedImage.isFromGPIO) {
      server.sendHeader("Content-Type", "image/jpeg");
      server.sendHeader("Content-Length", String(lastCapturedImage.frameBuffer->len));
      server.sendHeader("Cache-Control", "no-cache");
      
      WiFiClient client = server.client();
      client.write(lastCapturedImage.frameBuffer->buf, lastCapturedImage.frameBuffer->len);
      
      Serial.println("Image sent to web client");
    } else {
      server.send(500, "text/plain", "No recent web capture available - camera may have failed");
    }
    xSemaphoreGive(imageBufferMutex);
  } else {
    server.send(500, "text/plain", "Camera system busy");
  }
}

void handleLastImage() {
  if (xSemaphoreTake(imageBufferMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
    if (lastCapturedImage.frameBuffer && lastCapturedImage.isFromGPIO) {
      server.sendHeader("Content-Type", "image/jpeg");
      server.sendHeader("Content-Length", String(lastCapturedImage.frameBuffer->len));
      server.sendHeader("Cache-Control", "no-cache");
      
      WiFiClient client = server.client();
      client.write(lastCapturedImage.frameBuffer->buf, lastCapturedImage.frameBuffer->len);
      
      Serial.println("Last GPIO image sent to web client");
    } else {
      server.send(404, "text/plain", "No GPIO-captured image available");
    }
    xSemaphoreGive(imageBufferMutex);
  } else {
    server.send(500, "text/plain", "System busy");
  }
}

void handleStatus() {
  String status = "=== ESP32-CAM FreeRTOS System Status ===\n\n";
  
  status += "Memory Info:\n";
  status += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
  status += "Min Free Heap: " + String(ESP.getMinFreeHeap()) + " bytes\n";
  status += "PSRAM Total: " + String(ESP.getPsramSize()) + " bytes\n";
  status += "PSRAM Free: " + String(ESP.getFreePsram()) + " bytes\n\n";
  
  status += "WiFi Status:\n";
  status += "Connected: " + String(WiFi.status() == WL_CONNECTED ? "Yes" : "No") + "\n";
  status += "IP Address: " + WiFi.localIP().toString() + "\n";
  status += "RSSI: " + String(WiFi.RSSI()) + " dBm\n\n";
  
  status += "FreeRTOS Tasks:\n";
  status += "Camera Task: " + String(cameraTaskHandle ? "Running" : "Stopped") + " (Stack High Water Mark: " + String(uxTaskGetStackHighWaterMark(cameraTaskHandle)) + " bytes)\n";
  status += "Web Server Task: " + String(webServerTaskHandle ? "Running" : "Stopped") + " (Stack High Water Mark: " + String(uxTaskGetStackHighWaterMark(webServerTaskHandle)) + " bytes)\n";
  status += "WiFi Task: " + String(wifiTaskHandle ? "Running" : "Stopped") + " (Stack High Water Mark: " + String(uxTaskGetStackHighWaterMark(wifiTaskHandle)) + " bytes)\n";
  status += "GPIO Task: " + String(gpioTaskHandle ? "Running" : "Stopped") + " (Stack High Water Mark: " + String(uxTaskGetStackHighWaterMark(gpioTaskHandle)) + " bytes)\n\n";
  
  status += "Camera Status:\n";
  sensor_t * s = esp_camera_sensor_get();
  if (s) {
    status += "Sensor initialized: Yes\n";
    status += "Current framesize: " + String(s->status.framesize) + "\n";
  } else {
    status += "Sensor initialized: No\n";
  }
  
  if (lastCapturedImage.frameBuffer) {
    status += "Last image size: " + String(lastCapturedImage.frameBuffer->len) + " bytes\n";
    status += "Last image source: " + String(lastCapturedImage.isFromGPIO ? "GPIO" : "Web") + "\n";
    status += "Last capture time: " + String(lastCapturedImage.timestamp) + " ms\n";
  } else {
    status += "No images captured yet\n";
  }
  
  server.send(200, "text/plain", status);
}

void handleCameraTest() {
  String result = "=== Camera Test Results ===\n\n";
  
  result += "Camera Initialization Status: " + String(cameraInitialized ? "✅ SUCCESS" : "❌ FAILED") + "\n\n";
  
  // Test camera sensor availability
  sensor_t * s = esp_camera_sensor_get();
  if (s == NULL) {
    result += "❌ Camera sensor: NOT AVAILABLE\n";
    result += "Camera driver may not be properly initialized\n";
    result += "Possible causes:\n";
    result += "- Camera module not connected\n";
    result += "- Insufficient memory for camera driver\n";
    result += "- Hardware failure\n";
  } else {
    result += "✅ Camera sensor: AVAILABLE\n";
    result += "Current framesize: " + String(s->status.framesize) + "\n";
    result += "Current quality: " + String(s->status.quality) + "\n";
  }
  
  result += "\nMemory before test:\n";
  result += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
  
  // Test capture only if sensor is available
  result += "\n=== Capture Test ===\n";
  if (s != NULL && cameraInitialized) {
    camera_fb_t * test_fb = esp_camera_fb_get();
    if (test_fb) {
      result += "✅ Test capture: SUCCESS\n";
      result += "Image size: " + String(test_fb->len) + " bytes\n";
      result += "Width: " + String(test_fb->width) + "\n";
      result += "Height: " + String(test_fb->height) + "\n";
      result += "Format: " + String(test_fb->format) + "\n";
      esp_camera_fb_return(test_fb);
    } else {
      result += "❌ Test capture: FAILED\n";
      result += "Camera sensor available but capture fails\n";
      result += "Possible causes:\n";
      result += "- Insufficient memory for frame buffer\n";
      result += "- Camera sensor malfunction\n";
      result += "- Power supply issues\n";
    }
  } else {
    result += "⚠️ Test capture: SKIPPED\n";
    result += "Camera sensor not available for testing\n";
  }
  
  result += "\nMemory after test:\n";
  result += "Free Heap: " + String(ESP.getFreeHeap()) + " bytes\n";
  
  result += "\n=== Recommendations ===\n";
  if (!cameraInitialized) {
    result += "1. Check camera module connection\n";
    result += "2. Verify power supply (5V, 2A+)\n";
    result += "3. Try different camera module\n";
    result += "4. Check for hardware conflicts\n";
  } else {
    result += "Camera appears to be working properly!\n";
  }
  
  server.send(200, "text/plain", result);
}
