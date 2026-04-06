// Author: Leonardo La Rocca
// email: info@melopero.com
// 
// In this example it is shown how to configure the device to detect
// gestures for a given amount of time.
// 
// First make sure that your connections are setup correctly:
// I2C pinout:
// APDS9960 <------> Arduino MKR
//     VIN <------> VCC
//     SCL <------> SCL (12)
//     SDA <------> SDA (11)
//     GND <------> GND
// 
//
// Note: Do not connect the device to the 5V pin!

#include "Melopero_APDS9960.h"

Melopero_APDS9960 device;

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

  device.wakeUp(); // wake up the device
}

void loop() {
  //update the status to see if there is some gesture data to parse
  device.updateGestureStatus();

  if (device.gestureFifoHasData){

    // Reads the gesture data for the given amount of time and tries to interpret a gesture. 
    // The device tries to detect a gesture by comparing the gesture data values through time. 
    // The device compares the up data with the down data to detect a gesture on the up-down axis and
    // it compares the left data with the right data to detect a gesture on the left right axis.
    //
    // ADVANCED SETTINGS:
    // device.parseGesture(uint parse_millis, uint8_t tolerance = 12, uint8_t der_tolerance = 6, uint8_t confidence = 6);
    //
    // parse_millis: the time in millisecond to read the gesture data and try to interpret a gesture
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
    device.parseGesture(300);

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