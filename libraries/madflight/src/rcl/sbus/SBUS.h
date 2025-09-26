#pragma once

#include "Arduino.h"
#include "elapsedMillis.h"
#include "../../hal/MF_Serial.h"

class SBUS{
  public:
    MF_Serial* serial;

    void begin();
    int read(uint16_t* channels, bool* failsafe, bool* lostFrame); //returns number of packets received

  private:
    bool parse();
    uint8_t _parserState = 0;
    uint8_t _prevByte = 0xFF;
    uint8_t _payload[24];
};
