#ifndef SENSOR_HUB_TEMPLATES_H
#define SENSOR_HUB_TEMPLATES_H

#include "SensorHub.h"
#include <Wire.h>

#ifdef __cplusplus

template <typename T>
bool SensorHub::i2c_read_Xbit_LE(uint8_t reg, T *const data, uint8_t length)
{
    uint8_t l = length % 8 ? (length + (8 - length % 8)) / 8 : length / 8;
    startTransmission(reg);
    Wire.requestFrom(ADDR, l);
    if (Wire.available() == l)
    {
        T tempData = 0;
        for (int i = 0; i < l; i++)
            tempData |= (T)Wire.read() << 8 * i;

        if (length % 8)
            tempData >>= (8 - length % 8);

        *data = (T)tempData;
    }
    else
        return false;
    return true;
}

template <typename T>
bool SensorHub::i2c_read_Xbit(uint8_t reg, T *const data, uint8_t length)
{
    uint8_t l = length % 8 ? (length + (8 - length % 8)) / 8 : length / 8;
    startTransmission(reg);
    Wire.requestFrom(ADDR, l);
    if (Wire.available() == l)
    {
        T tempData = 0;
        for (int i = 0; i < l; i++)
            tempData |= (T)Wire.read() << 8 * (l - 1 - i);

        if (length % 8)
            tempData >>= (8 - length % 8);

        *data = (T)tempData;
    }
    else
        return false;
    return true;
}

#endif // __cplusplus
#endif // SENSOR_HUB_TEMPLATES_H
