
#define IN1 32
#define IN2 33
#define IN3 25
#define IN4 26

#define OUT1 27
#define OUT2 14
#define OUT3 20
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

void pinCheck(int pin) {
  int status_IN = digitalRead(pin);

  if (status_IN == 0) {
    digitalWrite(debugLED, LOW);
    Serial.println("000");

  } else {
    digitalWrite(debugLED, HIGH);
    Serial.println("111");
  }
}

void loop() {
//  pinCheck(IN1);
//  pinCheck(IN2);
//  pinCheck(IN3);
  pinCheck(IN4);
}
