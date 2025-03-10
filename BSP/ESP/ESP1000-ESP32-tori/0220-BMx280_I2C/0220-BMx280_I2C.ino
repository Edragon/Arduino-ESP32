#include <Arduino.h>
#include <Wire.h>
#include <BMx280I2C.h>

#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
SSD1306Wire display(0x3c, 15, 13);

BMx280I2C bmx280(0x76);

#define flash 4
#define PIR 3

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  while (!Serial);
  Wire.begin(15, 13);

  //begin() checks the Interface, reads the sensor ID (to differentiate between BMP280 and BME280)
  //and reads compensation parameters.
  if (!bmx280.begin())
  {
    Serial.println("begin() failed. check your BMx280 Interface and I2C Address.");
    while (1);
  }

  if (bmx280.isBME280())
    Serial.println("sensor is a BME280");
  else
    Serial.println("sensor is a BMP280");

  //reset sensor to default parameters.

  bmx280.resetToDefaults();
  bmx280.writeOversamplingPressure(BMx280MI::OSRS_P_x16);
  bmx280.writeOversamplingTemperature(BMx280MI::OSRS_T_x16);
  delay(100);

  Serial.println("sensor setup is done");

  pinMode(flash, OUTPUT);
  pinMode(PIR, INPUT_PULLDOWN);
  // analogReadResolution(12);
  delay(100);
}

void loop() {

  delay(100);

  //start a measurement
  if (!bmx280.measure())
  {
    Serial.println("could not start measurement, is a measurement already running?");
    return;
  }

  //wait for the measurement to finish
  do
  {
    delay(100);
  } while (!bmx280.hasValue());

  Serial.println("Measurement is done");

  double pres = bmx280.getPressure64();
  float temp = bmx280.getTemperature();
 
  int ValuePIR = digitalRead(PIR);


  Serial.print("  Pressure (64 bit): "); Serial.print(temp);
  Serial.print("  Temperature: "); Serial.print(pres);
  Serial.print("PIR: "); Serial.println(ValuePIR);

  if (ValuePIR == HIGH) {
    digitalWrite(flash, HIGH);
    delay(20); // make a photo for 2 seconds freeze
  } else {
    // turn LED off:
    digitalWrite(flash, LOW);
  }
  delay(100);        // delay in between reads for stability

  display.init();
  delay(50);
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, "Temp: ");
  display.drawString(50, 0, String(temp));

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 20, "Press: ");
  display.drawString(50, 20, String(pres));

  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 40, "PIR: ");
  display.drawString(50, 40, String(ValuePIR));

  display.display();
  delay(500);
}
