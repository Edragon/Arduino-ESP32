#include <driver/i2s.h>

// I2S configuration
#define I2S_NUM I2S_NUM_0 // I2S port number
#define SAMPLE_RATE 16000 // Sampling rate in Hz
#define I2S_BCK_PIN 26    // Bit clock pin (SCK)
#define I2S_WS_PIN 25     // Word select pin (WS/LRCLK)
#define I2S_DATA_PIN 22   // Serial data pin (SD)

#define BUFFER_SIZE 1024 // Buffer size for audio data

void setup()
{
    Serial.begin(115200);

    // Configure I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX), // Master receive mode
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT, // 32-bit data
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo format
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1, // Interrupt level 1
        .dma_buf_count = 4,                       // Number of DMA buffers
        .dma_buf_len = BUFFER_SIZE                // DMA buffer size
    };

    // Configure I2S pins
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_DATA_PIN};

    // Install and start I2S
    i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM);

    Serial.println("I2S initialized!");
}

void loop()
{
    int32_t i2s_buffer[BUFFER_SIZE]; // Buffer for interleaved audio data
    size_t bytes_read;

    // Read data from I2S
    i2s_read(I2S_NUM, i2s_buffer, sizeof(i2s_buffer), &bytes_read, portMAX_DELAY);

    // Process interleaved data
    size_t samples_read = bytes_read / sizeof(int32_t);
    for (size_t i = 0; i < samples_read; i += 2)
    {
        int32_t left_channel = i2s_buffer[i];      // Left channel (Microphone 1)
        int32_t right_channel = i2s_buffer[i + 1]; // Right channel (Microphone 2)

        // Print the audio data for debugging
        Serial.print("Left: ");
        Serial.print(left_channel);
        Serial.print(" | Right: ");
        Serial.println(right_channel);
    }
}
