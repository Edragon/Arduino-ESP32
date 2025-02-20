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


#include "BMP180MI.h"

BMP180MI::BMP180MI() :
	command_(0x00),
	sampling_mode_(BMP180MI::MODE_ULP),
	up_(0L),
	ut_(0l),
	B5_(0L)
{
	cal_params_ = {
	0, //cp_AC1_
	0, //cp_AC2_
	0, //cp_AC3_
	0, //cp_AC4_
	0, //cp_AC5_
	0, //cp_AC6_

	0, //cp_B1_
	0, //cp_B2_

	0, //cp_MB_
	0, //cp_MC_
	0, //cp_MD_
	};
}

BMP180MI::~BMP180MI()
{
	//nothing to do here...
}

bool BMP180MI::begin()
{
	//return false if the interface could not be initialized. 
	if (!beginInterface())
		return false;

	//read sensor ID. 
	uint8_t id = readID();

	//check sensor ID. return false if sensor ID does not match a BMP180 ID.
	if (id != BMP180MI::BMP180_ID)
		return false;

	//read compensation parameters
	cal_params_ = readCalibrationParameters();

	return true;
}

bool BMP180MI::measurePressure()
{
	//return false if a measurement is already running. 
	if (readRegisterValue(BMP180_REG_CTRL_MEAS, BMP180_MASK_SCO))
		return false;

	command_ = BMP180_CMD_PRESS;

	//start a measurement. command includes the 'start of conversion' bit. 
	writeRegisterValue(BMP180_REG_CTRL_MEAS, BMP180_MASK_OSS | BMP180_MASK_SCO | BMP180_MASK_MCTRL, (sampling_mode_ << 6) | command_);

	return true;
}

bool BMP180MI::measureTemperature()
{
	//return false if a measurement is already running. 
	if (readRegisterValue(BMP180_REG_CTRL_MEAS, BMP180_MASK_SCO))
		return false;

	command_ = BMP180_CMD_TEMP;

	//start a measurement. command includes the 'start of conversion' bit. 
	writeRegisterValue(BMP180_REG_CTRL_MEAS, BMP180_MASK_SCO | BMP180_MASK_MCTRL, command_);

	return true;
}

bool BMP180MI::hasValue()
{
	if (readRegisterValue(BMP180_REG_CTRL_MEAS, BMP180_MASK_SCO))
		return false;

	switch (command_)
	{
	case BMP180_CMD_PRESS:
		up_ = static_cast<int32_t>(readRegisterValueBurst(BMP180_REG_OUT, BMP180_MASK_PRESS, 3));
		up_ >>= (8 - sampling_mode_);
		break;
	case BMP180_CMD_TEMP:
		ut_ = static_cast<int32_t>(readRegisterValueBurst(BMP180_REG_OUT, BMP180_MASK_TEMP, 2));
		break;
	default:
		return false;
	}

	return true;
}

float BMP180MI::getPressure()
{
	int32_t p = 0;

	int32_t B6 = B5_ - 4000;

	int32_t X1 = (static_cast<int32_t>(cal_params_.cp_B2_) * ((B6 * B6) >> 12)) >> 11;

	int32_t X2 = (static_cast<int32_t>(cal_params_.cp_AC2_) * B6) >> 11;

	int32_t X3 = X1 + X2;

	int32_t B3 = ((((static_cast<int32_t>(cal_params_.cp_AC1_) << 2) + X3) << sampling_mode_) + 2) >> 2;

	X1 = (static_cast<int32_t>(cal_params_.cp_AC3_) * B6) >> 13;

	X2 = (static_cast<int32_t>(cal_params_.cp_B1_) * ((B6 * B6) >> 12)) >> 16;

	X3 = (X1 + X2 + 2) >> 2;

	uint32_t B4 = static_cast<uint32_t>(cal_params_.cp_AC4_) * (static_cast<uint32_t>(X3 + 32768)) >> 15;

	uint32_t B7 = static_cast<uint32_t>(up_ - B3) * (50000 >> sampling_mode_);

	if (B7 < 0x80000000)
		p = (B7 << 1) / B4;
	else
		p = (B7 / B4) << 1;

	X1 = (p >> 8) * (p >> 8);

	X1 = (X1 * 3038) >> 16;

	X2 = (-7357 * p) >> 16;

	p = p + ((X1 + X2 + 3791) >> 4);

	return static_cast<float>(p);
}

float BMP180MI::getTemperature()
{
	int32_t X1 = ((ut_ - static_cast<int32_t>(cal_params_.cp_AC6_)) * static_cast<int32_t>(cal_params_.cp_AC5_)) >> 15;

	int32_t X2 = (static_cast<int32_t>(cal_params_.cp_MC_) << 11) / (X1 + static_cast<int32_t>(cal_params_.cp_MD_));

	B5_ = X1 + X2;

	int32_t T = (B5_ + 8) >> 4;

	return static_cast<float>(T) * 0.1;
}

