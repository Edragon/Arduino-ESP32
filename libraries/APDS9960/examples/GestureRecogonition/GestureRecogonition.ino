#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize the sensor with all features
  if (sensor.beginAll()) {
    Serial.println("APDS9960 initialized with all features!");
  } else {
    Serial.println("Failed to initialize APDS9960. Check connections.");
    while (1);
  }

  // Enable gesture sensing
  sensor.enableGestureSensing(true);
  
  // Optional: Adjust gesture settings
  sensor.setGestureGain(2);           // Gain factor 2 (0-3)
  sensor.setGestureSensitivity(1, 32); // Pulse length 1 (0-3), count 32 (0-63)
  sensor.setGestureDetectorMode(0);    // Mode 0: All directions active (0-3)
}

void loop() {
  // Read raw gesture data
  Gesture gesture = sensor.readGesture();
  
  // Resolve gesture into a direction
  String direction = sensor.resolveGesture(gesture, 50); // 50% threshold
  
  if (direction != "") {
    Serial.print("Gesture detected: ");
    Serial.println(direction);
  }
  
  delay(100); // Short delay to avoid flooding Serial
}