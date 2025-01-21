#include "I2S.h"

const int frequency = 440;    // frequency of square wave in Hz
const int amplitude = 500;    // amplitude of square wave
const int sampleRate = 8000;  // sample rate in Hz

//i2s_data_bit_width_t bps = I2S_DATA_BIT_WIDTH_16BIT;
//i2s_mode_t mode = I2S_MODE_STD;
//i2s_slot_mode_t slot = I2S_SLOT_MODE_STEREO;
//
const int halfWavelength = (sampleRate / frequency);  // half wavelength of square wave

int32_t sample = amplitude;  // current sample value
int count = 0;

void setup() {
  I2S_Init();

}

void loop() {
  if (count % halfWavelength == 0) {
    // invert the sample every half wavelength count multiple to generate square wave
    sample = -1 * sample;
  }

  I2S_Write(sample);  // Right channel
  I2S_Write(sample);  // Left channel

  // increment the counter for the next sample
  count++;
}
