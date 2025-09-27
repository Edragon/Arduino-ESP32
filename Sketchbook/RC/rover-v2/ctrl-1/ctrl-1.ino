#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define PIN_RX 17
#define PIN_TX 16

#define LEFT_MOTOR_IN1 15
#define LEFT_MOTOR_IN2 18
#define RIGHT_MOTOR_IN1 7
#define RIGHT_MOTOR_IN2 8

#define RELAY1_PIN 5
#define RELAY2_PIN 6
#define RELAY3_PIN 19
#define RELAY4_PIN 20

#define SIGNAL_LED_PIN 35
#define WS2812_PIN 48
#define WS2812_NUM_LEDS 1

// Set up a new Serial object
HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;
Adafruit_NeoPixel ws2812(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// Soft start variables
int pwmLeftIn1 = 0, pwmLeftIn2 = 0, pwmRightIn1 = 0, pwmRightIn2 = 0;
const int SOFTSTART_STEP = 8; // PWM increment per loop
const int SOFTSTART_TARGET = 128; // 50% speed

void setup()
{
  Serial.begin(115200);
  Serial.println("COM Serial initialized");
  
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) while (1) Serial.println("Invalid crsfSerial configuration");

  crsf.begin(crsfSerial);

  // Motor pins setup
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2, OUTPUT);

  // Set all motor pins LOW at startup
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Set all relay pins LOW at startup
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  pinMode(SIGNAL_LED_PIN, OUTPUT);
  digitalWrite(SIGNAL_LED_PIN, LOW);
  ws2812.begin();
  ws2812.show();
}

void loop()
{
    // Must call crsf.update() in loop() to process data
    crsf.update();
    controlMotors();
    controlRelays();
    updateSignalLed();
    printChannels();
}

void updateSignalLed() {
  // LED on if any channel 1-4 is nonzero
  if (crsf.getChannel(1) != 0 || crsf.getChannel(2) != 0 || crsf.getChannel(3) != 0 || crsf.getChannel(4) != 0) {
    digitalWrite(SIGNAL_LED_PIN, HIGH);
  } else {
    digitalWrite(SIGNAL_LED_PIN, LOW);
  }
}

// Map channel values to motor control
void controlMotors()
{
  int roll = crsf.getChannel(1);      // Channel 1: Roll (left/right)
  int throttle = crsf.getChannel(3);  // Channel 3: Throttle (forward/backward)
  int ch2 = crsf.getChannel(2);
  int ch3 = throttle;

  // Failsafe: if all CRSF channels 1-4 are 0, stop motors and turn off WS2812
  if (roll == 0 && throttle == 0 && ch2 == 0 && ch3 == 0) {
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
  // We'll use 1500 as center, <1450 is left/reverse, >1550 is right/forward
  const int center = 1500;
  const int deadband = 50;

  // Determine direction for each motor
  bool leftForward = false, leftBackward = false;
  bool rightForward = false, rightBackward = false;

  // Reverse forward/backward logic
  if (throttle > center + deadband) {
    // throttle high = forward
    leftForward = true;
    rightForward = true;
  } else if (throttle < center - deadband) {
    // throttle low = backward
    leftBackward = true;
    rightBackward = true;
  }

  // Reverse left/right logic (steering)
  if (roll > center + deadband) {
    // Now: turn right = left motor backward, right motor forward
    leftBackward = true;
    rightForward = true;
    leftForward = false;
    rightBackward = false;
  } else if (roll < center - deadband) {
    // Now: turn left = right motor backward, left motor forward
    rightBackward = true;
    leftForward = true;
    leftBackward = false;
    rightForward = false;
  }

  // Determine target PWM for each pin
  int targetLeftIn1 = leftForward ? SOFTSTART_TARGET : 0;
  int targetLeftIn2 = leftBackward ? SOFTSTART_TARGET : 0;
  int targetRightIn1 = rightForward ? SOFTSTART_TARGET : 0;
  int targetRightIn2 = rightBackward ? SOFTSTART_TARGET : 0;

  // Soft start ramping
  if (pwmLeftIn1 < targetLeftIn1) pwmLeftIn1 = min(pwmLeftIn1 + SOFTSTART_STEP, targetLeftIn1);
  else if (pwmLeftIn1 > targetLeftIn1) pwmLeftIn1 = max(pwmLeftIn1 - SOFTSTART_STEP, targetLeftIn1);

  if (pwmLeftIn2 < targetLeftIn2) pwmLeftIn2 = min(pwmLeftIn2 + SOFTSTART_STEP, targetLeftIn2);
  else if (pwmLeftIn2 > targetLeftIn2) pwmLeftIn2 = max(pwmLeftIn2 - SOFTSTART_STEP, targetLeftIn2);

  if (pwmRightIn1 < targetRightIn1) pwmRightIn1 = min(pwmRightIn1 + SOFTSTART_STEP, targetRightIn1);
  else if (pwmRightIn1 > targetRightIn1) pwmRightIn1 = max(pwmRightIn1 - SOFTSTART_STEP, targetRightIn1);

  if (pwmRightIn2 < targetRightIn2) pwmRightIn2 = min(pwmRightIn2 + SOFTSTART_STEP, targetRightIn2);
  else if (pwmRightIn2 > targetRightIn2) pwmRightIn2 = max(pwmRightIn2 - SOFTSTART_STEP, targetRightIn2);

  // Set left motor
  analogWrite(LEFT_MOTOR_IN1, pwmLeftIn1);
  analogWrite(LEFT_MOTOR_IN2, pwmLeftIn2);
  // Set right motor
  analogWrite(RIGHT_MOTOR_IN1, pwmRightIn1);
  analogWrite(RIGHT_MOTOR_IN2, pwmRightIn2);

  // WS2812 LED direction indication
  if (leftForward && rightForward) {
    ws2812.setPixelColor(0, ws2812.Color(0,255,0)); // Green: forward
  } else if (leftBackward && rightBackward) {
    ws2812.setPixelColor(0, ws2812.Color(255,0,0)); // Red: backward
  } else if (leftForward && rightBackward) {
    ws2812.setPixelColor(0, ws2812.Color(255,255,0)); // Yellow: right
  } else if (leftBackward && rightForward) {
    ws2812.setPixelColor(0, ws2812.Color(0,0,255)); // Blue: left
  } else {
    ws2812.setPixelColor(0, ws2812.Color(0,0,0)); // Off
  }
  ws2812.show();
}

void controlRelays()
{
  // If no RC signal (all channels 1-4 are zero), keep all relays OFF (LOW)
  if (crsf.getChannel(1) == 0 && crsf.getChannel(2) == 0 && crsf.getChannel(3) == 0 && crsf.getChannel(4) == 0) {
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(RELAY4_PIN, LOW);
    return;
  }
  // Otherwise, control relays by channels 5-8
  digitalWrite(RELAY1_PIN, crsf.getChannel(5) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY2_PIN, crsf.getChannel(6) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY3_PIN, crsf.getChannel(7) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY4_PIN, crsf.getChannel(8) > 1500 ? HIGH : LOW);
}

//Use crsf.getChannel(x) to get us channel values (1-16).
void printChannels()
{
  for (int ChannelNum = 1; ChannelNum <= 16; ChannelNum++)
  {
    Serial.print(crsf.getChannel(ChannelNum));
    Serial.print(", ");
  }
  Serial.println(" ");
}