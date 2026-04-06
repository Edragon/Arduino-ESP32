// Author: Leonardo La Rocca
// email: info@melopero.com
// 
// In this example it is shown how to configure the device to detect
// gesture related interrupts.
// 
// First make sure that your connections are setup correctly:
// I2C pinout:
// APDS9960 <------> Arduino MKR
//     VIN <------> VCC
//     SCL <------> SCL (12)
//     SDA <------> SDA (11)
//     GND <------> GND
//     INT <------> 1 
// 
// In this example we are using an MKR board. Each arduino board (type)
// has dfferent pins that can listen for interrupts. If you want to learn 
// more about which pins can be used for interrupt detection on your arduino 
// board we recommend you to consult this site: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
//
// Note: Do not connect the device to the 5V pin!

#include "Melopero_APDS9960.h"

Melopero_APDS9960 device;

bool interruptOccurred = false;
//This is the pin that will listen for the hardware interrupt.
const byte interruptPin = 1;

void interruptHandler(){
  interruptOccurred = true;
}

void setup() {
  Serial.begin(9600); // Initialize serial comunication
  while (!Serial); // wait for serial to be ready

  int8_t status = NO_ERROR;
  
  Wire.begin();

  status = device.initI2C(0x39, Wire); // Initialize the comunication library
  if (status != NO_ERROR){
    Serial.println("Error during initialization");
    while(true);
  }
  status = device.reset(); // Reset all interrupt settings and power off the device
  if (status != NO_ERROR){
    Serial.println("Error during reset.");
    while(true);
  }

  Serial.println("Device initialized correctly!");

  // Gesture engine settings
  device.enableGesturesEngine(); // enable the gesture engine
  device.setGestureProxEnterThreshold(25); // Enter the gesture engine only when the proximity value 
  // is greater than this value proximity value ranges between 0 and 255 where 0 is far away and 255 is very near.
  device.setGestureExitThreshold(20); // Exit the gesture engine only when the proximity value is less 
  // than this value.
  device.setGestureExitPersistence(EXIT_AFTER_4_GESTURE_END); // Exit the gesture engine only when 4
  // consecutive gesture end signals are fired (distance is greater than the threshold)

  // Gesture engine interrupt settings
  device.enableGestureInterrupts();
  device.setGestureFifoThreshold(FIFO_INT_AFTER_16_DATASETS); // trigger an interrupt as soon as there are 16 datasets in the fifo
  // To clear the interrupt pin we have to read all datasets that are available in the fifo.
  // Since it takes a little bit of time to read alla these datasets the device may collect 
  // new ones in the meantime and prevent us from clearing the interrupt ( since the fifo 
  // would not be empty ). To prevent this behaviour we tell the device to enter the sleep 
  // state after an interrupt occurred. The device will exit the sleep state when the interrupt
  // is cleared.
  device.setSleepAfterInterrupt(true);

  //Next we want to setup our interruptPin to detect the interrupt and to call our
  //interruptHandler function each time an interrupt is triggered.
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), interruptHandler, FALLING);

  device.wakeUp(); // wake up the device
}

void loop() {
  if (interruptOccurred){
    // clear interrupt
    interruptOccurred = false;
    Serial.println("Interrupt occurred!");
    // The interrupt is cleared by reading all available datasets in the fifo

    // Retrieve number of datasets in fifo 
    device.updateNumberOfDatasetsInFifo();
    Serial.print("There are ");
    Serial.print(device.datasetsInFifo);
    Serial.println(" datasets in the fifo!");

    // print them out : UP DOWN LEFT RIGHT
    for (int i = 0; i < device.datasetsInFifo; i++){
      device.updateGestureData();

      Serial.print(i);
      Serial.print(" : ");
      Serial.print(device.gestureData[0]);
      Serial.print(" ");
      Serial.print(device.gestureData[1]);
      Serial.print(" ");
      Serial.print(device.gestureData[2]);
      Serial.print(" ");
      Serial.println(device.gestureData[3]);
    }
  }
}