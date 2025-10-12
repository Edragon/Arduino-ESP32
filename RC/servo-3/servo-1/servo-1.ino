#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>

#define PIN_RX 17
#define PIN_TX 16
#define SERVO_1_PIN 13
#define SERVO_2_PIN 14

// Set up a new Serial object
HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;

// Servo objects
Servo servo1;
Servo servo2;

void setup()
{
  Serial.begin(115200);
  Serial.println("COM Serial initialized");
  
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) while (1) Serial.println("Invalid crsfSerial configuration");

  crsf.begin(crsfSerial);
  
  // Initialize servos
  servo1.attach(SERVO_1_PIN);
  servo2.attach(SERVO_2_PIN);
  
  Serial.println("Servos initialized");
}

void loop()
{
    // Must call crsf.update() in loop() to process data
    crsf.update();
    controlServos();
    printChannels();
}

void controlServos()
{
  // Read CRSF channels 2 and 4
  int channel2Value = crsf.getChannel(2);
  int channel4Value = crsf.getChannel(4);
  
  // Debug: Print raw channel values to see actual range
  Serial.print("Ch1 raw: ");
  Serial.print(channel1Value);
  Serial.print(", Ch3 raw: ");
  Serial.println(channel3Value);
  
  // Try wider CRSF range mapping (common ranges: 172-1811, 988-2012, or 1000-2000)
  int servo1Angle = map(channel1Value, 988, 2012, 0, 180);
  int servo2Angle = map(channel3Value, 988, 2012, 0, 180);
  
  // Constrain values to valid servo range
  servo1Angle = constrain(servo1Angle, 0, 180);
  servo2Angle = constrain(servo2Angle, 0, 180);
  
  // Control the servos
  servo1.write(servo1Angle);
  servo2.write(servo2Angle);
  
  // Optional: Print servo positions
  Serial.print("Servo1: ");
  Serial.print(servo1Angle);
  Serial.print("°, Servo2: ");
  Serial.print(servo2Angle);
  Serial.println("°");
}

void printChannels()
{
  for (int ChannelNum = 1; ChannelNum <= 16; ChannelNum++)
  {
    Serial.print(crsf.getChannel(ChannelNum));
    Serial.print(", ");
  }
  Serial.println(" ");
}