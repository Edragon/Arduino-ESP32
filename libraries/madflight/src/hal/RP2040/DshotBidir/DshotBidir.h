#include "DshotBidirPio.h"

class DshotBidir {
private:
  DshotBidirPio dshot_impl;
public:
  bool setup( int* pins, uint8_t cnt, int freq_khz = 300) {
    //check pins
    for(int i = 0; i < cnt - 1; i++) {
      if(pins[i] + 1 != pins[i + 1]) {
        Serial.printf("OUT: ERROR dshot pins should be sequential");
        return false;
      }
    }
    
    if(!dshot_impl.begin(pins[0], cnt, freq_khz)) {
      Serial.printf("OUT: ERROR dshot init failed");
      return false;
    }
    return true;
  }

  void set_throttle( uint16_t* throttle) {
    dshot_impl.set_throttle(throttle);
  }

  //get erpm values, call before set_throttle. Returns negative values on error.
  void get_erpm( int* erpm) {
    dshot_impl.get_erpm(erpm);
  }
};
