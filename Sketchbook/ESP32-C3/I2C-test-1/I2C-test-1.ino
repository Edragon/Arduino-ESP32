/*
  Author: Roy Hazan
  Date: 9th August 2021
*/
// #include "task_watchdog_example_main.c"

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif

#define CORE_DEBUG_LEVEL 3
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG

#include <Wire.h>
#define I2C_SDA 5
#define I2C_SCL 6

byte address; // For printing error information

TwoWire I2CBME = TwoWire(0);

void PrintErrors(bool bResult) {
  return;
  if (!bResult) {
    uint8_t errorNumber = Wire.lastError();
    char * errorText = Wire.getErrorText(errorNumber);
    Serial.print("\nERROR ");
    Serial.print(errorNumber);
    Serial.print(" ");
    if (errorNumber < 16) Serial.print("0");
    Serial.println(errorNumber, HEX);
    Serial.print(" ");
    Serial.print(errorText);
  }
}

void PrintError(byte errorNumber) {
  return;
  char * errorText = Wire.getErrorText(errorNumber);
  Serial.print("\nERROR ");
  Serial.print(errorNumber);
  Serial.print(" ");
  if (errorNumber < 16) Serial.print("0");
  Serial.println(errorNumber, HEX);
  Serial.print(" ");
  Serial.print(errorText);
}

void setup() {
  //I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  PrintErrors(Wire.setPins(I2C_SDA, I2C_SCL));
  PrintErrors(Wire.begin());
  Serial.begin(115200);
  Serial.println("\nI2C Scanner");
}

void loop() {
  byte errorByte;
  int nDevices;
  Serial.println("Scanning...");
  nDevices = 0;
  for (address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    Wire.write("c");
    errorByte = Wire.endTransmission();
    if (errorByte == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    } else {
      Serial.print("Error ");
      Serial.print(errorByte);
      Serial.print(" = 0x");
      Serial.print(errorByte, HEX);
      Serial.print(" at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
  }
  else {
    Serial.print(nDevices);
    Serial.println(" I2C devices found\n");
  }

  delay(5000);
}
