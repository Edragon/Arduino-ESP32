#ifndef __MDO_ESP32SERVOCONTROLLER_LIB_
#define __MDO_ESP32SERVOCONTROLLER_LIB_


//Please see DebugMsg.h in case you want to enable / disable reported debug messages from this lib


//our main registry / settings
#include "Esp32LedcRegistry.h"

#include "PWMController.h"				//a PWM controller, for generic PWM needs
#include "ServoController.h"			//a servo controller, specifically meant for (180 degree) servo's

//the factory types, needed for the several options in [timer and channel] creation
#include "HighSpeedFactory.h"			//always high speed
#include "LowSpeedFactory.h"			//always low speed
#include "BestAvailableFactory.h"		//best available (high speed first, if not: tries low speed)
#include "ServoFactoryDecorator.h"		//specifically for servo's. A factory decorator which in itself needs one of the above factory instances
#include "PwmFactoryDecorator.h"		//A factory decorator specifically for PWM controllers with as main goal to re-use timers when possible 

//timers
#include "LedcTimer.h"					//generic base class for (ledc-) timers
#include "LedcTimerHighSpeed.h"			//high speed timer, availability is hardware platform related
#include "LedcTimerLowSpeed.h"			//low speed timer

//channels
#include "LedcChannel.h"				//generic base class for (ledc-) channels
#include "LedcChannelHighSpeed.h"		//high speed channel, availability is hardware platform related
#include "LedcChannelLowSpeed.h"		//low speed channel

#endif