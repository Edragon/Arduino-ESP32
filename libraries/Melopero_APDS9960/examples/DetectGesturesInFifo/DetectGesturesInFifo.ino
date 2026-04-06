// Author: Leonardo La Rocca
// email: info@melopero.com
// 
// In this example it is shown how to configure the device to detect
// gestures.
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
   
    // The interrupt is cleared by reading all available datasets in the fifo
    // When an interrupt occurs we know we have 16 datasets in the fifo, by analyzing 
    // these datasets we can detect a gesture.

    // Reads the gesture data currently available in the fifo and tries to interpret a gesture. 
    // The device tries to detect a gesture by comparing the gesture data values through time. 
    // The device compares the up data with the down data to detect a gesture on the up-down axis and
    // it compares the left data with the right data to detect a gesture on the left right axis.
    //
    // ADVANCED SETTINGS:
    // device.parseGestureInFifo(uint8_t tolerance = 12, uint8_t der_tolerance = 6, uint8_t confidence = 6);
    //
    // The tolerance parameter determines how much the two values (on the same axis) have to differ to interpret
    // the current dataset as valid for gesture detection (if the values are nearly the same then its not possible to decide the direction 
    // in which the object is moving).
    //
    // The der_tolerance does the same for the derivative of the two curves (the values on one axis through time):
    // this prevents the device from detecting a gesture if the objects surface is not even...
    //
    // The confidence tells us the minimum amount of "detected gesture samples" needed for an axis to tell that a gesture has been detected on that axis:
    // How its used in the source code: if (detected_up_gesture_samples > detected_down_gesture_samples + confidence) gesture_up_down = GESTURE_UP
    device.parseGestureInFifo();

    if (device.parsedUpDownGesture != NO_GESTURE || device.parsedLeftRightGesture != NO_GESTURE)
        Serial.print("Gesture : ");

    if (device.parsedUpDownGesture == UP_GESTURE)
        Serial.print("UP ");
    else if (device.parsedUpDownGesture == DOWN_GESTURE)
        Serial.print("DOWN ");

    if (device.parsedLeftRightGesture == LEFT_GESTURE)
        Serial.print("LEFT ");
    else if (device.parsedLeftRightGesture == RIGHT_GESTURE)
        Serial.print("RIGHT ");

    if (device.parsedUpDownGesture != NO_GESTURE || device.parsedLeftRightGesture != NO_GESTURE)
        Serial.println();
  }

}