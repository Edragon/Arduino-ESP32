// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>


#define RS485_RX 16
#define RS485_TX 17

#define CAN_RX 22
#define CAN_TX 23

#define SW1_CTRL 12
#define SW1_Status 13

#define SW2_CTRL 19 // not solder 
#define SW2_Status 18

#define PROG_LED 5
#define PWR_ADC 33

#define BAUD_RATE 9600

SoftwareSerial sCAN;
// SoftwareSerial RS485;

void setup() {
  Serial.begin(9600);
  sCAN.begin(BAUD_RATE, SWSERIAL_8N1, CAN_RX, CAN_TX, false, 95, 11);
  pinMode(15, OUTPUT);

  Serial.println(" receiver mode CAN B /r/n");
  digitalWrite(15, HIGH);
  delay(1000);

}

void loop() {
    while (sCAN.available() > 0) {
    Serial.write(sCAN.read());
    yield();
  }
  while (Serial.available() > 0) {
    sCAN.write(Serial.read());
    yield();
  }

}
