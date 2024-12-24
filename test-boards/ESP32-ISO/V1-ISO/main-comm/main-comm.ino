#include <SoftwareSerial.h>

#define IN1 32
#define IN2 33
#define IN3 25
#define IN4 26

#define OUT1 27
#define OUT2 14
#define OUT3 13
#define OUT4 15

#define debugLED 0

#define RS232_RX 23
#define RS232_TX 22
#define RS485_RX 18
#define RS485_TX 19

SoftwareSerial RS232;
SoftwareSerial RS485;

void setup() {
  Serial.begin(9600);
  delay(500);
  Serial.println("\nSoftware serial test started");

  RS232.begin(9600, SWSERIAL_8N1, 23, 22, false, 95, 11);
  RS232.println("\nRS232 ready 1");
  delay(500);
  
  RS485.begin(9600, SWSERIAL_8N1, 18, 19, false, 95, 11);
  RS485.println("\nRS232 ready 1");
  delay(500);
  
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

void allLOW() {
  Serial.println("000");
  digitalWrite(debugLED, LOW);
  digitalWrite(OUT1, LOW);
  digitalWrite(OUT2, LOW);
  digitalWrite(OUT3, LOW);
  digitalWrite(OUT4, LOW);
}

void allHIGH() {
  Serial.println("111");
  digitalWrite(debugLED, HIGH);
  digitalWrite(OUT1, HIGH);
  digitalWrite(OUT2, HIGH);
  digitalWrite(OUT3, HIGH);
  digitalWrite(OUT4, HIGH);
}

void pinCheck(int pin) {
  int status_IN = digitalRead(pin);
  if (status_IN == 0) {
    allLOW();
  } else {
    allHIGH();
  }
}

void loop() {
  while (RS232.available() > 0) {
    Serial.write(RS232.read());
    yield();
  }
  while (RS485.available() > 0) {
    Serial.write(RS485.read());
    yield();
  }
  while (Serial.available() > 0) {
    int dat = Serial.read();
    RS232.write(dat);
    RS485.write(dat);
    yield();
  }



}
