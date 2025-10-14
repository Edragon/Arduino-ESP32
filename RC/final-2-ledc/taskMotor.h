#ifndef TASK_MOTOR_H
#define TASK_MOTOR_H

#include <Arduino.h>
#include <AlfredoCRSF.h>
#include <Adafruit_NeoPixel.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "PinsConfig.h"
#include "ControlData.h"
#include "SharedGlobals.h"

// External references to motor control variables
extern int pwmLeftIn1;
extern int pwmLeftIn2;
extern int pwmRightIn1;
extern int pwmRightIn2;

// Function declarations
void initMotors();
void initLEDs();
void taskMotor(void* pv);
void controlMotorsFromValues(uint16_t roll, uint16_t throttle);
void controlRelays(const ControlData& cmd);

#endif // TASK_MOTOR_H
