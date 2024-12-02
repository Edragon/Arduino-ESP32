// 9600 57600 works fine, 115200 not working


#include <SoftwareSerial.h>

#include "hal/uart_types.h"

#define RS232_out_RX 4
#define RS232_out_TX 5
SoftwareSerial RS232_dat;

#define RS485_out_RX 6
#define RS485_out_TX 7

// SoftwareSerial RS485_dat;

#define RS485_dat Serial1


#define PROG_LED 9

void setup() {
  pinMode(PROG_LED, OUTPUT);

  Serial.begin(9600);
  while (!Serial) {
    delay(10);
  }
  
  RS485_dat.begin(9600, SERIAL_8N1, RS485_out_RX, RS485_out_TX);
  while (!RS485_dat) {
    delay(10);
  }
  
  //Serial1.begin(9600, SERIAL_8N1, 6, 7);
  // Serial.setTxTimeoutMs(0);
  RS232_dat.begin(9600, SWSERIAL_8N1, RS232_out_RX, RS232_out_TX, false, 95, 11);

  digitalWrite(PROG_LED, LOW);
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  
  Serial.println("\nSerial test started 1");
  RS232_dat.println("\n RS232 Software serial test started 1");
  RS485_dat.println("\n RS485 Software serial test started 1");

  digitalWrite(PROG_LED, LOW);
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  
  Serial.println("\nSerial test started 2");
  RS232_dat.println("\n RS232 Software serial test started 2");
  RS485_dat.println("\n RS485 Software serial test started 2");

  digitalWrite(PROG_LED, LOW);
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  
  Serial.println("\nSerial test started 3");
  RS232_dat.println("\n RS232 Software serial test started 3");
  RS485_dat.println("\n RS485 Software serial test started 3");

  digitalWrite(PROG_LED, LOW);
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  
  Serial.println("\nSerial test started 4");
  RS232_dat.println("\n RS232 Software serial test started 4");
  RS485_dat.println("\n RS485 Software serial test started 4");

  // delay(1000);
  // RS485.println("AT\r\n");
}


void loop() {
  while (Serial.available() > 0) {
    RS485_dat.write(Serial.read());
    // yield();
  }
  
  while (RS485_dat.available() > 0) {
    Serial.write(RS485_dat.read());
    // yield();
  }
  
//  while (Serial1.available() > 0) {
//    Serial.write(Serial1.read());
//    // yield();
//  }

}
