#include <driver/i2s.h>
#include <math.h>

#define I2S_PORT I2S_NUM_0

// MAX98357
#define MAX_DIN 33   
#define MAX_LRC 32   
#define MAX_BCLK 25  

// Audio parameters
#define SAMPLE_RATE 44100
#define TONE_FREQ 1000
#define AMPLITUDE 10000  // Max 32767

// Channels
#define LEFT false
#define RIGHT true

// Buffer size (frames, not samples)
#define BUFFER_LEN 256

// Stereo buffer: Left, Right
int16_t samples[BUFFER_LEN * 2];

void setup() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUFFER_LEN,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = MAX_BCLK,
    .ws_io_num = MAX_LRC,
    .data_out_num = MAX_DIN,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_PORT, &pin_config);
}

void loop() {
  static float phase = 0.0;
  const float phaseIncrement = 2.0 * PI * TONE_FREQ / SAMPLE_RATE;

  for (int i = 0; i < BUFFER_LEN; i++) {
    int16_t sound = (int16_t)(AMPLITUDE * sin(phase));
    int16_t silence = 0;

    samples[2 * i] = (LEFT) ? sound : silence;       // Left channel
    samples[2 * i + 1] = (RIGHT) ? sound : silence;  // Right channel

    phase += phaseIncrement;
    if (phase >= 2.0 * PI) phase -= 2.0 * PI;
  }

  size_t bytes_written;
  i2s_write(I2S_PORT, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}