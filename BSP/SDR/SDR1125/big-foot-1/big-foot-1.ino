#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RX 17
#define PIN_TX 16

// Motor Driver DRV8871 Pins
// Motor 1 (Left): IO15, IO18
#define M1_IN1 15
#define M1_IN2 18

// Motor 2 (Right): Reserved
#define M2_IN1 7
#define M2_IN2 8

// WS2812 Motion Indicator
#define RGB_PIN 48
#define NUM_PIXELS 1
Adafruit_NeoPixel statusLED(NUM_PIXELS, RGB_PIN, NEO_GRB + NEO_KHZ800);

// Channels
#define CH_THROTTLE 1    // Forward/Backward (CH1)
#define CH_STEERING 3    // Left/Right (CH3)
#define CH_SPEED_MODE 6  // Speed Mode (Low/Mid/High) (CH6)

// Motor speed scalers
float MOTOR_SCALER = 1.0;

// Servo definition
#define TAIL_SERVO_PIN 14
Servo tailServo;

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
  Serial.println("Rover Controller (Single Motor + Tail + RGB) initializing...");
  
  // Motor pins setup
  pinMode(M1_IN1, OUTPUT);
  pinMode(M1_IN2, OUTPUT);
  pinMode(M2_IN1, OUTPUT);
  pinMode(M2_IN2, OUTPUT);

  // RGB LED setup
  statusLED.begin();
  statusLED.setBrightness(50);
  statusLED.setPixelColor(0, statusLED.Color(0, 0, 255)); // Blue for init
  statusLED.show();

  // Servo setup
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  tailServo.setPeriodHertz(50);
  tailServo.attach(TAIL_SERVO_PIN, 500, 2400);

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
    int steering = map(steeringInput, 1000, 2000, 0, 180); // CH3 to Servo Angle

    // Apply Deadband to prevent motor hum near center
    if (abs(throttle) < 20) throttle = 0;

    int motorSpeed = throttle * MOTOR_SCALER;
    motorSpeed = constrain(motorSpeed, -255, 255);

    setMotor(motorSpeed, M1_IN1, M1_IN2);
    tailServo.write(steering);

    // Update Motion Indicator (WS2812)
    if (motorSpeed > 20) {
      statusLED.setPixelColor(0, statusLED.Color(0, 255, 0)); // Green for Forward
    } else if (motorSpeed < -20) {
      statusLED.setPixelColor(0, statusLED.Color(255, 0, 0)); // Red for Backward
    } else {
      statusLED.setPixelColor(0, statusLED.Color(0, 0, 0));   // Off for Neutral
    }
    statusLED.show();

    // Occasional debug print
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 200) {
      Serial.print("MODE:"); Serial.print(maxLimit == 85 ? "LOW" : (maxLimit == 170 ? "MID" : "HIGH"));
      Serial.print(" CH1:"); Serial.print(crsf.getChannel(1));
      Serial.print(" CH3:"); Serial.print(crsf.getChannel(3));
      Serial.print(" SPD:"); Serial.print(motorSpeed);
      Serial.print(" TAIL:"); Serial.println(steering);
      lastPrint = millis();
    }
  } else {
    // Failsafe: Stop motors if RC is lost
    setMotor(0, M1_IN1, M1_IN2);
    statusLED.setPixelColor(0, statusLED.Color(255, 255, 0)); // Yellow for signal lost
    statusLED.show();
  }
}
