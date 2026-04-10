#include <ESP_I2S.h>
#include <math.h>

static constexpr int I2S_BCLK_PIN = 35;
static constexpr int I2S_LRCLK_PIN = 47;
static constexpr int I2S_DOUT_PIN = 36;

static constexpr uint32_t SAMPLE_RATE = 44100;
static constexpr i2s_data_bit_width_t BITS_PER_SAMPLE = I2S_DATA_BIT_WIDTH_16BIT;
static constexpr i2s_mode_t I2S_MODE = I2S_MODE_STD;
static constexpr i2s_slot_mode_t SLOT_MODE = I2S_SLOT_MODE_STEREO;

static constexpr float TONE_HZ = 440.0f;
static constexpr int16_t AMPLITUDE = 14000;
static constexpr size_t BUFFER_SAMPLES = 256;

I2SClass i2s;

float phase = 0.0f;

static int16_t nextSample() {
	const float phaseStep = 2.0f * PI * TONE_HZ / SAMPLE_RATE;

	phase += phaseStep;
	if (phase >= 2.0f * PI) {
		phase -= 2.0f * PI;
	}

	return static_cast<int16_t>(sinf(phase) * AMPLITUDE);
}

void setup() {
	Serial.begin(115200);
	delay(200);
	Serial.println("ESP32-S3 MAX98357A simple tone");
	Serial.println("LRCLK=47, BCLK=35, DIN=36, stereo duplicated");

	i2s.setPins(I2S_BCLK_PIN, I2S_LRCLK_PIN, I2S_DOUT_PIN);

	if (!i2s.begin(I2S_MODE, SAMPLE_RATE, BITS_PER_SAMPLE, SLOT_MODE)) {
		Serial.println("Failed to initialize ESP_I2S");
		while (true) {
			delay(1000);
		}
	}
}

void loop() {
	int16_t buffer[BUFFER_SAMPLES * 2];

	for (size_t index = 0; index < BUFFER_SAMPLES; ++index) {
		const int16_t sample = nextSample();
		buffer[index * 2] = sample;
		buffer[index * 2 + 1] = sample;
	}

	i2s.write(reinterpret_cast<const uint8_t *>(buffer), sizeof(buffer));
}
