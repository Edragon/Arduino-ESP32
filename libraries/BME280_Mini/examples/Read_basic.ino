#include <BME280_Mini.h>

// Create sensor object - connect SDA to pin 2, SCL to pin 3
BME280_Mini bme(2, 3);  // SDA=2, SCL=3

void setup() {
  // Start serial communication
  Serial.begin(9600);
  
  // Initialize the sensor
  if (!bme.begin()) {
    Serial.println("Could not find the BME280 sensor!");
    while (1); // Don't continue if sensor not found
  }
}

void loop() {
  // Create a variable to store the sensor readings
  BME280_Mini::Data data;
  
  // Try to read the sensor
  if (bme.read(data)) {
    // Print temperature
    Serial.print("Temperature: ");
    Serial.print(data.temperature);
    Serial.println(" °C");
    
    // Print pressure
    Serial.print("Pressure: ");
    Serial.print(data.pressure);
    Serial.println(" hPa");
    
    // Print humidity
    Serial.print("Humidity: ");
    Serial.print(data.humidity);
    Serial.println(" %");
    
    Serial.println("-------------------");
  }
  
  // Wait 2 seconds before next reading
  delay(2000);
}