#include "SharedGlobals.h"

// Shared control data (struct defined in ControlData.h)
// Initialize all fields to zero: throttle, steering, relayCh5, relayCh6, relayCh7, servoCh1, servoCh2, battery_mv
volatile ControlData control = {0, 0, 0, 0, 0, 0, 0, 0};

// Concurrency primitives
SemaphoreHandle_t controlMutex;
QueueHandle_t motorCmdQueue; // single-element queue carrying latest control snapshot to motor task

// Peripherals
HardwareSerial crsfSerial(1);
HardwareSerial modemSerial(2); // New serial for MQTT modem
AlfredoCRSF crsf;
Adafruit_NeoPixel ws2812(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
