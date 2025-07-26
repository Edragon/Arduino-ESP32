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
#include <ESPAsyncWebServer.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <Wire.h>
#include "SSD1306Wire.h"
#include <BMx280I2C.h>

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

AsyncWebServer server(80);
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
TaskHandle_t wifiTaskHandle = NULL;
SemaphoreHandle_t fileMutex = NULL;

// Cached image list stored in RAM to avoid FS scans during web requests
std::vector<String> imageList;

// WiFi management variables
bool wifiConnected = false;
bool wifiInitialized = false;
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 30000; // Check WiFi every 30 seconds

unsigned long lastCaptureTime = 0;
const long captureInterval = 60000; // 60 seconds

// BMP280 sensor readings (shared between tasks)
float currentTemperature = 0.0;
double currentPressure = 0.0;
bool sensorDataValid = false;
bool bmp280Available = false;

// OLED display availability flag
bool oledAvailable = false;

// GPIO interrupt handler - made more robust
void IRAM_ATTR gpio3InterruptHandler() {
  // Keep interrupt handler as minimal as possible
  gpioTriggered = true;
}

void blinkCaptureLEDs() {
  // Blink both LEDs twice quickly - use vTaskDelay instead of delay in FreeRTOS context
  for (int i = 0; i < 2; i++) {
    digitalWrite(LED1_GPIO_PIN, HIGH);
    digitalWrite(LED2_GPIO_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100)); // LED on for 100ms
    digitalWrite(LED1_GPIO_PIN, LOW);
    digitalWrite(LED2_GPIO_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(100)); // LED off for 100ms
  }
}

void captureAndSaveImage() {
  Serial.println("\nCapturing image...");
  
  // Trigger LED blink to indicate capture
  blinkCaptureLEDs();
  
  // Suspend camera-related tasks before file operations to prevent conflicts
  if (displayTaskHandle != NULL) {
    vTaskSuspend(displayTaskHandle);
  }
  if (sensorTaskHandle != NULL) {
    vTaskSuspend(sensorTaskHandle);
  }
  
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
    
    // Enter critical section for file operations
    taskENTER_CRITICAL();
    File file = LittleFS.open(filename, FILE_WRITE);
    taskEXIT_CRITICAL();
    
    if (!file) {
      Serial.println("Failed to open file for writing");
      esp_camera_fb_return(fb);
      xSemaphoreGive(fileMutex);
      // Resume suspended tasks
      if (displayTaskHandle != NULL) {
        vTaskResume(displayTaskHandle);
      }
      if (sensorTaskHandle != NULL) {
        vTaskResume(sensorTaskHandle);
      }
      return;
    }
    
    // Write image data to file in chunks with better memory management
    size_t written = 0;
    size_t chunk_size = 1024; // Increased chunk size for better performance
    uint8_t* buf_ptr = fb->buf;
    size_t remaining = fb->len;
    
    while (remaining > 0) {
      size_t to_write = (remaining > chunk_size) ? chunk_size : remaining;
      
      // Enter critical section for each write operation
      taskENTER_CRITICAL();
      size_t chunk_written = file.write(buf_ptr, to_write);
      taskEXIT_CRITICAL();
      
      if (chunk_written != to_write) {
        Serial.printf("Write error: expected %d, wrote %d\n", to_write, chunk_written);
        break;
      }
      
      written += chunk_written;
      buf_ptr += chunk_written;
      remaining -= chunk_written;
      
      // Allow other tasks to run and prevent watchdog timeouts
      vTaskDelay(pdMS_TO_TICKS(1));
    }
    
    // Close file in critical section
    taskENTER_CRITICAL();
    file.close();
    taskEXIT_CRITICAL();
    
    // Update cached image list
    imageList.push_back(filename);
    
    if (written == fb->len) {
      Serial.printf("Image saved successfully as %s (%d bytes)\n", filename.c_str(), written);
    } else {
      Serial.printf("Failed to save complete image. Written: %d, Expected: %d\n", written, fb->len);
    }
    
    // Return frame buffer immediately after use
    esp_camera_fb_return(fb);
    xSemaphoreGive(fileMutex);
    
    // Resume suspended tasks
    if (displayTaskHandle != NULL) {
      vTaskResume(displayTaskHandle);
    }
    if (sensorTaskHandle != NULL) {
      vTaskResume(sensorTaskHandle);
    }
  } else {
    Serial.println("Failed to take file mutex, skipping capture");
    // Resume suspended tasks even if mutex failed
    if (displayTaskHandle != NULL) {
      vTaskResume(displayTaskHandle);
    }
    if (sensorTaskHandle != NULL) {
      vTaskResume(sensorTaskHandle);
    }
  }
}

