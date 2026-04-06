#include "SensorHub.h"

SensorHub::SensorHub(uint8_t addr) : ADDR(addr)
{
    mode = 1;
    Wire.begin();
}

#if SOC_I2S_SUPPORTS_ADC
SensorHub::SensorHub(uint8_t ws, uint8_t bclock, uint8_t data, uint32_t samplingRate, uint8_t channel)
{
    this->ws = ws;
    this->bclock = bclock;
    this->data = data;
    this->samplingRate = samplingRate;
    mode = 2;
}
#endif

void SensorHub::i2c_execute(uint8_t reg, uint8_t data)
{
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    Wire.write(data);
    Wire.endTransmission(true);
}

void SensorHub::i2c_execute_16bit(uint8_t reg, uint16_t data)
{
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    Wire.write(data >> 8);
    Wire.write(data & 0xFF);
    Wire.endTransmission(true);
}

bool SensorHub::i2c_readByte(uint8_t reg, uint8_t *const data, uint8_t length)
{
    startTransmission(reg);
    Wire.requestFrom(ADDR, length);
    if (Wire.available() < length)
        return false;
    for (uint8_t i = 0; i < length; i++)
        data[i] = Wire.read();
    return true;
}

bool SensorHub::i2c_readByte(uint8_t reg, int8_t *const data, uint8_t length)
{
    startTransmission(reg);
    Wire.requestFrom(ADDR, length);
    if (Wire.available() < length)
        return false;
    for (uint8_t i = 0; i < length; i++)
        data[i] = Wire.read();
    return true;
}

void SensorHub::startTransmission(uint8_t reg)
{
    Wire.beginTransmission(ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
}

bool SensorHub::is_sensor_connected()
{
    Wire.beginTransmission(ADDR);
    return (Wire.endTransmission() == 0);
}

SensorHub::~SensorHub()
{
}