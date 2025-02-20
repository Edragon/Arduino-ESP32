/**************************************************************************
   Tests the getPressure functions
 **************************************************************************/
#include <BMP280.h>
BMP280 bmp280();

void setup()
{
  Serial.begin(115200);
  delay(10);

  Wire.begin(15, 13); //Join I2C bus SDA, SCL
  bmp280.begin();

  //  while(!bmp280.begin())
  //  {
  //    Serial.println("Could not find BMP280 sensor!");
  //    delay(1000);
  //  }
}

void loop()
{
  //Get pressure value

  //int32_t raw = bmp280.getTemperatureRaw();

  uint32_t pressure = bmp280.getPressure();
  float temperature = bmp280.getTemperature();
//
//  Serial.print(raw);
//  Serial.print(" --- ");

  //Print the results
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C \t");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println("Pa");

  delay(2000);
}
