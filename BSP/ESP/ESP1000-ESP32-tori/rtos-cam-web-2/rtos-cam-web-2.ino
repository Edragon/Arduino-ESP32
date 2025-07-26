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
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BME280I2C.h>

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

// I2C configuration
#define I2C_SDA           15
#define I2C_SCL           13
#define SSD1306_ADDRESS   0x3C
#define BME280_ADDR       0x76

// OLED display configuration
#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define OLED_RESET        -1  // Reset pin not used

// Global objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
BME280I2C bme; // Use I2C interface - BME280I2C library

WebServer server(80);
bool captureRequested = false; // Flag for manual capture requests
bool gpioTriggered = false;    // Flag for GPIO3 trigger

// GPIO pin for external trigger
#define TRIGGER_GPIO_PIN 3

// FreeRTOS handles
TaskHandle_t cameraTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;
TaskHandle_t sensorDisplayTaskHandle = NULL;
SemaphoreHandle_t fileMutex = NULL;
SemaphoreHandle_t i2cMutex = NULL;

// Sensor data variables
float temperature = 0.0;
float pressure = 0.0;
float altitude = 0.0;
float humidity = 0.0;
bool sensorDataValid = false;

unsigned long lastCaptureTime = 0;
const long captureInterval = 60000; // 60 seconds

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