void handleRoot(AsyncWebServerRequest *request) {
  // Take mutex to protect file operations (specifically, the imageList vector)
  if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
    // Build complete HTML response
    String response = "<html><head><title>ESP32-CAM + BMP280 Sensor</title>";
    response += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
    response += "<style>body{font-family:sans-serif; background-color:#222; color: #fff; padding:20px;} ";
    response += "table{border-collapse:collapse; width:100%;} th,td{padding:8px;text-align:left;border-bottom:1px solid #444;} ";
    response += "tr:hover{background-color:#333;} a{text-decoration:none;color:#00abff;} ";
    response += ".button{background-color:#00abff; color:white; padding:10px 20px; text-align:center; display:inline-block; font-size:16px; margin:10px 5px; border:none; cursor:pointer; border-radius:5px;} ";
    response += "img{max-width:200px; border:1px solid #666;}</style>";
    response += "</head><body><h1>ESP32-CAM + BMP280 Sensor (FreeRTOS)</h1>";
    response += "<p><a href='/capture' class='button'>Take New Picture</a> ";
    response += "<a href='/' class='button'>Refresh</a></p>";
    response += "<h2>Captured Images</h2>";
    response += "<table><tr><th>Image</th><th>File</th><th>Size</th><th>Actions</th></tr>";

    // Use cached image list to avoid FS scanning
    int fileCount = imageList.size();
    for (int i = 0; i < fileCount; i++) {
      String fileName = imageList[i];
      response += "<tr>";
      response += "<td><img src='/download?file=" + fileName + "' alt='" + fileName + "'></td>";
      response += "<td>" + fileName + "</td>";
      response += "<td>--- bytes</td>"; // size not cached
      response += "<td><a href='/download?file=" + fileName + "' target='_blank'>View</a></td>";
      response += "</tr>";
    }
    if (fileCount == 0) {
      response += "<tr><td colspan='4'>No images found. Take a picture first!</td></tr>";
    }
    
    response += "</table>";
    
    // Add BMP280 sensor data section
    response += "<h2>Sensor Data (BMP280)</h2>";
    response += "<div style='background-color:#333; padding:15px; border-radius:5px; margin:10px 0;'>";
    if (bmp280Available && sensorDataValid) {
      response += "<p><strong>Temperature:</strong> " + String(currentTemperature, 1) + " °C</p>";
      response += "<p><strong>Pressure:</strong> " + String(currentPressure, 0) + " Pa</p>";
      response += "<p><strong>Pressure:</strong> " + String(currentPressure / 100.0, 2) + " hPa</p>";
    } else if (bmp280Available) {
      response += "<p><strong>Sensor Status:</strong> Reading...</p>";
    } else {
      response += "<p><strong>Sensor Status:</strong> BMP280 Not Available or Failed to Initialize</p>";
    }
    response += "</div>";
    
    // Add system information section
    response += "<h2>System Information</h2>";
    response += "<div style='background-color:#333; padding:15px; border-radius:5px; margin:10px 0;'>";
    
    // Calculate uptime
    unsigned long uptimeMs = millis();
    unsigned long uptimeSeconds = uptimeMs / 1000;
    unsigned long uptimeMinutes = uptimeSeconds / 60;
    unsigned long uptimeHours = uptimeMinutes / 60;
    unsigned long uptimeDays = uptimeHours / 24;
    
    // Format uptime string
    String uptimeStr = "";
    if (uptimeDays > 0) {
      uptimeStr += String(uptimeDays) + "d ";
    }
    uptimeStr += String(uptimeHours % 24) + "h ";
    uptimeStr += String(uptimeMinutes % 60) + "m ";
    uptimeStr += String(uptimeSeconds % 60) + "s";
    
    response += "<p><strong>Uptime:</strong> " + uptimeStr + "</p>";
    response += "<p><strong>Uptime (ms):</strong> " + String(uptimeMs) + "</p>";
    response += "<p><strong>Total images:</strong> " + String(fileCount) + "</p>";
    response += "<p><strong>Free heap:</strong> " + String(ESP.getFreeHeap()) + " bytes</p>";
    response += "<p><strong>OLED Display:</strong> " + String(oledAvailable ? "Available" : "Not Available") + "</p>";
    response += "<p><strong>BMP280 Sensor:</strong> " + String(bmp280Available ? "Available" : "Not Available") + "</p>";
    response += "</div></body></html>";
    
    request->send(200, "text/html", response);
    xSemaphoreGive(fileMutex);
  } else {
    request->send(503, "text/plain", "Service temporarily unavailable - file system busy");
  }
}

