#include <driver/i2s.h>

// I2S configuration
#define I2S_NUM I2S_NUM_0
#define SAMPLE_RATE 44100  // Sampling frequency (44.1 kHz)
#define I2S_BCLK 5        // Bit Clock
#define I2S_LRC 25         // Word Select (Left/Right Clock)
#define I2S_DOUT 26        // Data Out (DIN on MAX98357)

// Sine wave parameters
const int frequency = 440;  // Frequency of the tone (A4 note, 440 Hz)
const int amplitude = 5000; // Amplitude of the sine wave (max = 32767)

// Buffer for one cycle of the sine wave
int16_t sine_wave[100];

void setup() {
  Serial.begin(115200);
  
  // Configure I2S
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = 0, // Default interrupt priority
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Initialize I2S
  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
  i2s_zero_dma_buffer(I2S_NUM);

  // Generate the sine wave buffer
  generateSineWave();
}

void loop() {
  // Play the sine wave
  size_t bytes_written;
  i2s_write(I2S_NUM, sine_wave, sizeof(sine_wave), &bytes_written, portMAX_DELAY);
}

// Function to generate a sine wave buffer
void generateSineWave() {
  for (int i = 0; i < 100; i++) {
    float angle = (2.0 * M_PI * i) / 100; // Angle for each sample
    sine_wave[i] = (int16_t)(amplitude * sin(angle)); // Scale to amplitude
  }
}
