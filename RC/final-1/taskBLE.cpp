#include "taskBLE.h"
#include "taskLogger.h"

// BLE Server globals
BLEServer *pServer = nullptr;
BLECharacteristic *pBatteryCharacteristic = nullptr;
BLECharacteristic *pThrottleCharacteristic = nullptr;
BLECharacteristic *pSteeringCharacteristic = nullptr;

void initBLE() {
  LOG_INFO_BLE("Initializing BLE server...");
  BLEDevice::init("RC-RVR");
  pServer = BLEDevice::createServer();
  
  // Create service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristics
  pBatteryCharacteristic = pService->createCharacteristic(
    BATTERY_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pThrottleCharacteristic = pService->createCharacteristic(
    THROTTLE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pSteeringCharacteristic = pService->createCharacteristic(
    STEERING_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Set initial values
  uint16_t initialValue = 0;
  pBatteryCharacteristic->setValue((uint8_t*)&initialValue, 2);
  pThrottleCharacteristic->setValue((uint8_t*)&initialValue, 2);
  pSteeringCharacteristic->setValue((uint8_t*)&initialValue, 2);
  
  // Start service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  LOG_INFO_BLE("BLE server started - waiting for connections...");
}

void taskBLE(void* pv) {
  (void)pv;
  LOG_INFO_BLE("Task started");
  
  for (;;) {
    ControlData snapshot;
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      snapshot.battery_mv = control.battery_mv;
      snapshot.throttle = control.throttle;
      snapshot.steering = control.steering;
      xSemaphoreGive(controlMutex);
    } else {
      snapshot.battery_mv = 0;
      snapshot.throttle = 0;
      snapshot.steering = 0;
    }

    // Debug output
    LOG_DEBUG_BLE("Battery: %d mV | Throttle: %d | Steering: %d", snapshot.battery_mv, snapshot.throttle, snapshot.steering);

    // Update BLE characteristics if server is created
    if (pBatteryCharacteristic && pThrottleCharacteristic && pSteeringCharacteristic) {
      pBatteryCharacteristic->setValue((uint8_t*)&snapshot.battery_mv, 2);
      pBatteryCharacteristic->notify();
      
      pThrottleCharacteristic->setValue((uint8_t*)&snapshot.throttle, 2);
      pThrottleCharacteristic->notify();
      
      pSteeringCharacteristic->setValue((uint8_t*)&snapshot.steering, 2);
      pSteeringCharacteristic->notify();
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // update every 500ms (2 Hz)
  }
}
