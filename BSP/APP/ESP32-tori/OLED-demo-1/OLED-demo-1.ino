
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

SSD1306Wire display(0x3c, 15, 13);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  display.init();
}

void loop() {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 20, "Hello world");
  
  display.display();
  delay(10);
}
