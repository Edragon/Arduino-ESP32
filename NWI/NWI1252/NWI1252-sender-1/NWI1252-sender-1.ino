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
  SR_out.begin(BAUD_RATE, SWSERIAL_8N1, SR_out_RX, SR_out_TX, false, 95, 11);

}


void loop() {
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  SR_out.println("\nSoftware serial test started 1");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  SR_out.println("\nSoftware serial test started 2");
  
  delay(200);
  digitalWrite(PROG_LED, HIGH);
  SR_out.println("\nSoftware serial test started 3");
  
  delay(200);
  digitalWrite(PROG_LED, LOW);
  SR_out.println("\nSoftware serial test started 4");
}
