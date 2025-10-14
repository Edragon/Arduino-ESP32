#ifndef TASK_SERVO_H
#define TASK_SERVO_H

#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "PinsConfig.h"

// LEDC configuration constants
// ESP32S3: only LOW_SPEED mode is available
#define SERVO_SPEED_MODE LEDC_LOW_SPEED_MODE
#define SERVO_FREQUENCY 50                          // 50Hz for servo control
#define SERVO_RESOLUTION LEDC_TIMER_14_BIT          // 14-bit resolution
#define SERVO_UP_CHANNEL LEDC_CHANNEL_6
#define SERVO_DN_CHANNEL LEDC_CHANNEL_7
#define SERVO_TIMER LEDC_TIMER_3                    // Prefer timer 3 to avoid analogWrite defaults

// Selected LEDC resources (may differ from preferred if a conflict is detected)
extern ledc_mode_t servoSpeedMode;
extern ledc_timer_t servoTimer;

// Servo pulse width constants (in microseconds)
#define SERVO_MIN_PULSE_US 500
#define SERVO_MAX_PULSE_US 2500
#define SERVO_CENTER_PULSE_US 1500

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
uint32_t calculateDutyCycle(uint16_t pulseWidthUs);
uint16_t angleToPulseWidth(int angle);
void setServoPosition(ledc_channel_t channel, int angle);

#endif // TASK_SERVO_H
