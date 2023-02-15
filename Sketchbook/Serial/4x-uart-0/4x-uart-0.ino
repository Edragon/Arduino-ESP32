#include <SoftwareSerial.h>

#define UART1_TX 17
#define UART1_RX 16

#define UART2_TX 23
#define UART2_RX 22

#define SIM_TX 18
#define SIM_RX 19


SoftwareSerial UART1(UART1_RX, UART1_TX);
SoftwareSerial UART2(UART2_RX, UART2_TX);
SoftwareSerial SIM(SIM_RX, SIM_TX);

void setup() {
  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  while (!Serial);

  //Being serial communication witj Arduino and SIM800
  UART2.begin(9600);
  SIM.begin(9600);
  
  Serial.println("\r\nSetup Complete!");
  UART2.println("\r\nSetup UART2 Complete!");
  delay(1000);
  pinMode(4, OUTPUT);
  
}

void loop() {
  digitalWrite(4, HIGH);
  UART2.println("Setup UART2 Complete! x2");
  delay(500);

  UART2.println("Setup UART2 Complete! x2xy");
  digitalWrite(4, LOW);
  delay(500);
}
