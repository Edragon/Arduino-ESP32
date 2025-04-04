#include "Arduino.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

#define I2S_DOUT      26  // I2S Data Output
#define I2S_BCLK      5  // I2S Bit Clock
#define I2S_LRC       25  // I2S Left-Right Clock

#define SD_CS         13   // SD card chip select
#define SD_MOSI       15  // Custom MOSI pin
#define SD_MISO       02  // Custom MISO pin
#define SD_SCK        14  // Custom SCK pin

#define SAMPLE_RATE   44100
#define BUFFER_SIZE   1024

void setupI2S() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = BUFFER_SIZE,
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
    
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
}

void playWav(const char *filename) {
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Failed to open file");
        return;
    }
    
    byte buffer[BUFFER_SIZE];
    file.seek(44); // Skip WAV header
    while (file.available()) {
        size_t bytesRead = file.read(buffer, BUFFER_SIZE);
        size_t bytesWritten;
        i2s_write(I2S_NUM_0, buffer, bytesRead, &bytesWritten, portMAX_DELAY);
    }
    
    file.close();
    Serial.println("Playback finished");
}

void setup() {
    Serial.begin(115200);
    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    if (!SD.begin(SD_CS)) {
        Serial.println("SD card initialization failed!");
        return;
    }
    Serial.println("SD card initialized");
    setupI2S();
    playWav("/test.wav");
}

void loop() {
}
