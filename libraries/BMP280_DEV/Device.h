/*
  Device is an I2C/SPI compatible base class library.
	
	Copyright (C) Martin Lindupp 2019
	
	V1.0.0 -- Initial release 
	V1.0.1 -- Added ESP32 HSPI support	
	V1.0.2 -- Modification to allow external creation of HSPI object on ESP32
	V1.0.3 -- Addition of SPI write and read byte masks
	V1.0.4 -- Modification to allow user-defined pins for I2C operation on the ESP8266
	V1.0.5 -- Modification to allow user-defined pins for I2C operation on the ESP32
	V1.0.6 -- Initialise "device" constructor member variables in the same order they are declared
	V1.0.7 -- Allow for additional TwoWire instances
	V1.0.8 -- Fixed uninitialised "Wire" pointer for ESP8266/ESP32 with user defined I2C pins 
	
	The MIT License (MIT)
	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:
	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
*/

#ifndef Device_h
#define Device_h

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

////////////////////////////////////////////////////////////////////////////////
// Device Communications
////////////////////////////////////////////////////////////////////////////////

#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
enum Comms { I2C_COMMS, SPI_COMMS, I2C_COMMS_DEFINED_PINS };
#else						 
enum Comms { I2C_COMMS, SPI_COMMS };		 
#endif

////////////////////////////////////////////////////////////////////////////////
// Device Class definition
////////////////////////////////////////////////////////////////////////////////

class Device{
	public:
		Device(TwoWire& twoWire);																		// Device object for I2C operation
#ifdef ARDUINO_ARCH_ESP8266
		Device(uint8_t sda, uint8_t scl, TwoWire& twoWire);					// Device object for ESP8266 I2C operation with user-defined pins
#endif
		Device(uint8_t cs);																					// Device object for SPI operation
#ifdef ARDUINO_ARCH_ESP32
		Device(uint8_t sda, uint8_t scl, TwoWire& twoWire);					// Device object for ESP32 I2C operation with user-defined pins
		Device(uint8_t cs, uint8_t spiPort, SPIClass& spiClass);		// Device object for ESP32 HSPI operation with supplied SPI object
#endif		
		void setClock(uint32_t clockSpeed);													// Set the I2C/SPI clock speed
	protected:
		void initialise();																					// Initialise communications	
		void setI2CAddress(uint8_t addr);											  		// Set the Device I2C address
		void writeByte(uint8_t subAddress, uint8_t data);						// I2C and SPI write byte wrapper function
		uint8_t readByte(uint8_t subAddress);												// I2C and SPI read byte wrapper function
		void readBytes(uint8_t subAddress, uint8_t* dest, uint16_t count);		// I2C and SPI read bytes wrapper function
	private:
		Comms comms;																								// Communications bus: I2C or SPI
		uint8_t address;																						// The device I2C address
		uint8_t cs;																									// The SPI chip select pin
#ifdef ARDUINO_ARCH_ESP32
		uint8_t spiPort;																						// SPI port type VSPI or HSPI
#endif
		TwoWire* i2c;																								// Pointer to the Wire class
		SPIClass* spi;																							// Pointer to the SPI class
		uint32_t spiClockSpeed;																			// The SPI clock speed
		const uint8_t WRITE_MASK = 0x7F;														// Sub-address write mask for SPI communications
		const uint8_t READ_MASK  = 0x80;														// Sub-address read mask for SPI communications
#if defined ARDUINO_ARCH_ESP8266 || defined ARDUINO_ARCH_ESP32
		uint8_t sda, scl;																						// Software I2C SDA and SCL pins 
#endif
};
#endif
