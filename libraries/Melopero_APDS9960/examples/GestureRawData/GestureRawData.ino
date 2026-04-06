// Author: Leonardo La Rocca
// email: info@melopero.com
// 
// In this example it is shown how to configure the device to print out
// the gesture datasets stored in the gesture dataset fifo.
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

  device.initI2C(0x39, Wire); // Initialize the comunication library
  device.reset(); // Reset all interrupt settings and power off the device

  device.enableGesturesEngine(); // enable the gesture engine
  device.setGestureProxEnterThreshold(25); // Enter the gesture engine only when the proximity value 
  // is greater than this value proximity value ranges between 0 and 255 where 0 is far away and 255 is very near.
  device.setGestureExitThreshold(20); // Exit the gesture engine only when the proximity value is less 
  // than this value.
  device.setGestureExitPersistence(EXIT_AFTER_4_GESTURE_END); // Exit the gesture engine only when 4
  // consecutive gesture end signals are fired (distance is greater than the threshold)


  device.wakeUp(); // wake up the device
}

void loop() {

  // Retrieve number of datasets in fifo 
  device.updateNumberOfDatasetsInFifo();

  // print them out : UP DOWN LEFT RIGHT
  for (int i = 0; i < device.datasetsInFifo; i++){
    device.updateGestureData();

    Serial.print(device.gestureData[0]);
    Serial.print(" ");
    Serial.print(device.gestureData[1]);
    Serial.print(" ");
    Serial.print(device.gestureData[2]);
    Serial.print(" ");
    Serial.println(device.gestureData[3]);
  }

}
