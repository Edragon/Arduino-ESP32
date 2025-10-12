#ifndef TASK_ELRS_H
#define TASK_ELRS_H

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "PinsConfig.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// External references to servo control variables (from taskServo)
extern volatile uint16_t servoCh1;
extern volatile uint16_t servoCh2;

// Function declarations
void initCRSF();
void taskELRS(void* pv);

#endif // TASK_ELRS_H
