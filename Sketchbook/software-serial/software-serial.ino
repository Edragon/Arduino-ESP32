
#include <SoftwareSerial.h>


SoftwareSerial mySerial(2, 3); // RX, TX

void setup() {
  Serial.begin(115200);
  mySerial.begin("ESP32test"); //Bluetooth device name
  Serial.println("ESP32test!");
}

void loop() {
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
  if (mySerial.available()) {
    Serial.write(SerialBT.read());
  }
  delay(20);
}
