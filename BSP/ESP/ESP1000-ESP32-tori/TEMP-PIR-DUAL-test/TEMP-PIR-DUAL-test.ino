
#include <Wire.h>               // Only needed for Arduino 1.6.5 and earlier

#include <BMP280.h>
BMP280 bmp280;

#define flash 4
#define onboard 33
#define PIR 3


void setup()
{
  Serial.begin(115200);
  delay(10);
  // Serial.println("BMP280 example");

  Wire.begin(15, 13); //Join I2C bus
  bmp280.begin();
  delay(100);
//  while (!bmp280.begin(0x77))
//  {
//    Serial.println("Could not find BMP280 sensor!");
//    delay(1000);
//  }
  pinMode(flash, OUTPUT);
  pinMode(onboard, OUTPUT);
  pinMode(PIR, INPUT_PULLDOWN);
  analogReadResolution(12);
  delay(100);
}

void loop()
{
  //Get pressure value
  uint32_t pressure = bmp280.getPressure();
  float temperature = bmp280.getTemperature();

  int ValuePIR = digitalRead(PIR);

  //Print the results
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C \t");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.print("Pa");

  Serial.printf("ADC analog value = %d\n", ValuePIR);
  Serial.println("  ");

  delay(2000);
}
