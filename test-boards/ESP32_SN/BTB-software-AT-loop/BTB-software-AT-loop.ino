// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>

// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail. 

SoftwareSerial swSer;

#define bootUp_pin 27

void setup() {

  pinMode(bootUp_pin, OUTPUT);
  
	Serial.begin(9600);
	swSer.begin(9600, SWSERIAL_8N1, 25, 26, false, 95, 11);

	Serial.println("\nSoftware serial test started");

	//for (char ch = ' '; ch <= 'z'; ch++) {
	//	swSer.write(ch);
	//}
	//swSer.println("");
  
  digitalWrite(bootUp_pin, LOW);
  delay(2000);
  digitalWrite(bootUp_pin, HIGH);
  
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
