#include <SoftwareSerial.h>


#define ESP32_TX_PIN 18
#define ESP32_RX_PIN 19

// can not reach 9600
//Create software serial object to communicate with SIM800
SoftwareSerial ser_ESP32(ESP32_TX_PIN, ESP32_RX_PIN);



void setup() {
  //Begin serial comunication with Arduino and Arduino IDE (Serial Monitor)
  Serial.begin(9600);
  while (!Serial);

  //Being serial communication witj Arduino and SIM800
  ser_ESP32.begin(9600);
  
  Serial.println("Setup Complete!");
}


void loop() {
  //relayTest();
  
  //Read SIM800 output (if available) and print it in Arduino IDE Serial Monitor
  if (ser_ESP32.available()) {
    Serial.write(ser_ESP32.read());
  }
  //Read Arduino IDE Serial Monitor inputs (if available) and send them to SIM800
  if (Serial.available()) {
    ser_ESP32.write(Serial.read());
  }
}
