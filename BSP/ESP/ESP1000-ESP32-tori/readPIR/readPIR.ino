
#include <Arduino.h>

//#define onboard 33

#define flash 4
#define PIR 3

void setup() {
  Serial.begin(115200);
  pinMode(flash, OUTPUT);
  
  pinMode(PIR, INPUT_PULLDOWN);
  // analogReadResolution(12);
  delay(100);
}


void loop() {

  int ValuePIR = digitalRead(PIR);
  Serial.print("PIR:  ");
  Serial.println(ValuePIR);
  
  if (ValuePIR == HIGH) {  
    digitalWrite(flash, HIGH);
    delay(500); // make a photo for 2 seconds freeze
  } else {
    // turn LED off:
    digitalWrite(flash, LOW);
  }
  delay(100);        // delay in between reads for stability
}
