// 9600 57600 works fine, 115200 not working


#include <SoftwareSerial.h>


#define SR_out_RX 4
#define SR_out_TX 5
#define BAUD_RATE 9600

#define PROG_LED 10


SoftwareSerial SR_out;

void setup() {
  pinMode(PROG_LED, OUTPUT);

  Serial.begin(9600);
  
  // Serial.setTxTimeoutMs(0);

  SR_out.begin(BAUD_RATE, SWSERIAL_8N1, SR_out_RX, SR_out_TX, false, 95, 11);

  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 1");
  SR_out.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 2");
  SR_out.println("\nSoftware serial test started 2");
  
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  Serial.println("\nSoftware serial test started 3");
  SR_out.println("\nSoftware serial test started 3");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  Serial.println("\nSoftware serial test started 4");
  SR_out.println("\nSoftware serial test started 4");
  
  // delay(1000);
  // RS485.println("AT\r\n");
  
}


void loop() {
  while (SR_out.available() > 0) {
    Serial.write(SR_out.read());
    // yield();
  }
  while (Serial.available() > 0) {
    SR_out.write(Serial.read());
    // yield();
  }

}
