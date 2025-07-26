
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"

SSD1306Wire display(0x3c, 15, 13);

#include <BME280I2C.h>

#define SERIAL_BAUD 115200

BME280I2C bme;

#define PIR 3
//#define flash 4
//#define onboard 33

void setup()
{

  Serial.begin(SERIAL_BAUD);
  while (!Serial) {} // Wait

  Wire.begin(15, 13);
  while (!bme.begin())
  {
    Serial.println("Could not find BME280 sensor!");
    delay(1000);
  }

  switch (bme.chipModel())
  {
    case BME280::ChipModel_BME280:
      Serial.println("Found BME280 sensor! Success.");
      break;
    case BME280::ChipModel_BMP280:
      Serial.println("Found BMP280 sensor! No Humidity available.");
      break;
    default:
      Serial.println("Found UNKNOWN sensor! Error!");
  }

  pinMode(PIR, INPUT_PULLDOWN);
  analogReadResolution(12);
  delay(100);
}



//////////////////////////////////////////////////////////////////
void loop()
{
  printBME280Data(&Serial);

}

//////////////////////////////////////////////////////////////////
void printBME280Data
(
  Stream* client
)
{
  float temp(NAN), hum(NAN), pres(NAN);

  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pres, temp, hum, tempUnit, presUnit);

  client->print("Temp: ");
  client->print(temp);
  client->print("°" + String(tempUnit == BME280::TempUnit_Celsius ? 'C' : 'F'));
  client->print("\tHumidity: ");
  client->print(hum);
  client->print("% RH");
  client->print("\tPressure: ");
  client->print(pres);
  client->print("Pa");
  delay(1000);

  int ValuePIR = digitalRead(PIR);
  client->print("\tPIR: ");
  client->println(ValuePIR);

  //  if (ValuePIR == HIGH) {
  //    digitalWrite(flash, HIGH);
  //    delay(500); // make a photo for 2 seconds freeze
  //  } else {
  //    // turn LED off:
  //    digitalWrite(flash, LOW);
  //  }
  //  delay(500);

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
