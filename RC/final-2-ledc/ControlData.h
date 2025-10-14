#ifndef CONTROL_DATA_H
#define CONTROL_DATA_H

#include <stdint.h>

// Shared data structure for RTOS tasks
typedef struct {

  uint16_t throttle;
  uint16_t steering;

  uint16_t relayCh5;     // Relay channel 5 (unused in new mapping)
  uint16_t relayCh6;     // Relay channel 6 (was used previously)
  uint16_t relayCh7;     // Relay channel 7 (new)

  uint16_t servoCh1;     // Servo channel 6 (now used for servo 1)
  uint16_t servoCh2;     // Servo channel 7 (now used for servo 2)


  uint16_t battery_mv;
} ControlData;

#endif // CONTROL_DATA_H
