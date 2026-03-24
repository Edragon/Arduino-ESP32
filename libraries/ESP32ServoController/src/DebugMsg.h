#ifndef _MDO_SERVO_DebugMsg_H
#define _MDO_SERVO_DebugMsg_H


namespace MDO {
namespace ESP32ServoController {

//if you like, enable the following line for debug serial messages
//I tried this from the INO file, but could not get that to work..
#define ESP32_SERVO_CONTROLLER_LIB_DEBUG

#ifdef ESP32_SERVO_CONTROLLER_LIB_DEBUG
#define MDO_SERVO_DEBUG_PRINT(str) Serial.print(str)
#else
#define MDO_SERVO_DEBUG_PRINT(str)
#endif

#ifdef ESP32_SERVO_CONTROLLER_LIB_DEBUG
#define MDO_SERVO_DEBUG_PRINTLN(str) Serial.println(str)
#else 
#define MDO_SERVO_DEBUG_PRINTLN(str)
#endif

}	//namespace end
}	//namespace end

#endif