float BMP180MI::readTemperature()
{
	if (!measureTemperature())
		return NAN;

	do
	{
		delay(10);
	} while (!hasValue());

	return getTemperature();
}

float BMP180MI::readPressure()
{
	if (isnan(readTemperature()))
		return NAN;

	if (!measurePressure())
		return NAN;

	do
	{
		delay(10);
	} while (!hasValue());

	return getPressure();
}

uint8_t BMP180MI::readID()
{
	return readRegisterValue(BMP180_REG_ID, BMP180_MASK_ID);
}

BMP180MI::BMP180CalParams BMP180MI::readCalibrationParameters()
{
	BMP180MI::BMP180CalParams cal_params = {
		0, //cp_AC1_
		0, //cp_AC2_
		0, //cp_AC3_
		0, //cp_AC4_
		0, //cp_AC5_
		0, //cp_AC6_

		0, //cp_B1_
		0, //cp_B2_

		0, //cp_MB_
		0, //cp_MC_
		0, //cp_MD_
	};

	//read compensation parameters
	cal_params.cp_AC1_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_AC1, BMP180_MASK_CAL, 2));
	cal_params.cp_AC2_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_AC2, BMP180_MASK_CAL, 2));
	cal_params.cp_AC3_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_AC3, BMP180_MASK_CAL, 2));
	cal_params.cp_AC4_ = readRegisterValueBurst(BMP180_REG_CAL_AC4, BMP180_MASK_CAL, 2);
	cal_params.cp_AC5_ = readRegisterValueBurst(BMP180_REG_CAL_AC5, BMP180_MASK_CAL, 2);
	cal_params.cp_AC6_ = readRegisterValueBurst(BMP180_REG_CAL_AC6, BMP180_MASK_CAL, 2);

	cal_params.cp_B1_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_B1, BMP180_MASK_CAL, 2));
	cal_params.cp_B2_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_B2, BMP180_MASK_CAL, 2));

	cal_params.cp_MB_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_MB, BMP180_MASK_CAL, 2));
	cal_params.cp_MC_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_MC, BMP180_MASK_CAL, 2));
	cal_params.cp_MD_ = static_cast<int16_t>(readRegisterValueBurst(BMP180_REG_CAL_MD, BMP180_MASK_CAL, 2));

	return cal_params;
}

void BMP180MI::resetToDefaults()
{
	writeRegister(BMP180_REG_RESET, BMP180_CMD_RESET);
}

uint8_t BMP180MI::getSamplingMode()
{
	return sampling_mode_;
}

bool BMP180MI::setSamplingMode(uint8_t mode)
{
	switch (mode)
	{
	case BMP180MI::MODE_ULP:
	case BMP180MI::MODE_STD:
	case BMP180MI::MODE_HR:
	case BMP180MI::MODE_UHR:
		sampling_mode_ = mode;
		break;
	default:
		return false;
	}

	return true;
}

uint8_t BMP180MI::getMaskShift(uint8_t mask)
{
	uint8_t return_value = 0;

	//count how many times the mask must be shifted right until the lowest bit is set
	if (mask != 0)
	{
		while (!(mask & 1))
		{
			return_value++;
			mask >>= 1;
		}
	}

	return return_value;
}

uint8_t BMP180MI::setMaskedBits(uint8_t reg, uint8_t mask, uint8_t value)
{
	//clear mask bits in register
	reg &= (~mask);

	//set masked bits in register according to value
	return ((value << getMaskShift(mask)) & mask) | reg;
}

uint8_t BMP180MI::readRegisterValue(uint8_t reg, uint8_t mask)
{
	return getMaskedBits(readRegister(reg), mask);
}

void BMP180MI::writeRegisterValue(uint8_t reg, uint8_t mask, uint8_t value)
{
	uint8_t reg_val = readRegister(reg);
	writeRegister(reg, setMaskedBits(reg_val, mask, value));
}

uint32_t BMP180MI::readRegisterValueBurst(uint8_t reg, uint32_t mask, uint8_t length)
{
	return getMaskedBits(readRegisterBurst(reg, length), mask);
}

uint32_t BMP180MI::readRegisterBurst(uint8_t reg, uint8_t length)
{
	if (length > 4)
		return 0L;

	uint32_t data = 0L;

	for (uint8_t i = 0; i < length; i++)
	{
		data <<= 8;
		data |= static_cast<uint32_t>(readRegister(reg));
	}

	return data;
}
