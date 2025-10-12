#ifndef SHARED_GLOBALS_H
#define SHARED_GLOBALS_H

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "ControlData.h"
#include "PinsConfig.h"

// Shared control data
extern volatile ControlData control;

// Concurrency primitives
extern SemaphoreHandle_t controlMutex;
extern QueueHandle_t motorCmdQueue;

// Peripherals
extern HardwareSerial crsfSerial;
extern HardwareSerial modemSerial;
extern AlfredoCRSF crsf;
extern Adafruit_NeoPixel ws2812;

#endif // SHARED_GLOBALS_H
