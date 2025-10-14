#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>
#include <stdint.h>
#ifdef ARDUINO_ARCH_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#endif

// Include shared data structures and pin configuration
#include "PinsConfig.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// Include separate task files
#include "taskLogger.h"  // Include logger first
#include "taskELRS.h"
#include "taskMQTT.h"
#include "taskMotor.h"
#include "taskServo.h"
#include "taskADC.h"
// #include "taskBLE.h"  // Uncomment to enable BLE functionality

void setup()
{
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor
  
  // IMMEDIATELY pull down IO20 after boot
  pinMode(20, OUTPUT);
  digitalWrite(20, LOW);
  Serial.println("IO20 forced LOW immediately after boot");
  
  // Initialize logger first
  initLogger();
  
  LOG_INFO_MAIN("=== RC Rover Control Starting ===");
  LOG_INFO_MAIN("COM Serial initialized");
  LOG_INFO_MAIN("IO20 pulled LOW at startup");
  
  // Initialize CRSF
  initCRSF();
  LOG_INFO_MAIN("CRSF initialized");

  // Initialize servos FIRST (before motors) to claim LEDC timer/channels
  // This prevents analogWrite() in motors from conflicting with servo LEDC
  initServos();
  LOG_INFO_MAIN("Servos initialized");

  // Initialize motors and relays (uses analogWrite which auto-allocates LEDC)
  initMotors();
  LOG_INFO_MAIN("Motors initialized");

  // Initialize LEDs
  initLEDs();
  LOG_INFO_MAIN("LEDs initialized");

  // Initialize ADC
  initADC();
  LOG_INFO_MAIN("ADC initialized");

  // BLE init (uncomment to enable BLE functionality)
  // initBLE();
  // LOG_INFO_MAIN("BLE initialized");

  // RTOS primitives
  controlMutex = xSemaphoreCreateMutex();
  motorCmdQueue = xQueueCreate(1, sizeof(ControlData)); // latest-sample queue
  LOG_INFO_MAIN("RTOS mutex and queue created");

  // Create logger task first (highest priority for logging)
  xTaskCreatePinnedToCore(taskLogger, "Logger", 3072, nullptr, 6, nullptr, 0);
  
  // Create other tasks (stack sizes tuned conservatively)
  // Core assignment: ELRS on core 1 (high priority for radio), Servo on core 0, Motor on core 1
  xTaskCreatePinnedToCore(taskELRS,  "ELRS",  4096, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(taskServo, "Servo", 2048, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(taskMotor,  "Motor", 4096, nullptr, 3, nullptr, 0);
  xTaskCreatePinnedToCore(taskADC,    "ADC",   3072, nullptr, 1, nullptr, 0);
  // xTaskCreatePinnedToCore(taskBLE,    "BLE",   4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(taskMQTT, "MQTT", 4096, nullptr, 1, nullptr, 0); // MQTT task on core 0
  
  LOG_INFO_MAIN("All tasks created");
}

void loop()
{
  // All work is done in tasks; keep loop idle to let IDLE tasks run (for WDT/housekeeping)
  vTaskDelay(pdMS_TO_TICKS(1000));
  // LOG_DEBUG_MAIN("Main loop heartbeat");
  LOG_DEBUG_MAIN("   ");
}