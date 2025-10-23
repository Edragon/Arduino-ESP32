#include <Wire.h>
#include "ssd1306.h"
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

// #define OLED_ADDR 0x3C
// #define OLED_SDA 13
// #define OLED_SCL 14
// #define OLED_RST 16
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64

#define LED_PIN 48
#define NUM_LEDS 1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

SSD1306Wire display(0x3c, 11, 12); 


void setup() {
  Serial.begin(115200);
  delay(100);

  // Initialize OLED display
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font6x8);

  // Connect to WiFi
  WiFi.begin("111", "electrodragon");

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  // Initialize WS2812
  strip.begin();
  strip.show(); // Turn off all LEDs
}

void loop() {
  int32_t rssi = WiFi.RSSI();

  ssd1306_fillScreen(0x00);
  ssd1306_printFixed(0, 8, "WiFi Strength:", STYLE_NORMAL);
  char rssiStr[20];
  sprintf(rssiStr, "%d dBm", (int)rssi);
  ssd1306_printFixed(0, 16, rssiStr, STYLE_NORMAL);

  delay(1000);
}