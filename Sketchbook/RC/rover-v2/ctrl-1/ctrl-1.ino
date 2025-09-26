#include <AlfredoCRSF.h>
#include <HardwareSerial.h>

#define PIN_RX 16
#define PIN_TX 17

#define LEFT_MOTOR_IN1 15
#define LEFT_MOTOR_IN2 18
#define RIGHT_MOTOR_IN1 7
#define RIGHT_MOTOR_IN2 8

// Set up a new Serial object
HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;

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
}

void loop()
{
    // Must call crsf.update() in loop() to process data
    crsf.update();
    controlMotors();
    printChannels();
}

// Map channel values to motor control
void controlMotors()
{
  int roll = crsf.getChannel(1);      // Channel 1: Roll (left/right)
  int throttle = crsf.getChannel(4);  // Channel 4: Throttle (forward/backward)

  // Center values for CRSF are typically 992-1504-2011
  // We'll use 1500 as center, <1450 is left/reverse, >1550 is right/forward
  const int center = 1500;
  const int deadband = 50;

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

  // Left/right logic (steering)
  if (roll > center + deadband) {
    // Turn right: left motor forward, right motor backward
    leftForward = true;
    rightBackward = true;
    leftBackward = false;
    rightForward = false;
  } else if (roll < center - deadband) {
    // Turn left: right motor forward, left motor backward
    rightForward = true;
    leftBackward = true;
    leftForward = false;
    rightBackward = false;
  }

  // Set left motor
  digitalWrite(LEFT_MOTOR_IN1, leftForward ? HIGH : LOW);
  digitalWrite(LEFT_MOTOR_IN2, leftBackward ? HIGH : LOW);
  // Set right motor
  digitalWrite(RIGHT_MOTOR_IN1, rightForward ? HIGH : LOW);
  digitalWrite(RIGHT_MOTOR_IN2, rightBackward ? HIGH : LOW);
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