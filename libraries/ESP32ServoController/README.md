# ESP32ServoController
A PWM and servo library for ESP32 platform.
This library follows the LEDC (LED Control) architecture from Espressif, see [here](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/api-reference/peripherals/ledc.html).

The LEDC architecture requires a timer (for the frequency) and a channel (for the duty cycle).
The combination of these are used in [PWMController](#PWMController) and the [ServoController](#ServoController).

Typically, there are a (very) limited amount timers. This library therefore has several options for (automatic) timer sharing.
This library has in itself no limits in the amount of PWM channels, Servo controllers, timers or channels. It scales based on the hardware capabilities.

Please see the UML in the [documentation page](https://github.com/mjdonders/ESP32ServoController/blob/main/doc/design.jpg).

## Usage
_Esp32LedcRegistry_ is a singleton used to register (/keep track of) the hardware platform capabilities.
Each sketch using this library should start by letting _Esp32LedcRegistry_ know the intended hardware platform using `Esp32LedcRegistry::instance()->begin`. 
_PWMController_ and _ServoController_ are the main classes to be used after this.

## PWMController
Depending on the platform (ESP32 subtype) used, timers and channels (as used by both _PWMController_ and _ServoController_) might be of type 'high speed' and/or 'low speed'.
Both _PWMController_ and _ServoController_ therefore require some guidance on which type should be used.
`PWMController.begin` requires a _Esp32LedcFactory_ instance. Options for that are:
 - _LowSpeedFactory_: always creates timers and channels in the low speed variant, when available (read: not all used).
 - _HighSpeedFactory_: always creates timers and channels in the high speed variant, when available for the platform and not fully used.
 - _BestAvailableFactory_: tries to create high speed timers/channels first. When that is (no longer) an option, it will try the low speed variants.
 - _PwmFactoryDecorator_: wraps one of the above _Esp32LedcFactory_ instances, with the goal of sharing a timer with another _PWMController_ if possible.

The easy option is therefor to use a _PwmFactoryDecorator_ with a _BestAvailableFactory_ in the case that there are no specific requirements.
See *PwmControllerShared* in [Examples](#Examples).

## ServoController
For the `ServoController.begin`, a _ServoFactoryDecorator_ is created.
This _ServoFactoryDecorator_ wraps a _Esp32LedcFactory_, making the interface easier to use.
Servo specific settings (frequency, min. timing and max. timing) can all be globally set using _Esp32LedcRegistry_.

## Examples
The main examples are:
 - *PwmController*, which demonstrates a basic PWM setup.
 - *PwmControllerShared*, which demonstrates a (library manager) timer-share. 
 - *ServoController*, demonstrating a basic servo setup. Please read the comments near `setServoParams` **carefully**. By default the _Esp32LedcRegistry_ will use 1 - 2 msec for 0 to 180 degrees.

(the **carefully** hint to physical servo damage which might occur based on wrong timing. Please check the relevant datasheet)


In addition, there are two additional 'examples'. These are mainly used for my test purposes, 'test suite' is therefore a better description.
These 'examples' might provide some additional clarification, but these are not really meant for that..
 - *PwmErrorHandlingTest_ESP32*, is a test suite for a ESP32 setup (LilyGo T-Display)
 - *PwmErrorHandlingTest_ESP32_S3*, is a test suite for a ESP32-S3 (LilyGo T-Display-S3)

I've compiled it for each ESP32 type found in the Esp32LedcRegistry comments, but have tested it only in the above two setup's.