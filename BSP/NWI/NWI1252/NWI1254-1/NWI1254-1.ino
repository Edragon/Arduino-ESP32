// 9600 57600 works fine, 115200 not working


#include <SoftwareSerial.h>


#define RS232_out_RX 4
#define RS232_out_TX 5

#define RS485_out_RX 6
#define RS485_out_TX 7

#define BAUD_RATE 9600

#define PROG_LED 10


SoftwareSerial RS232_dat;
SoftwareSerial RS485_dat;

void setup() {
  pinMode(PROG_LED, OUTPUT);

  Serial.begin(9600);
  
  // Serial.setTxTimeoutMs(0);

  RS232_dat.begin(BAUD_RATE, SWSERIAL_8N1, RS232_out_RX, RS232_out_TX, false, 95, 11);
  RS485_dat.begin(BAUD_RATE, SWSERIAL_8N1, RS485_out_RX, RS485_out_TX, false, 95, 11);

  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 1");
  RS232_dat.println("\nSoftware serial test started 1");
  RS485_dat.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 2");
  RS232_dat.println("\nSoftware serial test started 2");
  RS485_dat.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 3");
  RS232_dat.println("\nSoftware serial test started 3");
  RS485_dat.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 4");
  RS232_dat.println("\nSoftware serial test started 4");
  RS485_dat.println("\nSoftware serial test started 1");
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
    RS232_dat.write(Serial.read());
    RS485_dat.write(Serial.read());
    // yield();
  }

}
