#ifndef TASK_BLE_H
#define TASK_BLE_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// Use Arduino-ESP32 BLE Server (compatible with NimBLE and Bluedroid)
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BATTERY_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define THROTTLE_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STEERING_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// BLE Server globals
extern BLEServer *pServer;
extern BLECharacteristic *pBatteryCharacteristic;
extern BLECharacteristic *pThrottleCharacteristic;
extern BLECharacteristic *pSteeringCharacteristic;

// Function declarations
void initBLE();
void taskBLE(void* pv);

#endif // TASK_BLE_H
