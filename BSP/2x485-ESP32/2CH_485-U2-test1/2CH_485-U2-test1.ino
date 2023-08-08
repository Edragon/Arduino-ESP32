#include <SoftwareSerial.h>

#define UART1_TX 17
#define UART1_RX 16
#define UART1_CTRL 21

#define UART2_TX 23
#define UART2_RX 22
#define UART2_CTRL 20

#define SIM_TX 18
#define SIM_RX 19

SoftwareSerial UART1(UART1_RX, UART1_TX);
SoftwareSerial UART2(UART2_RX, UART2_TX);
SoftwareSerial SIM(SIM_RX, SIM_TX);

void setup() {
  Serial.begin(9600);
  while (!Serial);

  UART1.begin(9600);
  UART2.begin(9600);
  SIM.begin(9600); // software serial can only reach 9600
  delay(500);

  Serial.println("\nUART0 Setup Complete!");
  UART1.println("UART1 Setup Complete!");
  UART2.println("UART2 Setup Complete!");
  
  SIM.println("AT");
  delay(500);
  
}

void loop () {
  UART2.println("UART2 Test Loop ! ");
  delay(500);
}


void loop2 () {
  if (UART2.available()) {
    Serial.write(UART2.read());
  }
  if (Serial.available()) {
    UART2.write(Serial.read());
  }
}
