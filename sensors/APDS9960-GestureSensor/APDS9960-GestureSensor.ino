/*
  APDS-9960 - Gesture Sensor

  This example reads gesture data from the on-board APDS-9960 sensor of the
  Nano 33 BLE Sense and prints any detected gestures to the Serial Monitor.

  Gesture directions are as follows:
  - UP:    from USB connector towards antenna
  - DOWN:  from antenna towards USB connector
  - LEFT:  from analog pins side towards digital pins side
  - RIGHT: from digital pins side towards analog pins side

  The circuit:
  - Arduino Nano 33 BLE Sense

  This example code is in the public domain.
*/

#include <Wire.h>
#include <Arduino_APDS9960.h>

constexpr int SDA_PIN = 12;   // change to your SDA pin
constexpr int SCL_PIN = 11;   // change to your SCL pin

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Wire.setPins(SDA_PIN, SCL_PIN);   // ESP32-specific
  Wire.begin(SDA_PIN, SCL_PIN);  // use this only if setPins() is not available

  if (!APDS.begin()) {
    Serial.println("Error initializing APDS-9960 sensor!");
    while (1) {}
  }

  Serial.println("Detecting gestures ...");
}
void loop() {
  if (APDS.gestureAvailable()) {
    // a gesture was detected, read and print to Serial Monitor
    int gesture = APDS.readGesture();

    switch (gesture) {
      case GESTURE_UP:
        Serial.println("Detected UP gesture");
        break;

      case GESTURE_DOWN:
        Serial.println("Detected DOWN gesture");
        break;

      case GESTURE_LEFT:
        Serial.println("Detected LEFT gesture");
        break;

      case GESTURE_RIGHT:
        Serial.println("Detected RIGHT gesture");
        break;

      default:
        // ignore
        break;
    }
  }
}
