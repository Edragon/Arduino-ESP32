#include "taskServo.h"
#include "taskLogger.h"
#include "SharedGlobals.h"

// Servo objects
Servo servoUP;
Servo servoDN;


// Servo pulse width ranges (microseconds)
// int servoUP_MinUs = 500;
// int servoUP_MaxUs = 2500;
// int servoDN_MinUs = 500;
// int servoDN_MaxUs = 2500;

// Servo position variables
int pos1 = 90; // Servo 1 position (0-180 degrees)
int pos2 = 90; // Servo 2 position (0-180 degrees)

void initServos() {
  // Servo setup using ESP32Servo library
  // ESP32PWM::allocateTimer(0);
  // ESP32PWM::allocateTimer(1);
  
  //LOG_INFO_SERVO("Attaching servo 1 to pin %d | Range: %d-%d µs", servoUP_PIN, servoUP_MinUs, servoUP_MaxUs);
  // Use requested alias to set servo parameters
  // servoUP.setPeriodHertz(50);      // Standard 50Hz servo
  servoUP.attach(servoUP_PIN);
  
  //LOG_INFO_SERVO("Attaching servo 2 to pin %d | Range: %d-%d µs", servoDN_PIN, servoDN_MinUs, servoDN_MaxUs);
  // Use requested alias to set servo parameters
  // servoDN.setPeriodHertz(50);      // Standard 50Hz servo
  servoDN.attach(servoDN_PIN);
  
  // Set servos to center position (90 degrees)
  // LOG_INFO_SERVO("Setting servos to center position (90°)");
  // servoUP.write(90);
  // servoDN.write(90);
  // LOG_INFO_SERVO("Servos initialized with ESP32Servo library");

}





void taskServo(void* pv) {
  (void)pv;
  LOG_INFO_SERVO("Task started");
  
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(20); // 50 Hz servo update rate

  for (;;) {
    ControlData cmd;
    uint16_t ser1 = 1500, ser2 = 1500; // Default values
    
    // Get servo data from motor command queue
    if (xQueuePeek(motorCmdQueue, &cmd, pdMS_TO_TICKS(1)) == pdTRUE) {
      ser1 = cmd.servoCh1;
      ser2 = cmd.servoCh2;
    }
    
    // Convert CRSF channel values to servo degrees (0-180)
    // Always update servo positions when we have valid CRSF data
    if (ser1 != 0) { // Valid CRSF data received
      pos1 = map(ser1, 988, 2012, 0, 180);
      pos1 = constrain(pos1, 0, 180); // Ensure within servo range
      servoUP.write(pos1);
    }
    
    if (ser2 != 0) { // Valid CRSF data received  
      pos2 = map(ser2, 988, 2012, 0, 180);
      pos2 = constrain(pos2, 0, 180); // Ensure within servo range
      servoDN.write(pos2);
    }

    // Debug output every 2 seconds
    static uint32_t lastServoDebug = 0;
    if (millis() - lastServoDebug >= 2000) {
      LOG_DEBUG_SERVO("Raw ser1: %d -> Pos1: %d° | Raw ser2: %d -> Pos2: %d°", ser1, pos1, ser2, pos2);
      LOG_DEBUG_SERVO("Servo updates: ser1=%s, ser2=%s | Queue data: %s", 
                      (ser1 != 0) ? "ACTIVE" : "INACTIVE",
                      (ser2 != 0) ? "ACTIVE" : "INACTIVE",
                      (ser1 != 1500 || ser2 != 1500) ? "RECEIVED" : "DEFAULT");
      lastServoDebug = millis();
    }

    vTaskDelayUntil(&lastWake, period);
  }
}
