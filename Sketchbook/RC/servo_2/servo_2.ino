#include <Arduino.h>

#define SERVO1_PIN 13
#define SERVO2_PIN 14  // On UNO, D14 is A0. Adjust if needed.

// Function to generate a servo pulse manually
void servoPulse(int pin, int angle) {
  // Map angle (0–180) to pulse width (500–2500 µs)
  int pulseWidth = map(angle, 0, 180, 500, 2500);

  // Send pulse
  digitalWrite(pin, HIGH);
  delayMicroseconds(pulseWidth);
  digitalWrite(pin, LOW);

  // Complete 20ms cycle
  delayMicroseconds(20000 - pulseWidth);
}

void setup() {
  pinMode(SERVO1_PIN, OUTPUT);
  pinMode(SERVO2_PIN, OUTPUT);
  Serial.begin(9600);
  Serial.println("Manual PWM Servo Test Start");
}

void loop() {
  Serial.println("Move to 0°");
  for (int i = 0; i < 50; i++) {
    servoPulse(SERVO1_PIN, 0);
    servoPulse(SERVO2_PIN, 180);
  }

  Serial.println("Move to 180°");
  for (int i = 0; i < 50; i++) {
    servoPulse(SERVO1_PIN, 180);
    servoPulse(SERVO2_PIN, 0);
  }

  Serial.println("Center (90°)");
  for (int i = 0; i < 50; i++) {
    servoPulse(SERVO1_PIN, 90);
    servoPulse(SERVO2_PIN, 90);
  }
}
