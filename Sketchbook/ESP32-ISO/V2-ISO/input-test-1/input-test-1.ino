#define IN1 34
#define IN2 35
#define IN3 32
#define IN4 33

#define OUT1 27
#define OUT2 14
#define OUT3 13
#define OUT4 4

#define debugLED 0

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("\nSoftware serial test started");

  pinMode(IN1, INPUT_PULLUP);
  pinMode(IN2, INPUT_PULLUP);
  pinMode(IN3, INPUT_PULLUP);
  pinMode(IN4, INPUT_PULLUP);

  pinMode(debugLED, OUTPUT);
  digitalWrite(debugLED, HIGH);

}

void pinCheck() {

  int status_IN1 = digitalRead(IN1);
  int status_IN2 = digitalRead(IN2);
  int status_IN3 = digitalRead(IN3);
  int status_IN4 = digitalRead(IN4);

  if (status_IN1 == 0) {
    digitalWrite(debugLED, LOW);
    Serial.println("000");
  }
  else if (status_IN2 == 0) {
    digitalWrite(debugLED, LOW);
    Serial.println("000");
  }
  else if (status_IN3 == 0) {
    digitalWrite(debugLED, LOW);
    Serial.println("000");
  }
  else if (status_IN4 == 0) {
    digitalWrite(debugLED, LOW);
    Serial.println("000");
  }
  else {
    digitalWrite(debugLED, HIGH);
    Serial.println("111");
  }
}

void loop() {
  pinCheck();

}
