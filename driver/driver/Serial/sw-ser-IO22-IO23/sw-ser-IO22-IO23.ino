// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>

// Reminder: the buffer size optimizations here, in particular the isrBufSize that only accommodates
// a single 8N1 word, are on the basis that any char written to the loopback SoftwareSerial adapter gets read
// before another write is performed. Block writes with a size greater than 1 would usually fail.

#define UART1_RX 21
#define UART1_TX 22

SoftwareSerial UART1 (UART1_RX, UART1_TX);


void setup() {
  Serial.begin(9600);
  delay(500);
  //U1.begin(9600, SWSERIAL_8N1, U1R, U1T, false, 95, 11);
  UART1.begin(9600);
  delay(500);
  Serial.println("\nSoftware serial test started");
  delay(500);
  UART1.println("AT\r\n");
  delay(500);
}

void loop() {
  if (UART1.available()) {
    Serial.write(UART1.read());
    yield();
  }

  if (Serial.available()) { // note
    UART1.write(Serial.read());
    yield();
  }

}