void handleFileDownload(AsyncWebServerRequest *request) {
  if (request->hasParam("file")) {
    String path = request->getParam("file")->value();
    // Ensure path starts with /
    if (!path.startsWith("/")) {
      path = "/" + path;
    }
    Serial.println("Attempting to download file: " + path);

    // Take mutex before accessing LittleFS
    if (xSemaphoreTake(fileMutex, pdMS_TO_TICKS(5000)) == pdTRUE) {
      if (LittleFS.exists(path)) {
        Serial.println("File exists. Starting stream with onSent callback.");
        AsyncWebServerResponse *response = request->beginResponse(LittleFS, path, "image/jpeg");
        
        // Use onSent callback to release mutex after file is sent
        response->onSent([path, fileMutex]() {
          xSemaphoreGive(fileMutex);
          Serial.printf("Sent %s, mutex released.\n", path.c_str());
        });
        
        request->send(response);
      } else {
        Serial.println("File not found, releasing mutex.");
        xSemaphoreGive(fileMutex);
        request->send(404, "text/plain", "File not found");
      }
    } else {
      Serial.println("Could not get file mutex for download.");
      request->send(503, "text/plain", "Service temporarily unavailable - file system busy");
    }
  } else {
    request->send(400, "text/plain", "Bad Request: file argument missing");
  }
}

void handleCapture(AsyncWebServerRequest *request) {
  taskENTER_CRITICAL();
  captureRequested = true; // Set flag instead of calling function directly
  taskEXIT_CRITICAL();
  request->redirect("/");
}

// FreeRTOS Task Functions
void displayTask(void *pvParameters) {
  // Only initialize display if it's available
  if (oledAvailable) {
    // Display is already initialized in setup(), just confirm it's working
    Serial.println("OLED display task started - display already initialized");
  } else {
    Serial.println("OLED display not available - display task will skip updates");
  }
  
  for(;;) {
    if (oledAvailable) {
      // Add error handling for display operations
      static int displayErrorCount = 0;
      const int maxDisplayErrors = 3;
      
      try {
        display.clear();
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        
        // Display title
        display.setFont(ArialMT_Plain_10);
        display.drawString(0, 0, "ESP32-CAM + BMP280");
        
        // Display BMP280 sensor data
        if (sensorDataValid && bmp280Available) {
          display.setFont(ArialMT_Plain_10);
          display.drawString(0, 12, "Temp: " + String(currentTemperature, 1) + " C");
          display.drawString(0, 22, "Press: " + String(currentPressure, 0) + " Pa");
        } else if (bmp280Available) {
          display.drawString(0, 12, "Sensor: Reading...");
        } else {
          display.drawString(0, 12, "BMP280: Not Available");
        }
        
        // Display WiFi status (compact)
        if (wifiConnected) {
          display.drawString(0, 34, "WiFi: " + WiFi.localIP().toString());
        } else {
          display.drawString(0, 34, "WiFi: Disconnected");
        }
        
        // Display memory info
        display.drawString(0, 46, "Heap: " + String(ESP.getFreeHeap()));
        
        // Display capture count (if files exist)
        display.drawString(0, 56, "Images captured");
        
        display.display();
        displayErrorCount = 0; // Reset error count on successful update
      } catch (...) {
        displayErrorCount++;
        Serial.println("Display update failed");
        
        // If too many display errors, disable display to prevent I2C spam
        if (displayErrorCount >= maxDisplayErrors) {
          Serial.println("Too many display errors - disabling display to prevent I2C spam");
          oledAvailable = false;
        }
      }
    }
    
    // Update every 2 seconds
    vTaskDelay(pdMS_TO_TICKS(2000));
  }
}

