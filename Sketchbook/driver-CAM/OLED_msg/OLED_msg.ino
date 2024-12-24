
#include <Wire.h>
#include "SSD1306Wire.h"
SSD1306Wire display(0x3c, 4, 13);

#define PIR 14

#define BAT_LVL 12

String msg = "";
int msg2 = 0;

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  pinMode(PIR, INPUT);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_24);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
}

void dis() {
  display.clear();
  display.drawString(0, 0, msg);
  display.display();
}

void loop() {

  int buttonState = analogRead(PIR);
  msg = String(buttonState);
  Serial.println (buttonState);
  dis();
  delay(50);
  
//int sensorValue = analogRead( BAT_LVL );
// msg2 = (BAT_LVL -200) / 1000
//  
//  if (buttonState == HIGH) {
//    msg = "111";
//  } else {
//    msg = "000";
//  }
//  


}
