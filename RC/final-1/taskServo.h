#ifndef TASK_SERVO_H
#define TASK_SERVO_H

#include <Arduino.h>


#include <ESP32Servo.h>
// #include <Servo.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PinsConfig.h"

// Servo objects
extern Servo servoUP_;
extern Servo servoDN_;

// Servo pulse width ranges (microseconds)
extern int servoUP_MinUs;
extern int servoUP_MaxUs;
extern int servoDN_MinUs;
extern int servoDN_MaxUs;

// Servo control variables
extern volatile uint16_t servoCh1; // CRSF channel 2 for servo 1
extern volatile uint16_t servoCh2; // CRSF channel 4 for servo 2

// Function declarations
void initServos();
void taskServo(void* pv);

#endif // TASK_SERVO_H
