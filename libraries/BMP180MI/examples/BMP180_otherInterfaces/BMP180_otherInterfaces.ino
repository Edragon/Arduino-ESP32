// BMP180_otherInterfaces.ino
//
// shows how to use the BMP180MI library with interfaces other than the native I2C or SPI interfaces. 
//
// Copyright (c) 2018 Gregor Christandl
//
// connect the bmp180 to the Arduino like this:
// Arduino - bmp180
// 5V ------ VCC
// GND ----- GND
// SDA ----- SDA
// SCL ----- SCL

#include <Arduino.h>
#include <Wire.h>

#include <BMP180MI.h>

#define I2C_ADDRESS 0x77

//class derived from BMP180MI that implements communication using a library other than the native I2C library. 
class BMP180Wire1 : public BMP180MI
{
public:
	//constructor of the derived class. 
	//@param address i2c address of the sensor.
	BMP180Wire1(uint8_t i2c_address) :
		address_(i2c_address)	//initialize the BMP180Wire1 classes private member address_ to the i2c address provided
	{
		//nothing else to do here...
	}

private:
	//this function must be implemented by derived classes. it is used to initialize the interface. first time communication
	//test are not necessary - these checks are done by the BMP180MI class
	//@return true on success, false otherwise. 
	bool beginInterface()
	{
		Wire1.begin();

		return true;
	}

	//this function must be implemented by derived classes. this function is responsible for reading data from the sensor. 
	//@param reg register to read. 
	//@return read data (1 byte).
	uint8_t readRegister(uint8_t reg)
	{
#if defined(ARDUINO_SAM_DUE)
		//workaround for Arduino Due. The Due seems not to send a repeated start with the code above, so this 
		//undocumented feature of Wire::requestFrom() is used. can be used on other Arduinos too (tested on Mega2560)
		//see this thread for more info: https://forum.arduino.cc/index.php?topic=385377.0
		Wire1.requestFrom(address_, 1, reg, 1, true);
#else
		Wire1.beginTransmission(address_);
		Wire1.write(reg);
		Wire1.endTransmission(false);
		Wire1.requestFrom(address_, static_cast<uint8_t>(1));
#endif

		return Wire1.read();
	}

	//this function can be implemented by derived classes. implementing this function is optional. 
	//@param reg register to read. 
	//@param length number of registers to read (max: 4)
	//@return read data. LSB = last register read. 
	uint32_t readRegisterBurst(uint8_t reg, uint8_t length)
	{
		if (length > 4)
			return 0L;

		uint32_t data = 0L;

#if defined(ARDUINO_SAM_DUE)
		//workaround for Arduino Due. The Due seems not to send a repeated start with the code below, so this 
		//undocumented feature of Wire::requestFrom() is used. can be used on other Arduinos too (tested on Mega2560)
		//see this thread for more info: https://forum.arduino.cc/index.php?topic=385377.0
		Wire1.requestFrom(address_, length, data, length, true);
#else
		Wire1.beginTransmission(address_);
		Wire1.write(reg);
		Wire1.endTransmission(false);
		Wire1.requestFrom(address_, static_cast<uint8_t>(length));

		for (uint8_t i = 0; i < length; i++)
		{
			data <<= 8;
			data |= Wire1.read();
		}
#endif

		return data;
	}

	//this function must be implemented by derived classes. this function is responsible for sending data to the sensor. 
	//@param reg register to write to.
	//@param data data to write to register.
	void writeRegister(uint8_t reg, uint8_t data)
	{
		Wire1.beginTransmission(address_);
		Wire1.write(reg);
		Wire1.write(data);
		Wire1.endTransmission();
	}

	uint8_t address_;		//i2c address of sensor
};

//create an bmp180 object using the I2C interface, I2C address 0x77 and IRQ pin number 2
BMP180Wire1 bmp180(I2C_ADDRESS);

void setup() {
	// put your setup code here, to run once:
	Serial.begin(9600);

	//wait for serial connection to open (only necessary on some boards)
	while (!Serial);

	//begin() initializes the interface, checks the sensor ID and reads the calibration parameters.  
	if (!bmp180.begin())
	{
		Serial.println("begin() failed. check your BMP180 Interface and I2C Address.");
		while (1);
	}

	//reset sensor to default parameters.
	bmp180.resetToDefaults();

	//enable ultra high resolution mode for pressure measurements
	bmp180.setSamplingMode(BMP180MI::MODE_UHR);

	//...
}

void loop() {
	// put your main code here, to run repeatedly:

	delay(1000);

	//start a temperature measurement
	if (!bmp180.measureTemperature())
	{
		Serial.println("could not start temperature measurement, is a measurement already running?");
		return;
	}

	//wait for the measurement to finish. proceed as soon as hasValue() returned true. 
	do
	{
		delay(100);
	} while (!bmp180.hasValue());

	Serial.print("Temperature: ");
	Serial.print(bmp180.getTemperature());
	Serial.println("degC");

	//start a pressure measurement. pressure measurements depend on temperature measurement, you should only start a pressure 
	//measurement immediately after a temperature measurement. 
	if (!bmp180.measurePressure())
	{
		Serial.println("could not start perssure measurement, is a measurement already running?");
		return;
	}

	//wait for the measurement to finish. proceed as soon as hasValue() returned true. 
	do
	{
		delay(100);
	} while (!bmp180.hasValue());

	Serial.print("Pressure: ");
	Serial.print(bmp180.getPressure());
	Serial.println("Pa");
}