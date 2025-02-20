
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

#include <BMP280.h>
BMP280 bmp280;

SSD1306Wire display(0x3c, 15, 13);

//#define flash 4
//#define onboard 33
#define PIR 3

void setup() {
  Serial.begin(115200);

  Wire.begin(15, 13); //Join I2C bus
  bmp280.begin();
  delay(100);

  //  bmp280.reset();
  //  delay(100);

  //  pinMode(flash, OUTPUT);
  //  pinMode(onboard, OUTPUT);
  pinMode(PIR, INPUT_PULLDOWN);
  //  analogReadResolution(12);
  //  delay(100);
}

void loop() {
  //  Wire.endTransmission();
  //  bmp280.getCalibrate();
  //  delay(1000);

  bmp280.reset();
  delay(100);
  //  bmp280.getCalibrate();
  //  delay(100);

  uint32_t pressure = bmp280.getPressure();
  float temperature = bmp280.getTemperature();

  int ValuePIR = digitalRead(PIR);

  //Print the results
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C \t");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.print("Pa \t");

  Serial.printf("ADC analog value = %d\n", ValuePIR);
  Serial.println("  \t");
  //  Wire.endTransmission();
  delay(2000);


  display.init();
  delay(50);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Temp: ");
  display.drawString(50, 0, String(temperature));

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 20, "Press: ");
  display.drawString(50, 20, String(pressure));

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 40, "PIR: ");
  display.drawString(50, 40, String(ValuePIR));

  display.display();
  delay(1000);
  //display.end();


  //display.resetDisplay();
  //Wire.endTransmission();
}
