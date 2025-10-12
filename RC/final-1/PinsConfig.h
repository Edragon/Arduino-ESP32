#ifndef PINS_CONFIG_H
#define PINS_CONFIG_H

// CRSF/ELRS Serial Pins
#define PIN_RX 17
#define PIN_TX 16

// MQTT Modem Serial Pins
#define MODEM_RX_PIN 1
#define MODEM_TX_PIN 2

// Motor Control Pins
#define LEFT_MOTOR_IN1 15
#define LEFT_MOTOR_IN2 18
#define RIGHT_MOTOR_IN1 7
#define RIGHT_MOTOR_IN2 8

// Relay Control Pins
#define RELAY1_PIN 5
#define RELAY2_PIN 6
#define RELAY3_PIN 19
#define RELAY4_PIN 20

// LED Pins
#define SIGNAL_LED_PIN 35
#define WS2812_PIN 48
#define WS2812_NUM_LEDS 1

// Servo Pins
#define servoUP_PIN 13
#define servoDN_PIN 14

// ADC Pins
#define BATTERY_ADC_PIN 36
#define BATTERY_DIVIDER_RATIO 4.25f  // Vbatt = Vadc * ratio (390kΩ/120kΩ divider: (390+120)/120 = 4.25)

// Motor Control Constants
#define SOFTSTART_STEP 8
#define SOFTSTART_TARGET 128

#endif // PINS_CONFIG_H
