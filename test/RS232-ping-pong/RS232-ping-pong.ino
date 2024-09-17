// 9600 57600 works fine, 115200 not working
#include <SoftwareSerial.h>


#define RS232_RX 4
#define RS232_TX 5
#define BAUD_RATE 9600

#define PROG_LED 10


SoftwareSerial RS232;

void setup() {
  pinMode(PROG_LED, OUTPUT);

  Serial.begin(9600);
  
  // Serial.setTxTimeoutMs(0);

  RS232.begin(BAUD_RATE, SWSERIAL_8N1, RS232_RX, RS232_TX, false, 95, 11);

  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 1");
  RS232.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 2");
  RS232.println("\nSoftware serial test started 2");
  
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 3");
  RS232.println("\nSoftware serial test started 3");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 4");
  RS232.println("\nSoftware serial test started 4");
  
  // delay(1000);
  // RS485.println("AT\r\n");
  
}


void loop() {
  while (RS232.available() > 0) {
    Serial.write(RS232.read());
    // yield();
  }
  while (Serial.available() > 0) {
    RS232.write(Serial.read());
    // yield();
  }

}
