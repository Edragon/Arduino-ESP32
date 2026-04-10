#include <ESP_I2S.h>

namespace {

constexpr int kPdmClkPin = 46;
constexpr int kPdmDataPin = 45;
constexpr int kPdmWsPin = 0;  // PDM RX does not use WS; kept here to match the requested wiring list.

constexpr uint32_t kSampleRate = 16000;
constexpr size_t kSamplesPerRead = 256;
constexpr size_t kSerialDecimation = 16;

I2SClass i2s;
int16_t sampleBuffer[kSamplesPerRead];

void printPinSummary() {
	Serial.println();
	Serial.println("ESP32-S3 PDM microphone demo");
	Serial.printf("PDM CLK : IO%d\n", kPdmClkPin);
	Serial.printf("PDM DATA: IO%d\n", kPdmDataPin);
	Serial.printf("WS label: IO%d (not used by PDM RX mode)\n", kPdmWsPin);
	Serial.println("Streaming decimated PCM samples as CSV over Serial.");
	Serial.println("If your module drives IO0 low at boot, the ESP32-S3 can fail to start.");
	Serial.println();
}

bool beginPdmMic() {
	i2s.setPinsPdmRx(kPdmClkPin, kPdmDataPin);

	if (!i2s.begin(I2S_MODE_PDM_RX, kSampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO)) {
		Serial.printf("I2S PDM RX init failed, error=%d\n", i2s.lastError());
		return false;
	}

	return true;
}

void printSamplesCsv(const int16_t *samples, size_t count) {
	bool firstValue = true;

	for (size_t index = 0; index < count; index += kSerialDecimation) {
		if (!firstValue) {
			Serial.print(',');
		}
		Serial.print(samples[index]);
		firstValue = false;
	}

	Serial.println();
}

}  // namespace

void setup() {
	Serial.begin(115200);
	delay(1200);

	printPinSummary();

	if (!beginPdmMic()) {
		Serial.println("Check the microphone power, ground, and pin routing.");
		while (true) {
			delay(1000);
		}
	}
}

void loop() {
	const size_t bytesRead = i2s.readBytes(reinterpret_cast<char *>(sampleBuffer), sizeof(sampleBuffer));

	if (bytesRead == 0) {
		Serial.println("No microphone data received.");
		delay(100);
		return;
	}

	const size_t samplesRead = bytesRead / sizeof(sampleBuffer[0]);
	printSamplesCsv(sampleBuffer, samplesRead);
}
