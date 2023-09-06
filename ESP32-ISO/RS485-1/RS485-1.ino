
#include <SoftwareSerial.h>
SoftwareSerial RS485;

void setup() {
	Serial.begin(9600);

  // sSerial.begin(BAUD_RATE, RX_PIN, TX_PIN, SWSERIAL_8N1, false, 95, 11);
	RS485.begin(9600, SWSERIAL_8N1, 18, 19, false, 95, 11);
  delay(500);
  
	Serial.println("\nSoftware serial test started");
  RS485.println("RS485\r\n");
  delay(500);
}


void loop() {
	while (RS485.available() > 0) {
		Serial.write(RS485.read());
		yield();
	}
	while (Serial.available() > 0) {
		RS485.write(Serial.read());
		yield();
	}

}
