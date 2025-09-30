// ESP32-S3 UART AT command example for A7670 module
#include <HardwareSerial.h>

#define UART_TX 1  // IO1 as TX
#define UART_RX 2  // IO2 as RX
#define LED_PIN 35 // IO35 as LED indicator

HardwareSerial ATSerial(1); // Use UART1

void setup() {
  Serial.begin(115200); // USB serial for debug
  ATSerial.begin(115200, SERIAL_8N1, UART_RX, UART_TX); // A7670 default baudrate
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);
  delay(1000);
}

String atCommand = "AT";
String expectedFeedback = "OK";

void sendATCommand(const String& cmd, const String& expected) {
  ATSerial.println(cmd);
  Serial.println(cmd + " sent");
  unsigned long start = millis();
  while (millis() - start < 1000) { // Wait up to 1s for response
    if (ATSerial.available()) {
      String resp = ATSerial.readStringUntil('\n');
      Serial.println(resp);
      if (resp.indexOf(expected) >= 0) {
        Serial.println("Expected feedback received.");
        // Blink LED quickly
        digitalWrite(LED_PIN, LOW);
        delay(100);
        digitalWrite(LED_PIN, HIGH);
        break;
      }
    }
  }
}

unsigned long lastSend = 0;
const unsigned long sendInterval = 2000; // 2 seconds

void loop() {
  if (millis() - lastSend > sendInterval) {
    sendATCommand(atCommand, expectedFeedback);
    lastSend = millis();
  }
}
