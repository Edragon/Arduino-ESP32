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
#define CH_THROTTLE 3  // Forward/Backward (CH3)
#define CH_STEERING 1  // Left/Right (CH1)

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

  if (crsf.isConnected()) {
    // CRSF values typically range from 1000 to 2000
    int throttleInput = crsf.getChannel(CH_THROTTLE); 
    int steeringInput = crsf.getChannel(CH_STEERING);

    // Map Input (1000-2000) to Range (-255 to 255)
    // Adjust signs if directions are inverted
    int throttle = map(throttleInput, 1000, 2000, -255, 255);
    int steering = map(steeringInput, 1000, 2000, -255, 255);

    // Apply Deadband to prevent motor hum near center
    if (abs(throttle) < 20) throttle = 0;
    if (abs(steering) < 20) steering = 0;

    // Mixed differential steering
    int leftSpeed = throttle + steering;
    int rightSpeed = throttle - steering;

    // Constrain to PWM range
    leftSpeed = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    setMotor(leftSpeed, M1_IN1, M1_IN2);
    setMotor(rightSpeed, M2_IN1, M2_IN2);

    // Occasional debug print
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
      Serial.print("THR:"); Serial.print(throttle);
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
