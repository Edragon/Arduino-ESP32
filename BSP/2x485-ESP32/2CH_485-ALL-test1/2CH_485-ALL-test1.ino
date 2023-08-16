#include <SoftwareSerial.h>

#define UART1_TX 17
#define UART1_RX 16
#define UART1_CTRL 21 // not use

#define UART2_TX 23
#define UART2_RX 22
#define UART2_CTRL 20 // not use

#define SIM_TX 18
#define SIM_RX 19

#define debug_LED 4

SoftwareSerial UART1(UART1_RX, UART1_TX);
SoftwareSerial UART2(UART2_RX, UART2_TX);
SoftwareSerial SIM(SIM_RX, SIM_TX);

void setup() {
  pinMode(debug_LED, OUTPUT);
  digitalWrite(debug_LED, HIGH);
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
  
  // auto TX/RX set by hardware
}

// 一个网口电源输入 一个网口信号测试
// one of RJ45 port for power supply test, another for communication test

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
