//Multi interface Bosch Sensortec BMP180  pressure sensor library 
// Copyright (c) 2018-2019 Gregor Christandl <christandlg@yahoo.com>
// home: https://bitbucket.org/christandlg/BMP180mi
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

#ifndef BMP180TWOWIRE_H_
#define BMP180TWOWIRE_H_

#include <Arduino.h>
#include <Wire.h>

#include "BMP180MI.h"

class BMP180TwoWire : 
	public BMP180MI
{
public:
	BMP180TwoWire(TwoWire *wire, uint8_t i2c_address);
	virtual ~BMP180TwoWire();

protected:
	TwoWire *wire_;

	uint8_t address_;

private:
	bool beginInterface();

	uint8_t readRegister(uint8_t reg);

	uint32_t readRegisterBurst(uint8_t reg, uint8_t length);

	void writeRegister(uint8_t reg, uint8_t value);
};

#endif /* BMP180TWOWIRE_H_ */
