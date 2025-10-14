#include "taskServo.h"
#include "taskLogger.h"
#include "SharedGlobals.h"

// Selected LEDC resources (defaults to preferred)
ledc_mode_t servoSpeedMode = SERVO_SPEED_MODE;
ledc_timer_t servoTimer = SERVO_TIMER;

// Servo pulse width ranges (microseconds)
int servoUP_MinUs = SERVO_MIN_PULSE_US;
int servoUP_MaxUs = SERVO_MAX_PULSE_US;
int servoDN_MinUs = SERVO_MIN_PULSE_US;
int servoDN_MaxUs = SERVO_MAX_PULSE_US;

// Servo position variables
int pos1 = 90; // Servo 1 position (0-180 degrees)
int pos2 = 90; // Servo 2 position (0-180 degrees)

// Calculate duty cycle for given pulse width in microseconds
uint32_t calculateDutyCycle(uint16_t pulseWidthUs) {
  // For 50Hz (20ms period) with 14-bit resolution (16384 levels)
  // Duty cycle = (pulse_width_us / 20000) * 16384
  return (uint32_t)((pulseWidthUs * 16384ULL) / 20000);
}

// Convert servo angle (0-180) to pulse width in microseconds
uint16_t angleToPulseWidth(int angle) {
  return map(angle, 0, 180, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
}

// Set servo position using LEDC PWM
void setServoPosition(ledc_channel_t channel, int angle) {
  uint16_t pulseWidth = angleToPulseWidth(angle);
  uint32_t dutyCycle = calculateDutyCycle(pulseWidth);
  ledc_set_duty(servoSpeedMode, channel, dutyCycle);
  ledc_update_duty(servoSpeedMode, channel);
}

static esp_err_t configure_servo_timer_with_fallback() {
  // Try LOW_SPEED timers to avoid conflicts with analogWrite
  const struct { ledc_mode_t mode; ledc_timer_t timer; const char* label; } tries[] = {
    { LEDC_LOW_SPEED_MODE,  LEDC_TIMER_3, "LS/T3" },
    { LEDC_LOW_SPEED_MODE,  LEDC_TIMER_2, "LS/T2" },
    { LEDC_LOW_SPEED_MODE,  LEDC_TIMER_1, "LS/T1" },
    { LEDC_LOW_SPEED_MODE,  LEDC_TIMER_0, "LS/T0" },
  };

  for (size_t i = 0; i < sizeof(tries)/sizeof(tries[0]); ++i) {
    ledc_mode_t m = tries[i].mode;
    ledc_timer_t t = tries[i].timer;

    LOG_INFO_SERVO("Trying LEDC config %s", tries[i].label);
    // Reset the specific timer in the selected mode before configuring
    ledc_timer_rst(m, t);

    ledc_timer_config_t cfg = {};
    cfg.speed_mode = m;
    cfg.duty_resolution = LEDC_TIMER_14_BIT;
    cfg.timer_num = t;
    cfg.freq_hz = SERVO_FREQUENCY;

    esp_err_t err = ledc_timer_config(&cfg);
    if (err == ESP_OK) {
      servoSpeedMode = m;
      servoTimer = t;
      LOG_INFO_SERVO("Using LEDC %s successfully", tries[i].label);
      return ESP_OK;
    }
    LOG_WARN_SERVO("LEDC %s failed: %d", tries[i].label, (int)err);
  }
  return ESP_FAIL;
}

void initServos() {
  // Pick a timer/mode that doesn't conflict
  ESP_ERROR_CHECK(configure_servo_timer_with_fallback());

  // Configure LEDC channel for servo UP
  ledc_channel_config_t ledc_channel_up = {};
  ledc_channel_up.gpio_num = servoUP_PIN;
  ledc_channel_up.speed_mode = servoSpeedMode;
  ledc_channel_up.channel = SERVO_UP_CHANNEL;
  ledc_channel_up.timer_sel = servoTimer;
  ledc_channel_up.duty = 0;
  ledc_channel_up.hpoint = 0;
  ledc_channel_up.flags.output_invert = 0;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_up));

  // Configure LEDC channel for servo DN
  ledc_channel_config_t ledc_channel_dn = {};
  ledc_channel_dn.gpio_num = servoDN_PIN;
  ledc_channel_dn.speed_mode = servoSpeedMode;
  ledc_channel_dn.channel = SERVO_DN_CHANNEL;
  ledc_channel_dn.timer_sel = servoTimer;
  ledc_channel_dn.duty = 0;
  ledc_channel_dn.hpoint = 0;
  ledc_channel_dn.flags.output_invert = 0;
  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel_dn));

  // Set servos to center position (90 degrees)
  LOG_INFO_SERVO("Setting servos to center position (90°)");
  setServoPosition(SERVO_UP_CHANNEL, 90);
  setServoPosition(SERVO_DN_CHANNEL, 90);
  
  LOG_INFO_SERVO("Servos initialized with LEDC hardware PWM (mode=%d, timer=%d)", servoSpeedMode, servoTimer);
  LOG_INFO_SERVO("Servo UP on pin %d, Servo DN on pin %d", servoUP_PIN, servoDN_PIN);
  LOG_INFO_SERVO("PWM frequency: %dHz, Resolution: 14-bit", SERVO_FREQUENCY);
}

void taskServo(void* pv) {
  (void)pv;
  LOG_INFO_SERVO("Task started");
  
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(20); // 50 Hz servo update rate

  for (;;) {
    ControlData cmd;
    uint16_t ser1 = 1500, ser2 = 1500; // Default values
    
    // Get servo data from motor command queue (using new channel mapping)
    if (xQueuePeek(motorCmdQueue, &cmd, pdMS_TO_TICKS(1)) == pdTRUE) {
      ser1 = cmd.servoCh1;  // Channel 6 for servo 1 (mapped to servoCh1)
      ser2 = cmd.servoCh2;  // Channel 7 for servo 2 (mapped to servoCh2)
    }
    
    // Convert CRSF channel values to servo degrees (0-180)
    // Always update servo positions when we have valid CRSF data
    if (ser1 != 0) { // Valid CRSF data received
      pos1 = map(ser1, 988, 2012, 0, 180);
      pos1 = constrain(pos1, 0, 180); // Ensure within servo range
      setServoPosition(SERVO_UP_CHANNEL, pos1);
    }
    
    if (ser2 != 0) { // Valid CRSF data received  
      pos2 = map(ser2, 988, 2012, 0, 180);
      pos2 = constrain(pos2, 0, 180); // Ensure within servo range
      setServoPosition(SERVO_DN_CHANNEL, pos2);
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
