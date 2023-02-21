#include <SoftwareSerial.h>

#define UART1_TX 17
#define UART1_RX 16
#define UART1_CTRL 21

#define UART2_TX 23
#define UART2_RX 22
#define UART2_CTRL 20

#define SIM_TX 19
#define SIM_RX 18

SoftwareSerial UART1(UART1_RX, UART1_TX);
SoftwareSerial UART2(UART2_RX, UART2_TX);
SoftwareSerial SIM(SIM_RX, SIM_TX);

void setup() {

  pinMode(UART1_CTRL, OUTPUT);
  
  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  while (!Serial);

  //Being serial communication witj Arduino and SIM800
  UART2.begin(9600);
  UART1.begin(9600);
  SIM.begin(9600);

  delay(500);
  digitalWrite(UART1_CTRL, HIGH);
  Serial.println("\nUART0 Setup Complete!");
  
  UART1.println("UART1 Setup Complete!");
  UART2.println("UART2 Setup Complete!");
  SIM.println("AT\r\n");

  delay(500);
  
  // receiver mode 
  digitalWrite(UART1_CTRL, LOW);
  
}



void loop() {
  //relayTest();
  
  //Read SIM800 output (if available) and print it in Arduino IDE Serial Monitor
  if (SIM.available()) {
    Serial.write(SIM.read());
  }
  
  //Read Arduino IDE Serial Monitor inputs (if available) and send them to SIM800
  if (Serial.available()) {
    SIM.write(Serial.read());
  }
}
