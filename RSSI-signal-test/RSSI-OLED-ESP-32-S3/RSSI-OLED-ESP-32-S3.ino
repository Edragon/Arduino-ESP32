#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>

#define OLED_ADDR 0x3C
#define OLED_SDA 12
#define OLED_SCL 11
#define OLED_RST 14
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);
  delay(100);

  Wire.begin(OLED_SDA, OLED_SCL);  // Add this line

  // Initialize OLED display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(WHITE);

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

  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.print("WiFi      Strength:");
  display.setTextSize(2);
  display.setCursor(0, 35);
  display.print(rssi);
  display.setTextSize(2);
  //display.setCursor(0, 50);
  display.print(" dBm");
  display.display();

  delay(1000);
}