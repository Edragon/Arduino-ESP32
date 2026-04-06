#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  while (!Serial); // Wait for Serial to initialize (for boards like Leonardo)

  // Initialize the sensor
  if (sensor.begin()) {
    Serial.println("APDS9960 initialized successfully!");
  } else {
    Serial.println("Failed to initialize APDS9960. Check connections.");
    while (1); // Halt if initialization fails
  }

  // Enable proximity sensing
  sensor.enableProximitySensing(true);
  
  // Optional: Adjust sensitivity and range
  sensor.setProximitySensitivity(2); // Sensitivity level 2 (0-3)
  sensor.setProximitySensorRange(1);  // LED drive current level 1 (0-3)
}

void loop() {
  // Read proximity value (0-255, where higher values mean closer objects)
  uint8_t proximity = sensor.readProximity();
  
  Serial.print("Proximity: ");
  Serial.println(proximity);
  
  // Simple threshold-based detection
  if (proximity > 100) {
    Serial.println("Object detected nearby!");
  }
  
  delay(500); // Delay for readability
}