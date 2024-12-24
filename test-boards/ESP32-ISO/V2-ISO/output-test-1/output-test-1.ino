
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

  pinMode(OUT1, OUTPUT);
  pinMode(OUT2, OUTPUT);
  pinMode(OUT3, OUTPUT);
  pinMode(OUT4, OUTPUT);

  pinMode(debugLED, OUTPUT);
  digitalWrite(debugLED, HIGH);

}

void loop() {
  digitalWrite(debugLED, LOW);
  digitalWrite(OUT1, LOW);
  digitalWrite(OUT2, LOW);
  digitalWrite(OUT3, LOW);
  digitalWrite(OUT4, LOW);
  Serial.println("000");
  delay(5000);

  digitalWrite(debugLED, HIGH);
  digitalWrite(OUT1, HIGH);
  digitalWrite(OUT2, HIGH);
  digitalWrite(OUT3, HIGH);
  digitalWrite(OUT4, HIGH);
  Serial.println("111");
  delay(5000);

}
