
#include <SoftwareSerial.h>


SoftwareSerial mySerial(25, 26); // RX, TX

void setup() {
  Serial.begin(57600);
  mySerial.begin(57600); 
  
  mySerial.println("ESP32test!");
  Serial.println("ESP32test!");
}

void loop() {
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
  if (mySerial.available()) {
    Serial.write(mySerial.read());
  }
  delay(20);
}
