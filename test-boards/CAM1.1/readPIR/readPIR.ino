
#define flash 4
#define onboard 33

#define PIR 3

//#define PIR 16 // not working on GPIO16, pulled up 
//#define PIR2 0


void setup() {
  Serial.begin(115200);
  
  pinMode(flash, OUTPUT);
  pinMode(onboard, OUTPUT);
  
  //pinMode(PIR, INPUT);
  pinMode(PIR, INPUT_PULLDOWN);
  //pinMode(PIR2, INPUT);
  
  analogReadResolution(12);
  delay(100);
}


void loop() {

  int ValuePIR = digitalRead(PIR);
  Serial.printf("ADC analog value = %d\n", ValuePIR);
  
  //if (ValuePIR >= 4000) {
  if (ValuePIR == HIGH) {  
    digitalWrite(flash, HIGH);
    delay(2000); // make a photo for 2 seconds freeze
  } else {
    // turn LED off:
    digitalWrite(flash, LOW);
  }
  delay(100);        // delay in between reads for stability

  
}
