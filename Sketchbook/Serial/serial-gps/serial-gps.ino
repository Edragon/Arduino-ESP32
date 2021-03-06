// On ESP8266:
// At 80MHz runs up 57600ps, and at 160MHz CPU frequency up to 115200bps with only negligible errors.
// Connect pin 12 to 14.

// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>

// tested with SIM7020E, 9600bps, 

#define D5 (18)
#define D6 (19)
//#define D7 (23)
//#define D8 (5)
//#define TX (1)

#define BAUD_RATE 115200


// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail. 
SoftwareSerial swSer;

void setup() {
	Serial.begin(115200);
	swSer.begin(BAUD_RATE, SWSERIAL_8N1, D5, D6, false, 95, 11);

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
