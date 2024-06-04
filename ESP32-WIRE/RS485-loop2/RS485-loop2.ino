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

#define PROG_LED 15
#define PWR_ADC 33

#define BAUD_RATE 9600

SoftwareSerial RS485;

void setup()
{
  pinMode(15, OUTPUT);

  Serial.begin(9600);

  RS485.begin(BAUD_RATE, SWSERIAL_8N1, RS485_RX, RS485_TX, false, 95, 11);

}

void loop()
{
  delay(400);
  digitalWrite(15, HIGH);
  RS485.println("\nSoftware serial test started 1");

  delay(400);
  digitalWrite(15, LOW);
  RS485.println("\nSoftware serial test started 2");

  delay(400);
  digitalWrite(15, HIGH);
  RS485.println("\nSoftware serial test started 3");

  delay(400);
  digitalWrite(15, LOW);
  RS485.println("\nSoftware serial test started 4");
}

// void loop() {
//   while (RS485.available() > 0) {
//     Serial.write(RS485.read());
//     yield();
//   }
//   while (Serial.available() > 0) {
//     RS485.write(Serial.read());
//     yield();
//   }

// }
