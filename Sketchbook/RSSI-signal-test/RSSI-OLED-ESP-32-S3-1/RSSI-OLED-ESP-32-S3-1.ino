#include <Wire.h>
#include "SSD1306Wire.h"
#include <WiFi.h>

#define OLED_ADDR 0x3C
#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

SSD1306Wire display(OLED_ADDR, OLED_SDA, OLED_SCL);

void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize OLED display
  display.init();
  display.flipScreenVertically();

  // Connect to WiFi
  WiFi.begin("111", "electrodragon");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
}

void loop() {
  int32_t rssi = WiFi.RSSI();

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "WiFi      Strength:");
  display.drawString(0, 35, String(rssi));
  display.drawString(0, 50, " dBm");
  display.display();

  delay(1000);
}