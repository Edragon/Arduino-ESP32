#define touch 4
#define relay1 0
#define relay2 1
#define LED 2
#define buzzer 8

void setup() {
  pinMode(touch, INPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(LED, OUTPUT);
}

void loop() {
  if (digitalRead(touch) == 0 ) {
    digitalWrite(buzzer, HIGH);
    digitalWrite(relay1, HIGH);
    digitalWrite(relay2, HIGH);
    digitalWrite(LED, HIGH);
    
  }
  else {
    digitalWrite(buzzer, LOW);
    digitalWrite(relay1, LOW);
    digitalWrite(relay2, LOW);
    digitalWrite(LED, LOW);
  }
}