void sensorTask(void *pvParameters) {
  for(;;) {
    // Only try to read sensor if BMP280 is available
    if (bmp280Available) {
      // Add error counter to prevent continuous I2C errors
      static int errorCount = 0;
      const int maxErrors = 5;
      
      // Start a measurement
      if (bmx280.measure()) {
        // Wait for the measurement to finish
        int timeout = 0;
        while (!bmx280.hasValue() && timeout < 50) { // 5 second timeout
          vTaskDelay(pdMS_TO_TICKS(100));
          timeout++;
        }
        
        if (timeout < 50) {
          // Read sensor values
          currentTemperature = bmx280.getTemperature();
          currentPressure = bmx280.getPressure64();
          sensorDataValid = true;
          errorCount = 0; // Reset error count on successful reading
          
          Serial.printf("BMP280 - Temp: %.1f°C, Pressure: %.0f Pa\n", 
                       currentTemperature, currentPressure);
        } else {
          Serial.println("BMP280 measurement timeout");
          sensorDataValid = false;
          errorCount++;
        }
      } else {
        Serial.println("BMP280 measurement failed");
        sensorDataValid = false;
        errorCount++;
      }
      
      // If too many errors, disable sensor to prevent continuous I2C errors
      if (errorCount >= maxErrors) {
        Serial.println("Too many BMP280 errors - disabling sensor to prevent I2C spam");
        bmp280Available = false;
        sensorDataValid = false;
      }
    } else {
      // BMP280 not available, keep sensorDataValid false
      sensorDataValid = false;
    }
    
    // Read sensor every 5 seconds
    vTaskDelay(pdMS_TO_TICKS(5000));
  }
}

