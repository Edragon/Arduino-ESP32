#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Initialize the sensor
  if (sensor.begin()) {
    Serial.println("APDS9960 initialized successfully!");
  } else {
    Serial.println("Failed to initialize APDS9960. Check connections.");
    while (1);
  }

  // Enable light sensing
  sensor.enableLightSensing(true);
  
  // Optional: Adjust light sensitivity and gain
  sensor.setLightSensitivity(100); // Shutter speed (0-255)
  sensor.setLightGain(1);          // Gain factor 1 (0-3)
}

void loop() {
  // Read color data
  Color color = sensor.readColorData();
  
  // Print RGB and clear values
  Serial.print("Red: ");   Serial.print(color.red);
  Serial.print(" Green: "); Serial.print(color.green);
  Serial.print(" Blue: ");  Serial.print(color.blue);
  Serial.print(" Clear: "); Serial.println(color.clear);
  
  // Simple color detection example
  if (color.red > color.green && color.red > color.blue) {
    Serial.println("Dominant color: Red");
  }
  
  delay(1000); // Delay for readability
}