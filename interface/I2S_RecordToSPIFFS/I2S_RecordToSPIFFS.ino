/*
  ESP32 I2S Microphone Recording to SPIFFS Example

  This sketch records 5 seconds of audio from an I2S microphone (e.g., INMP441)
  and saves it as a WAV file to the SPIFFS (SPI Flash File System).

  This version moves the recording logic to the main loop to avoid
  blocking the setup() function and to prevent watchdog timer issues.

  Connections to INMP441 I2S microphone:
  - SCK: GPIO 12
  - WS:  GPIO 2
  - SD:  GPIO 14
  - VDD: 3.3V
  - GND: GND
*/

#include <driver/i2s.h>
#include "FS.h"
#include "SPIFFS.h"

// I2S Connections
#define I2S_WS    2
#define I2S_SD    14
#define I2S_SCK   12
#define I2S_PORT  I2S_NUM_0

// Recording parameters
#define SAMPLE_RATE 16000 // Switched to 16kHz. 44.1kHz is high for SPIFFS writing.
#define BITS_PER_SAMPLE 16
#define RECORD_DURATION 5 // seconds
const char* FILE_PATH = "/recording.wav";

// WAV file header structure
typedef struct {
    char     riff[4];           // "RIFF"
    uint32_t chunkSize;         // File size - 8
    char     wave[4];           // "WAVE"
    char     fmt[4];            // "fmt "
    uint32_t subchunk1Size;     // 16 for PCM
    uint16_t audioFormat;       // 1 for PCM
    uint16_t numChannels;       // 1 for Mono
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char     data[4];           // "data"
    uint32_t subchunk2Size;     // Data size
} wav_header_t;

// Function to write WAV header
void writeWavHeader(File file, uint32_t sampleRate, uint16_t bitsPerSample, uint32_t dataSize) {
    wav_header_t header;
    strncpy(header.riff, "RIFF", 4);
    header.chunkSize = dataSize + sizeof(wav_header_t) - 8;
    strncpy(header.wave, "WAVE", 4);
    strncpy(header.fmt, "fmt ", 4);
    header.subchunk1Size = 16;
    header.audioFormat = 1;
    header.numChannels = 1;
    header.sampleRate = sampleRate;
    header.bitsPerSample = bitsPerSample;
    header.byteRate = sampleRate * header.numChannels * (bitsPerSample / 8);
    header.blockAlign = header.numChannels * (bitsPerSample / 8);
    strncpy(header.data, "data", 4);
    header.subchunk2Size = dataSize;

    file.write((const byte*)&header, sizeof(header));
}

// State machine for recording
enum RecordingState {
    IDLE,
    START_RECORDING,
    RECORDING,
    FINISH_RECORDING
};
RecordingState currentState = IDLE;

File file;
uint32_t recordingStartTime = 0;
uint32_t totalBytesWritten = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("I2S Microphone Recording to SPIFFS");

    // 1. Initialize SPIFFS
    if (!SPIFFS.begin(true)) {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    Serial.println("SPIFFS mounted successfully.");

    // 2. Configure and install I2S driver
    const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
        .intr_alloc_flags = 0,
        .dma_buf_count = 8,
        .dma_buf_len = 1024,
        .use_apll = false
    };
    i2s_driver_install(I2S_PORT, &i2s_config, 0, NULL);

    // 3. Set I2S pin configuration
    const i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };
    i2s_set_pin(I2S_PORT, &pin_config);
    Serial.println("I2S driver installed.");

    // Start recording immediately
    currentState = START_RECORDING;
}

void loop() {
    // Declare variables outside switch statement to avoid "crosses initialization" error
    const int bufferSize = 1024;
    uint8_t buffer[bufferSize];
    size_t bytesRead;
    esp_err_t result;

    switch (currentState) {
        case IDLE:
            // Do nothing, or wait for a trigger to start again
            delay(100);
            break;

        case START_RECORDING:
            // 4. Open file for writing
            file = SPIFFS.open(FILE_PATH, FILE_WRITE);
            if (!file) {
                Serial.println("Failed to open file for writing.");
                currentState = IDLE; // Go back to idle if file fails to open
                return;
            }

            // 5. Write a placeholder for the WAV header
            wav_header_t header_placeholder;
            file.write((const byte*)&header_placeholder, sizeof(header_placeholder));
            
            Serial.println("Recording for 5 seconds...");
            recordingStartTime = millis();
            totalBytesWritten = 0;
            currentState = RECORDING;
            break;

        case RECORDING:
            // 6. Record audio
            // Read data from I2S, with a small timeout to prevent blocking forever
            result = i2s_read(I2S_PORT, buffer, bufferSize, &bytesRead, 100 / portTICK_PERIOD_MS);
            
            if (result == ESP_OK && bytesRead > 0) {
                file.write(buffer, bytesRead);
                totalBytesWritten += bytesRead;
            }

            // Check if recording duration has passed
            if (millis() - recordingStartTime >= (RECORD_DURATION * 1000)) {
                currentState = FINISH_RECORDING;
            }
            break;

        case FINISH_RECORDING:
            Serial.printf("Recording finished. Total bytes written: %u\n", totalBytesWritten);

            // 7. Go back to the beginning of the file to write the final header
            file.seek(0);
            writeWavHeader(file, SAMPLE_RATE, BITS_PER_SAMPLE, totalBytesWritten);
            
            // 8. Close the file
            file.close();
            Serial.printf("Audio saved to %s\n", FILE_PATH);
            
            currentState = IDLE; // Go to idle state
            break;
    }
}