void webServerTask(void *pvParameters) {
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  // Wait for WiFi to be initialized
  while (!wifiInitialized) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_task_wdt_reset();
  }
  
  Serial.println("Web server task started");
  
  // Add small delay before setting up routes to ensure system stability
  vTaskDelay(pdMS_TO_TICKS(500));
  
  bool serverStarted = false;
  bool routesConfigured = false;
  
  for(;;) {
    // Reset watchdog for this task
    esp_task_wdt_reset();
    
    // Setup server routes only once when WiFi is connected and routes not yet configured
    if (wifiConnected && !routesConfigured) {
      Serial.println("Configuring web server routes...");
      
      // Use a try-catch block to handle potential cache access errors
      try {
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
          handleRoot(request);
        });
        
        server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
          handleFileDownload(request);
        });
        
        server.on("/capture", HTTP_GET, [](AsyncWebServerRequest *request){
          handleCapture(request);
        });
        
        routesConfigured = true;
        Serial.println("Web server routes configured successfully");
      } catch (...) {
        Serial.println("Error configuring web server routes, will retry...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }
    
    // Only start server if WiFi is connected, routes are configured, and server not yet started
    if (wifiConnected && routesConfigured && !serverStarted) {
      try {
        server.begin();
        Serial.println("HTTP server started");
        serverStarted = true;
      } catch (...) {
        Serial.println("Error starting HTTP server, will retry...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        continue;
      }
    }
    
    // AsyncWebServer handles clients automatically, no need for handleClient()
    
    // If WiFi disconnected, stop server and reset configuration
    if (!wifiConnected && (serverStarted || routesConfigured)) {
      if (serverStarted) {
        server.end();
        Serial.println("HTTP server stopped due to WiFi disconnection");
        serverStarted = false;
      }
      routesConfigured = false; // Reset routes configuration
    }
    
    // Longer delay since AsyncWebServer handles requests automatically
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(captureInterval);
  
  for(;;) {
    // Use local variables to avoid race conditions
    bool localGpioTriggered = false;
    bool localCaptureRequested = false;
    
    // Check for GPIO3 trigger (highest priority) - atomic read
    taskENTER_CRITICAL();
    if (gpioTriggered) {
      localGpioTriggered = true;
      gpioTriggered = false;
    }
    taskEXIT_CRITICAL();
    
    // Check for manual capture request - atomic read
    taskENTER_CRITICAL();
    if (captureRequested) {
      localCaptureRequested = true;
      captureRequested = false;
    }
    taskEXIT_CRITICAL();
    
    if (localGpioTriggered) {
      Serial.println("GPIO3 trigger detected - capturing image");
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    else if (localCaptureRequested) {
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    
    // Auto capture every interval
    if (xTaskGetTickCount() - xLastWakeTime >= xFrequency) {
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount();
    }
    
    // Reset watchdog for this task
    esp_task_wdt_reset();
    
    // Task delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// WiFi Management Task
void wifiTask(void *pvParameters) {
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  Serial.println("WiFi task started");
  
  // Initialize WiFi in task context
  vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for system to stabilize
  
  for(;;) {
    // Reset watchdog for this task
    esp_task_wdt_reset();
    
    if (!wifiInitialized) {
      Serial.println("Initializing WiFi in task...");
      
      // Set WiFi to station mode
      WiFi.mode(WIFI_STA);
      vTaskDelay(pdMS_TO_TICKS(200));
      
      // Configure WiFi with lower power settings for stability
      WiFi.setTxPower(WIFI_POWER_8_5dBm);
      vTaskDelay(pdMS_TO_TICKS(200));
      
      // Set WiFi event handlers for better management
      WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        switch(event) {
          case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("WiFi connected to AP");
            break;
          case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            wifiConnected = true;
            Serial.printf("WiFi got IP: %s\n", WiFi.localIP().toString().c_str());
            break;
          case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            wifiConnected = false;
            Serial.println("WiFi disconnected from AP");
            break;
          default:
            break;
        }
      });
      
      wifiInitialized = true;
      Serial.println("WiFi initialized successfully");
    }
    
    // Check WiFi connection status
    if (wifiInitialized && !wifiConnected && WiFi.status() != WL_CONNECTED) {
      Serial.println("Attempting WiFi connection...");
      WiFi.begin(ssid, password);
      
      // Wait for connection with timeout
      int retries = 0;
      const int maxRetries = 20;
      
      while (WiFi.status() != WL_CONNECTED && retries < maxRetries) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        retries++;
        esp_task_wdt_reset();
        
        if (retries % 5 == 0) {
          Serial.printf("WiFi connection attempt %d, heap: %u\n", retries, ESP.getFreeHeap());
        }
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        wifiConnected = true;
        Serial.printf("WiFi connected! IP: %s, RSSI: %d dBm\n", 
                     WiFi.localIP().toString().c_str(), WiFi.RSSI());
      } else {
        Serial.println("WiFi connection failed, will retry later");
        WiFi.disconnect(true);
        vTaskDelay(pdMS_TO_TICKS(1000));
      }
    }
    
    // Periodic WiFi health check
    if (wifiConnected && WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost, marking as disconnected");
      wifiConnected = false;
    }
    
    // Long delay between WiFi management cycles
    vTaskDelay(pdMS_TO_TICKS(wifiCheckInterval));
  }
}

void cameraTask(void *pvParameters) {
  // Add this task to watchdog
  esp_task_wdt_add(NULL);
  
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(captureInterval);
  
  for(;;) {
    // Use local variables to avoid race conditions
    bool localGpioTriggered = false;
    bool localCaptureRequested = false;
    
    // Check for GPIO3 trigger (highest priority) - atomic read
    taskENTER_CRITICAL();
    if (gpioTriggered) {
      localGpioTriggered = true;
      gpioTriggered = false;
    }
    taskEXIT_CRITICAL();
    
    // Check for manual capture request - atomic read
    taskENTER_CRITICAL();
    if (captureRequested) {
      localCaptureRequested = true;
      captureRequested = false;
    }
    taskEXIT_CRITICAL();
    
    if (localGpioTriggered) {
      Serial.println("GPIO3 trigger detected - capturing image");
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    else if (localCaptureRequested) {
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount(); // Reset timer to avoid immediate auto-capture
    }
    
    // Auto capture every interval
    if (xTaskGetTickCount() - xLastWakeTime >= xFrequency) {
      captureAndSaveImage();
      xLastWakeTime = xTaskGetTickCount();
    }
    
    // Reset watchdog for this task
    esp_task_wdt_reset();
    
    // Task delay to prevent watchdog issues
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
  delay(2000); // Increased delay for stability
  Serial.begin(115200);
  Serial.println("ESP32-CAM FreeRTOS Image Capture to LittleFS");
  
  // Print initial memory info
  Serial.printf("Initial free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM available: %s\n", psramFound() ? "Yes" : "No");
  
  // Enable watchdog timer early to catch hangs
  esp_task_wdt_init(30, true); // 30 second timeout
  esp_task_wdt_add(NULL); // Add current task (setup) to watchdog
  
  // Create file mutex
  fileMutex = xSemaphoreCreateMutex();
  if (fileMutex == NULL) {
    Serial.println("Failed to create file mutex");
    return;
  }
  
  // Initialize I2C for OLED display and BMP280 sensor (SDA=15, SCL=13)
  Wire.begin(15, 13);
  Wire.setClock(100000); // Set I2C clock to 100kHz for better stability
  Serial.println("I2C initialized for OLED display and BMP280 sensor");
  
  // Small delay to let I2C bus stabilize
  delay(100);
  
  // Try to initialize OLED display with better error handling
  Serial.println("Attempting to initialize OLED display...");
  oledAvailable = false;
  
  // First check if OLED device responds on I2C bus
  Wire.beginTransmission(0x3c);
  if (Wire.endTransmission() == 0) {
    Serial.println("OLED device detected on I2C bus");
    try {
      display.init();
      delay(100);
      display.clear();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "ESP32-CAM Starting...");
      display.display();
      delay(100);
      oledAvailable = true;
      Serial.println("OLED display initialized successfully");
    } catch (...) {
      oledAvailable = false;
      Serial.println("OLED display initialization failed - continuing without display");
    }
  } else {
    Serial.println("OLED device not found on I2C bus - skipping OLED initialization");
  }
  
  // Try to initialize BMP280 sensor with better error handling
  Serial.println("Attempting to initialize BMP280 sensor...");
  bmp280Available = false;
  
  // First check if BMP280 device responds on I2C bus
  Wire.beginTransmission(0x76);
  if (Wire.endTransmission() == 0) {
    Serial.println("BMP280 device detected on I2C bus");
    if (bmx280.begin()) {
      bmp280Available = true;
      if (bmx280.isBME280()) {
        Serial.println("BME280 sensor detected and configured successfully");
      } else {
        Serial.println("BMP280 sensor detected and configured successfully");
      }
      
      // Reset sensor to default parameters and configure
      bmx280.resetToDefaults();
      bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
      bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
      sensorDataValid = false; // Will be set to true when first reading is successful
    } else {
      Serial.println("BMP280 sensor initialization failed despite I2C response - continuing without sensor");
      bmp280Available = false;
      sensorDataValid = false;
    }
  } else {
    Serial.println("BMP280 device not found on I2C bus - skipping BMP280 initialization");
    bmp280Available = false;
    sensorDataValid = false;
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
  // Preload image list into RAM
  imageList.clear();
  {
    // Use critical section for directory scanning
    taskENTER_CRITICAL();
    File dir = LittleFS.open("/");
    taskEXIT_CRITICAL();
    
    File f = dir.openNextFile();
    while (f) {
      String name = String(f.name());
      if (name.endsWith(".jpg")) {
        imageList.push_back(name);
      }
      f = dir.openNextFile();
    }
    
    // Close directory in critical section
    taskENTER_CRITICAL();
    dir.close();
    taskEXIT_CRITICAL();
    
    Serial.printf("Preloaded %u images into cache\n", imageList.size());
  }
  
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
  
  // Setup LED pins for capture indication
  pinMode(LED1_GPIO_PIN, OUTPUT);
  pinMode(LED2_GPIO_PIN, OUTPUT);
  digitalWrite(LED1_GPIO_PIN, LOW); // Start with LEDs off
  digitalWrite(LED2_GPIO_PIN, LOW);
  Serial.println("LED pins (GPIO33, GPIO4) configured for capture indication");
  
  Serial.printf("Free heap before task creation: %u bytes\n", ESP.getFreeHeap());
  esp_task_wdt_reset();
  
  // Create FreeRTOS tasks with conservative stack sizes for no PSRAM
  Serial.println("Creating FreeRTOS tasks...");
  esp_task_wdt_reset();
  
  // Create WiFi task first (highest priority for connectivity)
  if (xTaskCreatePinnedToCore(
    wifiTask,             // Task function
    "WiFiTask",           // Name of task
    8192,                 // Stack size (bytes) - larger for WiFi operations
    NULL,                 // Parameter to pass
    3,                    // Task priority (high priority)
    &wifiTaskHandle,      // Task handle
    0                     // Core 0 (same as WiFi radio)
  ) != pdPASS) {
    Serial.println("Failed to create WiFi task");
  } else {
    Serial.println("WiFi task created successfully");
  }
  
  esp_task_wdt_reset();
  
  // Create display task (only if OLED is available, but create task anyway for consistency)
  if (xTaskCreatePinnedToCore(
    displayTask,          // Task function
    "DisplayTask",        // Name of task
    6144,                 // Stack size (bytes) - increased for stability
    NULL,                 // Parameter to pass
    1,                    // Task priority (lower priority)
    &displayTaskHandle,   // Task handle
    0                     // Core 0 (moved from Core 1 to avoid cache conflicts)
  ) != pdPASS) {
    Serial.println("Failed to create display task");
  } else {
    Serial.println("Display task created successfully");
  }
  
  esp_task_wdt_reset();
  
  // Create sensor task (only if BMP280 is available, but create task anyway for consistency)
  if (xTaskCreatePinnedToCore(
    sensorTask,           // Task function
    "SensorTask",         // Name of task
    4096,                 // Stack size (bytes) - increased for stability
    NULL,                 // Parameter to pass
    1,                    // Task priority (same as display)
    &sensorTaskHandle,    // Task handle
    0                     // Core 0 (moved from Core 1 to avoid cache conflicts)
  ) != pdPASS) {
    Serial.println("Failed to create sensor task");
  } else {
    Serial.println("Sensor task created successfully");
  }
  
  esp_task_wdt_reset();
  
  if (xTaskCreatePinnedToCore(
    cameraTask,           // Task function
    "CameraTask",         // Name of task
    12288,                // Stack size (bytes) - increased for stability
    NULL,                 // Parameter to pass
    2,                    // Task priority (higher number = higher priority)
    &cameraTaskHandle,    // Task handle
    0                     // Core 0 (moved from Core 1 to avoid cache conflicts)
  ) != pdPASS) {
    Serial.println("Failed to create camera task");
  } else {
    Serial.println("Camera task created successfully");
  }
  
  esp_task_wdt_reset();
  
  // Create web server task with larger stack size for stability
  if (xTaskCreatePinnedToCore(
    webServerTask,        // Task function
    "WebServerTask",      // Name of task
    12288,                // Stack size (bytes) - increased for web operations and error handling
    NULL,                 // Parameter to pass
    2,                    // Task priority (medium priority)
    &webServerTaskHandle, // Task handle
    1                     // Core 1 (moved to Core 1 for better separation)
  ) != pdPASS) {
    Serial.println("Failed to create web server task");
  } else {
    Serial.println("Web server task created successfully");
  }
  
  Serial.println("All FreeRTOS tasks created successfully");
  Serial.printf("OLED Display: %s\n", oledAvailable ? "Available" : "Not Available");
  Serial.printf("BMP280 Sensor: %s\n", bmp280Available ? "Available" : "Not Available");
  
  esp_task_wdt_reset();
  Serial.printf("Free heap after setup: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("Largest free block: %u bytes\n", ESP.getMaxAllocHeap());
  Serial.println("Setup completed successfully!");
}

void loop() {
  // Main loop now just monitors tasks and prints status
  static unsigned long lastStatusPrint = 0;
  
  // Feed the watchdog to prevent timeouts
  esp_task_wdt_reset();
  
  if (millis() - lastStatusPrint >= 30000) { // Print status every 30 seconds
    Serial.printf("=== System Status ===\n");
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.printf("Largest free block: %u bytes\n", ESP.getMaxAllocHeap());
    Serial.printf("Camera task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(cameraTaskHandle));
    Serial.printf("Display task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(displayTaskHandle));
    Serial.printf("Sensor task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(sensorTaskHandle));
    if (wifiTaskHandle != NULL) {
      Serial.printf("WiFi task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(wifiTaskHandle));
    }
    if (webServerTaskHandle != NULL) {
      Serial.printf("Web server task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(webServerTaskHandle));
    }
    Serial.printf("WiFi status: %s\n", wifiConnected ? "Connected" : "Disconnected");
    Serial.printf("OLED Display: %s\n", oledAvailable ? "Available" : "Not Available");
    Serial.printf("BMP280 Sensor: %s\n", bmp280Available ? "Available" : "Not Available");
    if (sensorDataValid && bmp280Available) {
      Serial.printf("BMP280: %.1f°C, %.0f Pa\n", currentTemperature, currentPressure);
    }
    lastStatusPrint = millis();
  }
  
  // Small delay to prevent the loop from consuming too much CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}