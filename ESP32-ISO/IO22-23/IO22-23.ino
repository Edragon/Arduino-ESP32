
#include <SoftwareSerial.h>
SoftwareSerial swSer;

void setup() {
	Serial.begin(9600);

  // sSerial.begin(BAUD_RATE, RX_PIN, TX_PIN, SWSERIAL_8N1, false, 95, 11);
	swSer.begin(9600, SWSERIAL_8N1, 23, 22, false, 95, 11);

 
  delay(500);
  
	Serial.println("\nSoftware serial test started");
  swSer.println("AT\r\n");
  delay(500);
}

void loop() {
	while (swSer.available() > 0) {
		Serial.write(swSer.read());
		yield();
	}
	while (Serial.available() > 0) {
		swSer.write(Serial.read());
		yield();
	}

}
