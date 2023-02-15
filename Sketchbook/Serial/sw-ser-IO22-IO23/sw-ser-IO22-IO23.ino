// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>

// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail. 

SoftwareSerial swSer;

void setup() {
	Serial.begin(9600);
	swSer.begin(9600, SWSERIAL_8N1, 18, 19, false, 95, 11);

	Serial.println("\nSoftware serial test started");

	//for (char ch = ' '; ch <= 'z'; ch++) {
	//	swSer.write(ch);
	//}
	//swSer.println("");

  swSer.println("AT\r\n");
  
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
