#include <AlfredoCRSF.h>
#include <HardwareSerial.h>

#define PIN_RX 17
#define PIN_TX 16

// Motor Driver DRV8871 Pins
// Motor 1 (Left): IO15, IO18
#define M1_IN1 15
#define M1_IN2 18

// Motor 2 (Right): IO7, IO8
#define M2_IN1 7
#define M2_IN2 8

// Channels
#define CH_THROTTLE 1    // Forward/Backward (CH1)
#define CH_STEERING 3    // Left/Right (CH3)
#define CH_SPEED_MODE 6  // Speed Mode (Low/Mid/High) (CH6)

// Motor speed scalers (to compensate for motor variance)
// Current issue: Left is faster, Right is slower
float LEFT_MOTOR_SCALER = 0.8;
float RIGHT_MOTOR_SCALER = 0.5;

// Reserved Servo IOs (not in use)
#define SERVO1_PIN 11
#define SERVO2_PIN 12
#define SERVO3_PIN 13
#define SERVO4_PIN 14

// Reserved Buzzer control pin (not in use)
#define BUZZER_PIN 46

// Reserved Relay IOs (not in use)
#define RELAY1_PIN 9
#define RELAY2_PIN 10

// Reserved MOSFET IOs (not in use)
#define MOSFET1_PIN 5
#define MOSFET2_PIN 6

// Battery Monitor
#define BATTERY_ADC_PIN 36

// Set up a new Serial object
HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;

/**
 * Control a single motor
 * @param speed -255 to 255
 * @param pin1 Primary control pin
 * @param pin2 Secondary control pin
 */
void setMotor(int speed, int pin1, int pin2) {
  if (speed > 0) {
    if (speed > 255) speed = 255;
    analogWrite(pin1, speed);
    analogWrite(pin2, 0);
  } else if (speed < 0) {
    speed = -speed;
    if (speed > 255) speed = 255;
    analogWrite(pin1, 0);
    analogWrite(pin2, speed);
  } else {
    analogWrite(pin1, 0);
    analogWrite(pin2, 0);
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Rover Controller initializing...");
  
  // Motor pins setup
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  // Reserved Servo IOs setup (commented out/not in use)
  // pinMode(SERVO1_PIN, OUTPUT);
  // pinMode(SERVO2_PIN, OUTPUT);
  // pinMode(SERVO3_PIN, OUTPUT);
  // pinMode(SERVO4_PIN, OUTPUT);

  // Reserved Buzzer setup (not in use)
  // pinMode(BUZZER_PIN, OUTPUT);

  // Reserved Relay setup (not in use)
  // pinMode(RELAY1_PIN, OUTPUT);
  // pinMode(RELAY2_PIN, OUTPUT);

  // Reserved MOSFET setup (not in use)
  // pinMode(MOSFET1_PIN, OUTPUT);
  // pinMode(MOSFET2_PIN, OUTPUT);

  // Battery Monitor setup
  pinMode(BATTERY_ADC_PIN, INPUT);

  // Stop motors initially
  setMotor(0, M1_IN1, M1_IN2);
  setMotor(0, M2_IN1, M2_IN2);
  
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) {
    while (1) {
      Serial.println("Invalid crsfSerial configuration");
      delay(1000);
    }
  }

  crsf.begin(crsfSerial);
  Serial.println("CRSF initialized");
}

void loop()
{
  // Must call crsf.update() in loop() to process data
  crsf.update();

  if (crsf.isLinkUp()) {
    // CRSF values typically range from 1000 to 2000
    int throttleInput = crsf.getChannel(CH_THROTTLE); 
    int steeringInput = crsf.getChannel(CH_STEERING);
    int speedModeInput = crsf.getChannel(CH_SPEED_MODE);

    // Determine max PWM based on Speed Mode (CH6)
    int maxLimit = 255;
    if (speedModeInput < 1300) {
      maxLimit = 85;    // Low Speed
    } else if (speedModeInput < 1700) {
      maxLimit = 170;   // Middle Speed
    } else {
      maxLimit = 255;   // High Speed
    }

    // Map Input (1000-2000) to selected speed range (Inverted)
    int throttle = map(throttleInput, 1000, 2000, maxLimit, -maxLimit);
    int steering = map(steeringInput, 1000, 2000, maxLimit, -maxLimit);

    // Apply Deadband to prevent motor hum near center
    if (abs(throttle) < 20) throttle = 0;
    if (abs(steering) < 20) steering = 0;

    // Mixed differential steering
    int leftSpeed = (throttle + steering) * LEFT_MOTOR_SCALER;
    int rightSpeed = (throttle - steering) * RIGHT_MOTOR_SCALER;

    // Constrain to PWM range
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    // Stagger motor starts to reduce initial current surge
    static int lastLeftSent = 0;
    static int lastRightSent = 0;
    static unsigned long lastMotorStartedAt = 0;

    if (leftSpeed != 0 && lastLeftSent == 0) {
      if (millis() - lastMotorStartedAt < 150) {
        leftSpeed = 0;
      } else {
        lastMotorStartedAt = millis();
      }
    }

    if (rightSpeed != 0 && lastRightSent == 0) {
      if (millis() - lastMotorStartedAt < 150) {
        rightSpeed = 0;
      } else {
        lastMotorStartedAt = millis();
      }
    }
    
    lastLeftSent = leftSpeed;
    lastRightSent = rightSpeed;

    setMotor(leftSpeed, M1_IN1, M1_IN2);
    setMotor(rightSpeed, M2_IN1, M2_IN2);

    // Occasional debug print
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
      Serial.print("MODE:"); Serial.print(maxLimit == 85 ? "LOW" : (maxLimit == 170 ? "MID" : "HIGH"));
      Serial.print(" CH1:"); Serial.print(crsf.getChannel(1));
      Serial.print(" CH3:"); Serial.print(crsf.getChannel(3));
      Serial.print(" THR:"); Serial.print(throttle);
      Serial.print(" STR:"); Serial.print(steering);
      Serial.print(" L:"); Serial.print(leftSpeed);
      Serial.print(" R:"); Serial.println(rightSpeed);
      lastPrint = millis();
    }
  } else {
    // Failsafe: Stop motors if RC is lost
    setMotor(0, M1_IN1, M1_IN2);
    setMotor(0, M2_IN1, M2_IN2);
  }
}
