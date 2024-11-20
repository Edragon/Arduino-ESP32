// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>

// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail.

#define U1_RX 21
#define U1_TX 22

#define U2_RX 16
#define U2_TX 17


SoftwareSerial U1 (U1_RX, U1_TX);
SoftwareSerial U2 (U2_RX, U2_TX);   // rx tx 

#define activeUART U2

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("\nSoftware serial test started");
  delay(500);
  
  //U1.begin(9600, SWSERIAL_8N1, U1R, U1T, false, 95, 11);
  activeUART.begin(9600);
  delay(500);

}



void loop() {
  if (activeUART.available()) {
    Serial.write(activeUART.read());
    yield();
  }

  if (Serial.available()) { // note
    activeUART.write(Serial.read());
    yield();
  }

}
