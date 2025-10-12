#include "taskMotor.h"
#include "taskLogger.h"

// Soft start variables (constants defined in PinsConfig.h)
int pwmLeftIn1 = 0, pwmLeftIn2 = 0, pwmRightIn1 = 0, pwmRightIn2 = 0;

void initMotors() {
  // Motor pins setup
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2, OUTPUT);

  // Set all motor pins LOW at startup
  analogWrite(LEFT_MOTOR_IN1, 0);
  analogWrite(LEFT_MOTOR_IN2, 0);
  analogWrite(RIGHT_MOTOR_IN1, 0);
  analogWrite(RIGHT_MOTOR_IN2, 0);

  // Relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);
  
  LOG_INFO_MOTOR("Motors and relays initialized");
}

void initLEDs() {
  // WS2812 LED initialization
  ws2812.begin();
  ws2812.show();
  LOG_INFO_MOTOR("WS2812 LED initialized");
}

void taskMotor(void* pv) {
  (void)pv;
  LOG_INFO_MOTOR("Task started");
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(20); // 50 Hz control loop

  for (;;) {
    ControlData cmd;
    if (xQueueReceive(motorCmdQueue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
      // Process motor commands
      controlMotorsFromValues(cmd.steering, cmd.throttle);
      
      // Control relays using ControlData
      controlRelays(cmd);
      
      // Access servo and relay data for logging
      LOG_DEBUG_MOTOR("Motor: T=%d S=%d | Servos: CH1=%d CH2=%d | Relays: CH5=%d CH6=%d", 
                      cmd.throttle, cmd.steering, cmd.servoCh1, cmd.servoCh2,
                      cmd.relayCh5, cmd.relayCh6);
    }
    
    vTaskDelayUntil(&lastWake, period);
  }
}





void controlMotorsFromValues(uint16_t roll, uint16_t throttle) {
  // Failsafe: if no RC signal (both are zero), stop motors and turn off WS2812
  if (roll == 0 && throttle == 0) {
    pwmLeftIn1 = pwmLeftIn2 = pwmRightIn1 = pwmRightIn2 = 0;
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, 0);
    analogWrite(RIGHT_MOTOR_IN1, 0);
    analogWrite(RIGHT_MOTOR_IN2, 0);
    ws2812.setPixelColor(0, ws2812.Color(0,0,0));
    ws2812.show();
    return;
  }

  // Center values for CRSF are typically 992-1504-2011
  const int center = 1500;
  const int deadband = 200; // Increased deadband for larger no-control zone

  // Determine direction for each motor
  bool leftForward = false, leftBackward = false;
  bool rightForward = false, rightBackward = false;

  // Forward/backward logic
  if (throttle > center + deadband) {
    leftForward = true;
    rightForward = true;
  } else if (throttle < center - deadband) {
    leftBackward = true;
    rightBackward = true;
  }

  // Steering logic (turn overrides if outside deadband)
  if (roll > center + deadband) {
    // turn right: left motor backward, right motor forward
    leftBackward = true;
    rightForward = true;
    leftForward = false;
    rightBackward = false;
  } else if (roll < center - deadband) {
    // turn left: right motor backward, left motor forward
    rightBackward = true;
    leftForward = true;
    leftBackward = false;
    rightForward = false;
  }

  // Determine target PWM for each pin
  int targetLeftIn1  = leftForward  ? SOFTSTART_TARGET : 0;
  int targetLeftIn2  = leftBackward ? SOFTSTART_TARGET : 0;
  int targetRightIn1 = rightForward ? SOFTSTART_TARGET : 0;
  int targetRightIn2 = rightBackward? SOFTSTART_TARGET : 0;

  // Soft start ramping
  if (pwmLeftIn1  < targetLeftIn1)  pwmLeftIn1  = min(pwmLeftIn1  + SOFTSTART_STEP, targetLeftIn1);  else if (pwmLeftIn1  > targetLeftIn1)  pwmLeftIn1  = max(pwmLeftIn1  - SOFTSTART_STEP, targetLeftIn1);
  if (pwmLeftIn2  < targetLeftIn2)  pwmLeftIn2  = min(pwmLeftIn2  + SOFTSTART_STEP, targetLeftIn2);  else if (pwmLeftIn2  > targetLeftIn2)  pwmLeftIn2  = max(pwmLeftIn2  - SOFTSTART_STEP, targetLeftIn2);
  if (pwmRightIn1 < targetRightIn1) pwmRightIn1 = min(pwmRightIn1 + SOFTSTART_STEP, targetRightIn1); else if (pwmRightIn1 > targetRightIn1) pwmRightIn1 = max(pwmRightIn1 - SOFTSTART_STEP, targetRightIn1);
  if (pwmRightIn2 < targetRightIn2) pwmRightIn2 = min(pwmRightIn2 + SOFTSTART_STEP, targetRightIn2); else if (pwmRightIn2 > targetRightIn2) pwmRightIn2 = max(pwmRightIn2 - SOFTSTART_STEP, targetRightIn2);

  // Apply PWM
  analogWrite(LEFT_MOTOR_IN1,  pwmLeftIn1);
  analogWrite(LEFT_MOTOR_IN2,  pwmLeftIn2);
  analogWrite(RIGHT_MOTOR_IN1, pwmRightIn1);
  analogWrite(RIGHT_MOTOR_IN2, pwmRightIn2);

  // WS2812 LED channel indication - display unique colors for each active channel
  uint16_t ch1 = crsf.getChannel(1); // steering
  uint16_t ch2 = crsf.getChannel(2); // servo 1
  uint16_t ch3 = crsf.getChannel(3); // throttle
  uint16_t ch4 = crsf.getChannel(4); // servo 2
  
  // Channel thresholds - consider active if outside center deadband
  const int threshold = 100; // smaller threshold for channel detection
  
  bool ch1Active = (ch1 > center + threshold || ch1 < center - threshold);
  bool ch2Active = (ch2 > center + threshold || ch2 < center - threshold);
  bool ch3Active = (ch3 > center + threshold || ch3 < center - threshold);
  bool ch4Active = (ch4 > center + threshold || ch4 < center - threshold);
  
  // Priority-based color display (higher priority channels override lower ones)
  if (ch1Active && ch3Active) {
    // CH1 + CH3 active (steering + throttle): White
    ws2812.setPixelColor(0, ws2812.Color(255,255,255));
  } else if (ch1Active) {
    // CH1 active (steering): Red
    ws2812.setPixelColor(0, ws2812.Color(255,0,0));
  } else if (ch2Active) {
    // CH2 active (servo 1): Green  
    ws2812.setPixelColor(0, ws2812.Color(0,255,0));
  } else if (ch3Active) {
    // CH3 active (throttle): Blue
    ws2812.setPixelColor(0, ws2812.Color(0,0,255));
  } else if (ch4Active) {
    // CH4 active (servo 2): Yellow
    ws2812.setPixelColor(0, ws2812.Color(255,255,0));
  } else {
    // No channels active: Off
    ws2812.setPixelColor(0, ws2812.Color(0,0,0));
  }
  ws2812.show();
}

void controlRelays(const ControlData& cmd) {
  // If no RC signal (all main channels are zero), keep all relays OFF (LOW)
  if (cmd.steering == 0 && cmd.servoCh1 == 0 && cmd.throttle == 0 && cmd.servoCh2 == 0) {
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(RELAY4_PIN, LOW);
    return;
  }
  
  // Control relays by channels 5-8 from ControlData
  // digitalWrite(RELAY1_PIN, cmd.relayCh5 > 1500 ? HIGH : LOW);
  digitalWrite(RELAY2_PIN, cmd.relayCh6 > 1500 ? HIGH : LOW);
  digitalWrite(RELAY3_PIN, cmd.relayCh7 > 1500 ? HIGH : LOW);
  // digitalWrite(RELAY4_PIN, cmd.relayCh8 > 1500 ? HIGH : LOW);
}
