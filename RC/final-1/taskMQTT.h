#ifndef TASK_MQTT_H
#define TASK_MQTT_H

#include <Arduino.h>
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "PinsConfig.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// Function declaration
void taskMQTT(void* pv);

#endif // TASK_MQTT_H
