#ifndef TASK_ADC_H
#define TASK_ADC_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "PinsConfig.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// Function declarations
void initADC();
void taskADC(void* pv);

#endif // TASK_ADC_H
