#ifndef TASK_LOGGER_H
#define TASK_LOGGER_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

// Log levels
typedef enum {
  LOG_DEBUG = 0,
  LOG_INFO = 1,
  LOG_WARN = 2,
  LOG_ERROR = 3
} LogLevel_t;

// Log message structure
typedef struct {
  char taskName[16];
  LogLevel_t level;
  char message[256];
  uint32_t timestamp;
} LogMessage_t;

// Global logger queue and functions
extern QueueHandle_t loggerQueue;
extern SemaphoreHandle_t loggerMutex;

// Function declarations
void taskLogger(void* pv);
void logMessage(const char* taskName, LogLevel_t level, const char* format, ...);
void initLogger(void);

// Convenience macros for each task
#define LOG_DEBUG_ELRS(msg, ...) logMessage("ELRS", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_ELRS(msg, ...) logMessage("ELRS", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_ELRS(msg, ...) logMessage("ELRS", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_ELRS(msg, ...) logMessage("ELRS", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_MQTT(msg, ...) logMessage("MQTT", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_MQTT(msg, ...) logMessage("MQTT", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_MQTT(msg, ...) logMessage("MQTT", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_MQTT(msg, ...) logMessage("MQTT", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_MOTOR(msg, ...) logMessage("MOTOR", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_MOTOR(msg, ...) logMessage("MOTOR", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_MOTOR(msg, ...) logMessage("MOTOR", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_MOTOR(msg, ...) logMessage("MOTOR", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_SERVO(msg, ...) logMessage("SERVO", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_SERVO(msg, ...) logMessage("SERVO", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_SERVO(msg, ...) logMessage("SERVO", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_SERVO(msg, ...) logMessage("SERVO", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_ADC(msg, ...) logMessage("ADC", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_ADC(msg, ...) logMessage("ADC", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_ADC(msg, ...) logMessage("ADC", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_ADC(msg, ...) logMessage("ADC", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_BLE(msg, ...) logMessage("BLE", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_BLE(msg, ...) logMessage("BLE", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_BLE(msg, ...) logMessage("BLE", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_BLE(msg, ...) logMessage("BLE", LOG_ERROR, msg, ##__VA_ARGS__)

#define LOG_DEBUG_MAIN(msg, ...) logMessage("MAIN", LOG_DEBUG, msg, ##__VA_ARGS__)
#define LOG_INFO_MAIN(msg, ...) logMessage("MAIN", LOG_INFO, msg, ##__VA_ARGS__)
#define LOG_WARN_MAIN(msg, ...) logMessage("MAIN", LOG_WARN, msg, ##__VA_ARGS__)
#define LOG_ERROR_MAIN(msg, ...) logMessage("MAIN", LOG_ERROR, msg, ##__VA_ARGS__)

#endif // TASK_LOGGER_H