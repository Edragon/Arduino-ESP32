#include <driver/i2s.h>
#include <math.h>

#define I2S_PORT I2S_NUM_0

// MAX98357
#define MAX_DIN 36   
#define MAX_LRC 47   
#define MAX_BCLK 35  

// Audio parameters
#define SAMPLE_RATE 44100
#define TONE_FREQ 1000
#define AMPLITUDE 10000  // Max 32767

// Channels
#define LEFT false
#define RIGHT true

// Buffer size (frames, not samples)
#define BUFFER_LEN 256

#define LOOP_DEBUG_INTERVAL_MS 1000

// Stereo buffer: Left, Right
int16_t samples[BUFFER_LEN * 2];

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println();
  Serial.println("MAX98357 ESP32-S3 tone test start");
  Serial.printf("Pins: DIN=%d, LRC=%d, BCLK=%d\n", MAX_DIN, MAX_LRC, MAX_BCLK);
  Serial.printf("Audio: sample_rate=%d, tone=%d Hz, amplitude=%d\n", SAMPLE_RATE, TONE_FREQ, AMPLITUDE);
  Serial.printf("Buffer: %d frames, %d bytes\n", BUFFER_LEN, sizeof(samples));
  Serial.printf("Channels: left=%s, right=%s\n", LEFT ? "tone" : "mute", RIGHT ? "tone" : "mute");

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

  Serial.println("Installing I2S driver...");
  esp_err_t install_result = i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);
  Serial.printf("i2s_driver_install() -> %d\n", install_result);
  if (install_result != ESP_OK) {
    Serial.println("I2S driver install failed. Stop here.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("Applying I2S pin configuration...");
  esp_err_t pin_result = i2s_set_pin(I2S_PORT, &pin_config);
  Serial.printf("i2s_set_pin() -> %d\n", pin_result);
  if (pin_result != ESP_OK) {
    Serial.println("I2S pin setup failed. Stop here.");
    while (true) {
      delay(1000);
    }
  }

  Serial.println("I2S init done. Starting tone output.");
}

void loop() {
  static float phase = 0.0;
  static uint32_t loop_count = 0;
  static uint32_t last_debug_ms = 0;
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
  esp_err_t write_result = i2s_write(I2S_PORT, samples, sizeof(samples), &bytes_written, portMAX_DELAY);

  loop_count++;
  uint32_t now = millis();
  if (now - last_debug_ms >= LOOP_DEBUG_INTERVAL_MS) {
    Serial.printf(
      "loop=%lu, write_result=%d, bytes_written=%u, firstL=%d, firstR=%d, phase=%.4f\n",
      (unsigned long)loop_count,
      write_result,
      (unsigned int)bytes_written,
      samples[0],
      samples[1],
      phase
    );
    last_debug_ms = now;
  }
}