
#include <SoftwareSerial.h>
SoftwareSerial RS232;

void setup() {
	Serial.begin(9600);

  // sSerial.begin(BAUD_RATE, RX_PIN, TX_PIN, SWSERIAL_8N1, false, 95, 11);
	RS232.begin(9600, SWSERIAL_8N1, 23, 22, false, 95, 11);

 
  delay(500);
  
	Serial.println("\nSoftware serial test started");
  RS232.println("AT\r\n");
  delay(500);
}

void loop() {
	while (RS232.available() > 0) {
		Serial.write(RS232.read());
		yield();
	}
	while (Serial.available() > 0) {
		RS232.write(Serial.read());
		yield();
	}

}
