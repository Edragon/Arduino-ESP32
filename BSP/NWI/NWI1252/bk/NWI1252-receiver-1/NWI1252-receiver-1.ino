// 9600 57600 works fine, 115200 not working


#include <SoftwareSerial.h>


#define SR_out_RX 4
#define SR_out_TX 5
#define BAUD_RATE 9600

#define PROG_LED 10


SoftwareSerial SR_out;

void setup() {
  pinMode(PROG_LED, OUTPUT);
  pinMode(PROG_LED, HIGH);
  
  Serial.begin(9600);
  SR_out.begin(BAUD_RATE, SWSERIAL_8N1, SR_out_RX, SR_out_TX, false, 95, 11);
}


void loop() {
  while (SR_out.available() > 0) {
    digitalWrite(PROG_LED, LOW);
    Serial.write(SR_out.read());
    // yield();
  }

  while (Serial.available() > 0) {
    SR_out.write(Serial.read());
    // yield();
  }

  digitalWrite(PROG_LED, HIGH);
}
