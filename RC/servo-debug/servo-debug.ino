#define SERVO_PIN1 13
#define SERVO_PIN2 14
#define PERIOD_US 20000
#define STEP_US 20 // change per period (1 µs per 20 ms => ~40 s for full travel)

void setup() {
  pinMode(SERVO_PIN1, OUTPUT);
  pinMode(SERVO_PIN2, OUTPUT);
  Serial.begin(115200);
  Serial.println("SG90 calibration: send one or two numbers in µs (500-2500). Example: '1500' or '1500 1600'");
}

void loop() {
  static int us1 = 1500;
  static int us2 = 1500;
  static int target1 = 1500;
  static int target2 = 1500;

  if (Serial.available()) {
    long v1 = Serial.parseInt();
    if (v1 >= 500 && v1 <= 2500) target1 = (int)v1;
    long v2 = Serial.parseInt();
    if (v2 >= 500 && v2 <= 2500) target2 = (int)v2;
    // if only one number provided, set both targets to it
    if (v1 >= 500 && v1 <= 2500 && !(v2 >= 500 && v2 <= 2500)) target2 = target1;
    Serial.print("Target1 = "); Serial.print(target1);
    Serial.print("  Target2 = "); Serial.println(target2);
  }

  // slowly move current positions toward targets
  if (us1 < target1) us1 = min(us1 + STEP_US, target1);
  else if (us1 > target1) us1 = max(us1 - STEP_US, target1);

  if (us2 < target2) us2 = min(us2 + STEP_US, target2);
  else if (us2 > target2) us2 = max(us2 - STEP_US, target2);

  // Start of period: set both servos HIGH
  unsigned long start = micros();
  bool p1High = true;
  bool p2High = true;
  digitalWrite(SERVO_PIN1, HIGH);
  digitalWrite(SERVO_PIN2, HIGH);

  // Keep the loop until the end of the period, dropping each servo when its pulse time expires
  while (micros() - start < PERIOD_US) {
    unsigned long elapsed = micros() - start;
    if (p1High && elapsed >= (unsigned long)us1) {
      digitalWrite(SERVO_PIN1, LOW);
      p1High = false;
    }
    if (p2High && elapsed >= (unsigned long)us2) {
      digitalWrite(SERVO_PIN2, LOW);
      p2High = false;
    }
    if (!p1High && !p2High) break;
  }

  // If one or both pulses ended early, wait the remainder of the period
  unsigned long now = micros();
  if (now - start < PERIOD_US) {
    unsigned long wait = PERIOD_US - (now - start);
    if (wait > 0) delayMicroseconds(wait);
  }
}
