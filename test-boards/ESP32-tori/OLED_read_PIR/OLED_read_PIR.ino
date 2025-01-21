
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

SSD1306Wire display(0x3c, 15, 13);  // this line cause the temp not reading properly
#define PIR 3

//#define flash 4
//#define onboard 33

void setup() {
  Serial.begin(115200);
  pinMode(PIR, INPUT_PULLDOWN);
  display.init();

  //pinMode(flash, OUTPUT);
  //pinMode(onboard, OUTPUT);
  //pinMode(PIR, INPUT);
  //pinMode(PIR2, INPUT);
  //analogReadResolution(12);
}


void loop() {
  //  display.clear();
  //  delay(100);
  display.resetDisplay();
  delay(100);
  display.displayOff();
  delay(100);
  display.displayOn();
  delay(100);
  
  //  delay(100);
  //  display.display();
  //  delay(100);
  //  display.clear();
  //  delay(100);

  int ValuePIR = digitalRead(PIR);
  Serial.printf("PIR Read = %d\n", ValuePIR);

  if (ValuePIR == HIGH) {
    display.clear();
    delay(100);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setColor(WHITE);
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 0, "FIND YOU !!");
    display.display();
    delay(1000);
    //    display.clear();
  } else {
    //    display.clear();
    //    delay(100);
    //    display.setFont(ArialMT_Plain_16);
    //    display.drawString(0, 0, "DEBUG !!");
    //    display.display();
  }

}
