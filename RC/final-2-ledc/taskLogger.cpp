#include "taskLogger.h"
#include <stdarg.h>

// Global logger queue and mutex
QueueHandle_t loggerQueue = nullptr;
SemaphoreHandle_t loggerMutex = nullptr;

// Log level strings
const char* logLevelStrings[] = {
  "DEBUG",
  "INFO ",
  "WARN ",
  "ERROR"
};

// ANSI color codes for different log levels
const char* logLevelColors[] = {
  "\033[36m", // Cyan for DEBUG
  "\033[32m", // Green for INFO
  "\033[33m", // Yellow for WARN
  "\033[31m"  // Red for ERROR
};
const char* colorReset = "\033[0m";

// Initialize logger
void initLogger(void) {
  loggerQueue = xQueueCreate(20, sizeof(LogMessage_t)); // Queue for 20 messages
  loggerMutex = xSemaphoreCreateMutex();
  
  if (loggerQueue == nullptr || loggerMutex == nullptr) {
    Serial.println("ERROR: Failed to create logger queue or mutex");
  } else {
    Serial.println("Logger initialized successfully");
  }
}

// Send log message to queue
void logMessage(const char* taskName, LogLevel_t level, const char* format, ...) {
  if (loggerQueue == nullptr) return;
  
  LogMessage_t logMsg;
  
  // Copy task name (truncate if too long)
  strncpy(logMsg.taskName, taskName, sizeof(logMsg.taskName) - 1);
  logMsg.taskName[sizeof(logMsg.taskName) - 1] = '\0';
  
  logMsg.level = level;
  logMsg.timestamp = millis();
  
  // Format the message
  va_list args;
  va_start(args, format);
  vsnprintf(logMsg.message, sizeof(logMsg.message), format, args);
  va_end(args);
  
  // Send to queue (non-blocking to avoid task delays)
  if (xQueueSend(loggerQueue, &logMsg, 0) != pdTRUE) {
    // Queue full - could increment a dropped message counter here
  }
}

// Logger task implementation
void taskLogger(void* pv) {
  (void)pv;
  
  LogMessage_t logMsg;
  
  // Print startup banner
  Serial.println("\n========================================");
  Serial.println("         CENTRALIZED LOGGER STARTED    ");
  Serial.println("========================================");
  Serial.println("Format: [TIME] [LEVEL] [TASK] Message");
  Serial.println("========================================");
  
  for (;;) {
    // Wait for log messages
    if (xQueueReceive(loggerQueue, &logMsg, pdMS_TO_TICKS(100)) == pdTRUE) {
      
      // Take mutex to ensure atomic printing
      if (xSemaphoreTake(loggerMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        
        // Print timestamp
        Serial.printf("[%08lu] ", logMsg.timestamp);
        
        // Print colored log level
        Serial.print(logLevelColors[logMsg.level]);
        Serial.printf("[%s]", logLevelStrings[logMsg.level]);
        Serial.print(colorReset);
        
        // Print task name with fixed width
        Serial.printf(" [%-6s] ", logMsg.taskName);
        
        // Print message
        Serial.println(logMsg.message);
        
        xSemaphoreGive(loggerMutex);
      }
    }
    
    // Small delay to prevent task from consuming too much CPU
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}