// Sensor and Display Task
void sensorDisplayTask(void *pvParameters) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(2000); // Update every 2 seconds
  
  for(;;) {
    // Take I2C mutex to prevent conflicts
    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
      // Try to read sensor data using BME280I2C library
      sensorDataValid = false; // Reset flag
      
      // Check if sensor is available before trying to read
      if (bme.begin(BME280_ADDR) || bme.begin(0x77)) {
        BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
        BME280::PresUnit presUnit(BME280::PresUnit_Pa);
        
        bme.read(pressure, temperature, humidity, tempUnit, presUnit);
        pressure = pressure / 100.0F; // Convert Pa to hPa
        altitude = 44330.0 * (1.0 - pow(pressure / 1013.25, 0.1903)); // Calculate altitude
        sensorDataValid = true;
      }
      
      // Update display
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(0, 0);
      
      // Display title
      display.println("ESP32-CAM Monitor");
      display.println("================");
      
      if (sensorDataValid) {
        display.print("Temp: ");
        display.print(temperature, 1);
        display.println(" C");
        
        display.print("Hum: ");
        display.print(humidity, 1);
        display.println(" %");
        
        display.print("Press: ");
        display.print(pressure, 1);
        display.println(" hPa");
        
        display.print("Alt: ");
        display.print(altitude, 1);
        display.println(" m");
      } else {
        display.println("BME280: Not Found");
        display.println("Check I2C wiring");
      }
      
      display.println();
      display.print("Free Heap: ");
      display.print(ESP.getFreeHeap());
      display.println(" B");
      
      display.print("WiFi: ");
      display.println(WiFi.status() == WL_CONNECTED ? "OK" : "NO");
      
      display.display();
      xSemaphoreGive(i2cMutex);
    }
    
    // Wait for next update cycle
    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void setup() {
  delay(3000); // Longer delay for stability
  Serial.begin(115200);
  Serial.println("ESP32-CAM FreeRTOS Image Capture to LittleFS");
  
  // Print initial memory info
  Serial.printf("Initial free heap: %lu bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM available: %s\n", psramFound() ? "Yes" : "No");
  
  // Reduce CPU frequency for stability during setup
  setCpuFrequencyMhz(160); // Reduce from default 240MHz
  Serial.println("CPU frequency reduced to 160MHz for stability");
  
  // Create file mutex
  fileMutex = xSemaphoreCreateMutex();
  if (fileMutex == NULL) {
    Serial.println("Failed to create file mutex");
    return;
  }
  
  // Create I2C mutex
  i2cMutex = xSemaphoreCreateMutex();
  if (i2cMutex == NULL) {
    Serial.println("Failed to create I2C mutex");
    return;
  }
  
  // Initialize BME280 sensor
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(100000); // 100kHz for compatibility
  Serial.println("I2C initialized");
  
  // Try to detect BME280 sensor (non-blocking)
  if (!bme.begin(BME280_ADDR)) {
    Serial.println("BME280 sensor not found at 0x76, trying 0x77...");
    if (!bme.begin(0x77)) {
      Serial.println("Could not find BME280 sensor, continuing without sensor");
      // Continue without sensor - don't block startup
    } else {
      Serial.println("Found BME280 sensor at address 0x77!");
    }
  } else {
    switch(bme.chipModel()) {
      case BME280::ChipModel_BME280:
        Serial.println("Found BME280 sensor at 0x76! Success.");
        break;
      case BME280::ChipModel_BMP280:
        Serial.println("Found BMP280 sensor at 0x76! No Humidity available.");
        break;
      default:
        Serial.println("Found UNKNOWN sensor! Error!");
    }
  }
  
  // Initialize OLED display after I2C is set up
  if (!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_ADDRESS)) {
    Serial.println("SSD1306 allocation failed or not found");
    // Continue without display
  } else {
    Serial.println("OLED display initialized");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("ESP32-CAM Starting...");
    display.display();
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
  Serial.println("Setup complete. Camera ready for capture.");
  
  // Setup GPIO3 as input with pull-down for external trigger
  pinMode(TRIGGER_GPIO_PIN, INPUT_PULLDOWN);
  delay(100); // Small delay before setting up interrupt
  
  // Check if GPIO ISR service is already installed
  esp_err_t isr_result = gpio_install_isr_service(0);
  if (isr_result == ESP_ERR_INVALID_STATE) {
    Serial.println("GPIO ISR service already installed - continuing");
  } else if (isr_result == ESP_OK) {
    Serial.println("GPIO ISR service installed successfully");
  } else {
    Serial.printf("Failed to install GPIO ISR service: 0x%x\n", isr_result);
  }
  
  attachInterrupt(digitalPinToInterrupt(TRIGGER_GPIO_PIN), gpio3InterruptHandler, RISING);
  Serial.println("GPIO3 interrupt configured for rising edge trigger");
  
  // Ensure sufficient free memory before WiFi operations
  Serial.printf("Free heap before WiFi: %lu bytes\n", ESP.getFreeHeap());
  
  // Connect to WiFi with more conservative approach
  Serial.print("Connecting to WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_8_5dBm); // Reduce WiFi power for stability
  
  // Clear any previous WiFi configuration
  WiFi.disconnect(true);
  delay(1000);
  
  WiFi.begin(ssid, password);
  
  int wifi_retry = 0;
  while (WiFi.status() != WL_CONNECTED && wifi_retry < 15) { // Reduced retries
    delay(1000); // Longer delay between retries
    Serial.print(".");
    wifi_retry++;
    
    // Yield to other tasks and check memory
    yield();
    if (ESP.getFreeHeap() < 30000) {
      Serial.println("\nLow memory detected, aborting WiFi connection");
      break;
    }
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
  
  // Create FreeRTOS tasks with very conservative stack sizes for no PSRAM
  xTaskCreatePinnedToCore(
    cameraTask,           // Task function
    "CameraTask",         // Name of task
    8192,                 // Stack size (bytes) - further reduced for no PSRAM
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
      4096,                 // Stack size (bytes) - further reduced for no PSRAM
      NULL,                 // Parameter to pass
      1,                    // Task priority
      &webServerTaskHandle, // Task handle
      0                     // Core 0
    );
    Serial.println("FreeRTOS tasks created successfully (including web server)");
  } else {
    Serial.println("FreeRTOS camera task created successfully (web server disabled)");
  }
  
  // Create sensor and display task
  xTaskCreatePinnedToCore(
    sensorDisplayTask,          // Task function
    "SensorDisplayTask",        // Name of task
    3072,                       // Stack size (bytes) - reduced further
    NULL,                       // Parameter to pass
    1,                          // Task priority
    &sensorDisplayTaskHandle,   // Task handle
    0                           // Core 0
  );
  Serial.println("Sensor and display task created successfully");
  Serial.printf("Free heap after setup: %lu bytes\n", ESP.getFreeHeap());
}

void loop() {
  // Main loop now just monitors tasks and prints status
  static unsigned long lastStatusPrint = 0;
  
  if (millis() - lastStatusPrint >= 30000) { // Print status every 30 seconds
    Serial.printf("=== System Status ===\n");
    Serial.printf("Free heap: %lu bytes\n", ESP.getFreeHeap());
    Serial.printf("Camera task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(cameraTaskHandle));
    if (webServerTaskHandle != NULL) {
      Serial.printf("Web server task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(webServerTaskHandle));
    }
    if (sensorDisplayTaskHandle != NULL) {
      Serial.printf("Sensor display task stack high water mark: %d\n", uxTaskGetStackHighWaterMark(sensorDisplayTaskHandle));
    }
    Serial.printf("WiFi status: %s\n", WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    if (sensorDataValid) {
      Serial.printf("Temperature: %.1f°C, Pressure: %.1f hPa, Altitude: %.1f m\n", temperature, pressure, altitude);
    }
    lastStatusPrint = millis();
  }
  
  // Small delay to prevent the loop from consuming too much CPU
  vTaskDelay(pdMS_TO_TICKS(1000));
}
