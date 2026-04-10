/*
  ESP32 I2S Audio Test
  This code generates a simple sine wave to test the MAX98357A amplifier output.
*/

// Include I2S driver
#include <driver/i2s.h>
#include <math.h>

// Connections to MAX98357A amplifier
#define I2S_DOUT  8
#define I2S_BCLK  7
#define I2S_LRC   6

// Use I2S Processor 0
#define I2S_PORT I2S_NUM_0

// Sample rate and wave parameters
const int sampleRate = 44100;
const float frequency = 440.0; // Frequency of the sine wave (A4 note)
const int amplitude = 3000;    // Amplitude of the sine wave

void i2s_install() {
  // Set up I2S Processor configuration for TX only
  const i2s_config_t i2s_config = {
    .mode = i2s_mode_t(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = sampleRate,
    .bits_per_sample = i2s_bits_per_sample_t(16),
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = i2s_comm_format_t(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
}

void i2s_setpin() {
  // Set I2S pin configuration for output (amp)
  const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_set_pin(I2S_PORT, &pin_config);
}

void setup() {
  // Set up Serial Monitor
  Serial.begin(115200);
  Serial.println("Generating sine wave on MAX98357A amplifier");

  // Set up I2S
  i2s_install();
  i2s_setpin();
  i2s_start(I2S_PORT);
}

void loop() {
  // Buffer to hold the audio data
  int16_t buffer[64];

  // Generate sine wave and write it to I2S buffer
  for (int i = 0; i < 64; i++) {
    float sample = sinf(2.0f * M_PI * frequency * i / sampleRate);
    buffer[i] = (int16_t)(sample * amplitude); // Convert float to 16-bit integer
  }

  // Send the buffer to the I2S amplifier
  size_t bytesWritten;
  i2s_write(I2S_PORT, &buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
}
