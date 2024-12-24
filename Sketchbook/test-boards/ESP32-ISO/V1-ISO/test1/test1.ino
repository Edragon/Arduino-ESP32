#include <SoftwareSerial.h>

#define UART1_TX 23
#define UART1_RX 22

#define UART2_TX 20
#define UART2_RX 21

#define debug_LED 0

SoftwareSerial UART1(UART1_RX, UART1_TX);
SoftwareSerial UART2(UART2_RX, UART2_TX);

void setup() {
  pinMode(debug_LED, OUTPUT);
  digitalWrite(debug_LED, HIGH);
  Serial.begin(9600);
  while (!Serial);

  UART1.begin(9600);
  UART2.begin(9600);
  delay(500);

  Serial.println("\nUART0 Setup Complete!");
  UART1.println("UART1 Setup Complete!");
  UART2.println("UART2 Setup Complete!");
  delay(500);
}

void loop() {
  if (UART1.available()) {
    digitalWrite(debug_LED, LOW);
    Serial.write(UART1.read());
  }
  
  if (UART2.available()) {
    digitalWrite(debug_LED, LOW);
    Serial.write(UART2.read());
  }
  
  if (Serial.available()) { // note 
    digitalWrite(debug_LED, HIGH);
    UART1.write(Serial.read()); // write serial first byte 
    UART2.write(Serial.read()); // write serial second byte 
  }


  
}
