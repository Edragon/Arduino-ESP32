#include <Wire.h>

void setup() {
  Wire.begin(22, 21);
  Serial.begin(115200);
  delay(1000);
  
  byte i2cAddress = 0x39; // Default APDS9960 address
  byte idRegister = 0x92; // Identification register
  byte chipID = 0;

  Serial.println("--- APDS9960 Authenticity Check ---");

  Wire.beginTransmission(i2cAddress);
  Wire.write(idRegister);
  
  if (Wire.endTransmission() == 0) {
    Wire.requestFrom(i2cAddress, (byte)1);
    if (Wire.available()) {
      chipID = Wire.read();
      Serial.print("Device found at 0x39. Reported Chip ID: 0x");
      Serial.println(chipID, HEX);

      if (chipID == 0xAB) {
        Serial.println("SUCCESS: Genuine APDS9960 detected.");
      } else if (chipID == 0xA8) {
        Serial.println("WARNING: Clone detected (Returned 0xA8).");
      } else {
        Serial.print("UNKNOWN: Chip ID 0x");
        Serial.print(chipID, HEX);
        Serial.println(" is not standard for this sensor.");
      }
    }
  } else {
    Serial.println("ERROR: APDS9960 not found at 0x39. Check wiring/power.");
  }
}

void loop() {}