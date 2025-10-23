// ESP32-S3 UART AT command example for A7670 module
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>

#define UART_TX 1  // IO1 as TX
#define UART_RX 2  // IO2 as RX
#define WS2812_PIN 48 // IO48 for WS2812
#define WS2812_NUM 1   // One LED

HardwareSerial ATSerial(1); // Use UART1
Adafruit_NeoPixel ws2812(WS2812_NUM, WS2812_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200); // USB serial for debug
  ATSerial.begin(115200, SERIAL_8N1, UART_RX, UART_TX); // A7670 default baudrate
  ws2812.begin();
  ws2812.show(); // Initialize all pixels to 'off'
  delay(1000);
}

String atCommand = "AT";
String expectedFeedback = "OK";
String cpinCommand = "AT+CPIN?";
String cpinExpected = "+CPIN: READY";

void sendATCommandWait(const String& cmd, const String& expected) {
  while (true) {
    ATSerial.println(cmd);
    Serial.println(cmd + " sent");
    unsigned long start = millis();
    while (millis() - start < 1000) { // Wait up to 1s for response
      if (ATSerial.available()) {
        String resp = ATSerial.readStringUntil('\n');
        Serial.println(resp);
        if (resp.indexOf(expected) >= 0) {
          return;
        }
      }
    }
    delay(500); // Wait before retry
  }
}

void loop() {
  // Step 1: Send AT until OK, then set WS2812 green
  sendATCommandWait(atCommand, expectedFeedback);
  ws2812.setPixelColor(0, ws2812.Color(0, 255, 0)); // Green
  ws2812.show();
  delay(500);

  // Step 2: Send AT+CPIN until ready, then set WS2812 red
  sendATCommandWait(cpinCommand, cpinExpected);
  ws2812.setPixelColor(0, ws2812.Color(255, 0, 0)); // Red
  ws2812.show();
  delay(1000); // Hold red for a second

  // Optionally, stop or repeat
  while (1) delay(1000); // Stop here
}
