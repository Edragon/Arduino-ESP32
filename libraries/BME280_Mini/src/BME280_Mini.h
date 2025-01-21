/*
BME280_Mini.h - header for a library for reading temperature, humidity and pressure from BME280 sensor.

Created by Asha Geyon

Distributed under CC-BY-SA 4.0

Last_update: 2024-11-17

Source : https://github.com/Natpol50/BME280_mini
*/

#ifndef BME280_MINI_H
#define BME280_MINI_H

#include <Arduino.h>

class BME280_Mini {
public:
    struct Data {
        float temperature;
        float pressure;
        uint8_t humidity;
    };

    BME280_Mini(uint8_t sda, uint8_t scl, uint8_t addr = 0x76);
    bool begin();
    bool sleep();
    bool wake();
    bool read(Data& data);

private:
    const uint8_t sda_pin;
    const uint8_t scl_pin;
    const uint8_t addr;
    
    struct {
        uint16_t T1, P1;
        int16_t T2, T3, P2, P3, P4, P5, P6, P7, P8, P9, H2, H4, H5;
        uint8_t H1, H3;
        int8_t H6;
    } cal;

    void i2c_start();
    void i2c_stop();
    bool i2c_write(uint8_t data);
    uint8_t i2c_read(bool ack);
    bool writeReg(uint8_t reg, uint8_t value);
    bool readRegs(uint8_t reg, uint8_t* buffer, uint8_t len);
    int32_t compensateTemp(int32_t adc_T, int32_t& t_fine);
    float compensatePress(int32_t adc_P, int32_t t_fine);
    uint8_t compensateHum(int32_t adc_H, int32_t t_fine);
};

#endif
