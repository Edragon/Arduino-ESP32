// 9600 57600 works fine, 115200 not working


#include <SoftwareSerial.h>

#define RS232_out_RX 4
#define RS232_out_TX 5
SoftwareSerial RS232_dat;

//#define RS485_out_RX 6
//#define RS485_out_TX 7
//SoftwareSerial RS485_dat;

#define PROG_LED 9

void setup() {
  pinMode(PROG_LED, OUTPUT);

  Serial.begin(9600);
  Serial1.begin(9600);
  
  // Serial.setTxTimeoutMs(0);
  RS232_dat.begin(9600, SWSERIAL_8N1, RS232_out_RX, RS232_out_TX, false, 95, 11);
  RS485_dat.begin(9600, SWSERIAL_8N1, RS485_out_RX, RS485_out_TX, false, 95, 11);
  delay(200);

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
  while (RS232_dat.available() > 0) {
    Serial.write(RS232_dat.read());
    // yield();
  }
  while (RS485_dat.available() > 0) {
    Serial.write(RS485_dat.read());
    // yield();
  }

  while (Serial.available() > 0) {
    int k = Serial.read();
    RS485_dat.write(k);
    RS232_dat.write(k);
    // yield();
  }

}
