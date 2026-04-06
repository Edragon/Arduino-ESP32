// Author: Leonardo La Rocca
// email: info@melopero.com
// 
// In this example it is shown how to configure the device to print out
// The proximity value and how to tell if any errors occurred.
// 
// First make sure that your connections are setup correctly:
// I2C pinout:
// APDS9960 <------> Arduino MKR
//     VIN <------> VCC
//     SCL <------> SCL (12)
//     SDA <------> SDA (11)
//     GND <------> GND
// 
// Note: Do not connect the device to the 5V pin!

#include "Melopero_APDS9960.h"

Melopero_APDS9960 device;

void setup() {
  Serial.begin(9600); // Initialize serial comunication
  while (!Serial); // wait for serial to be ready

  Wire.begin();

  // Every function of the library returns a status code (int8_t). 
  // There are 3 possible status codes (more may be added in the future):
  // NO_ERROR, I2C_ERROR, INVALID_ARGUMENT
  int8_t status = NO_ERROR;
  status = device.initI2C(0x39, Wire); // Initialize the comunication library
  printError(status);
  status = device.reset(); // Reset all interrupt settings and power off the device
  printError(status);

  status = device.enableProximityEngine(); // Enable the proximity engine
  printError(status);
  
  status = device.wakeUp(); // Wake up the device
  printError(status);
}

void loop() {
  delay(500);
  int8_t status = NO_ERROR;
  status = device.updateProximityData(); // Update the proximity data and retrieve the status code
  printError(status); // Examine the status code 

  Serial.println(device.proximityData); // print the proximity data
}

void printError(int8_t error_code){
  if (error_code == NO_ERROR)
    Serial.println("No error :)");
  else if (error_code == I2C_ERROR)
    Serial.println("I2C comunication error :(");
  else if (error_code == INVALID_ARGUMENT)
    Serial.println("Invalid argument error :(");
  else 
    Serial.println("Unknown error O.O");
}
