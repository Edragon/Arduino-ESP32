/*
BME280_Mini.cpp - Library for reading temperature, humidity and pressure from BME280 sensor.

Created by Asha Geyon

Distributed under CC-BY-SA 4.0

Last_update: 2024-11-17

Source : https://github.com/Natpol50/BME280_mini
*/


#include "BME280_Mini.h"

BME280_Mini::BME280_Mini(uint8_t sda, uint8_t scl, uint8_t addr) 
    : sda_pin(sda), scl_pin(scl), addr(addr) {}

void BME280_Mini::i2c_start() {
    digitalWrite(sda_pin, HIGH);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(4);
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(4);
    digitalWrite(scl_pin, LOW);
}

void BME280_Mini::i2c_stop() {
    digitalWrite(sda_pin, LOW);
    delayMicroseconds(4);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(4);
    digitalWrite(sda_pin, HIGH);
    delayMicroseconds(4);
}

bool BME280_Mini::i2c_write(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(sda_pin, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
        delayMicroseconds(2);
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(2);
        digitalWrite(scl_pin, LOW);
    }

    // Release SDA for ACK
    pinMode(sda_pin, INPUT_PULLUP);
    delayMicroseconds(2);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(2);
    bool ack = (digitalRead(sda_pin) == LOW);
    digitalWrite(scl_pin, LOW);
    pinMode(sda_pin, OUTPUT);
    return ack;
}

uint8_t BME280_Mini::i2c_read(bool ack) {
    uint8_t data = 0;
    pinMode(sda_pin, INPUT_PULLUP);
    
    for (uint8_t i = 0; i < 8; i++) {
        data <<= 1;
        digitalWrite(scl_pin, HIGH);
        delayMicroseconds(2);
        if (digitalRead(sda_pin)) data |= 1;
        digitalWrite(scl_pin, LOW);
        delayMicroseconds(2);
    }
    
    pinMode(sda_pin, OUTPUT);
    digitalWrite(sda_pin, ack ? LOW : HIGH);
    delayMicroseconds(2);
    digitalWrite(scl_pin, HIGH);
    delayMicroseconds(2);
    digitalWrite(scl_pin, LOW);
    
    return data;
}

bool BME280_Mini::writeReg(uint8_t reg, uint8_t value) {
    i2c_start();
    if (!i2c_write(addr << 1)) { i2c_stop(); return false; }
    if (!i2c_write(reg)) { i2c_stop(); return false; }
    if (!i2c_write(value)) { i2c_stop(); return false; }
    i2c_stop();
    return true;
}

bool BME280_Mini::readRegs(uint8_t reg, uint8_t* buffer, uint8_t len) {
    i2c_start();
    if (!i2c_write(addr << 1)) { i2c_stop(); return false; }
    if (!i2c_write(reg)) { i2c_stop(); return false; }
    i2c_start();
    if (!i2c_write((addr << 1) | 1)) { i2c_stop(); return false; }
    
    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = i2c_read(i < (len-1));
    }
    
    i2c_stop();
    return true;
}

bool BME280_Mini::begin() {
    // Configure pins
    pinMode(sda_pin, OUTPUT);
    pinMode(scl_pin, OUTPUT);
    digitalWrite(sda_pin, HIGH);
    digitalWrite(scl_pin, HIGH);
    
    // Verify chip ID
    uint8_t id;
    if (!readRegs(0xD0, &id, 1) || id != 0x60) return false;
    
    // Read calibration data
    uint8_t buffer[24];
    if (!readRegs(0x88, buffer, 24)) return false;
    
    cal.T1 = (buffer[1] << 8) | buffer[0];
    cal.T2 = (buffer[3] << 8) | buffer[2];
    cal.T3 = (buffer[5] << 8) | buffer[4];
    cal.P1 = (buffer[7] << 8) | buffer[6];
    cal.P2 = (buffer[9] << 8) | buffer[8];
    cal.P3 = (buffer[11] << 8) | buffer[10];
    cal.P4 = (buffer[13] << 8) | buffer[12];
    cal.P5 = (buffer[15] << 8) | buffer[14];
    cal.P6 = (buffer[17] << 8) | buffer[16];
    cal.P7 = (buffer[19] << 8) | buffer[18];
    cal.P8 = (buffer[21] << 8) | buffer[20];
    cal.P9 = (buffer[23] << 8) | buffer[22];

    if (!readRegs(0xA1, &cal.H1, 1)) return false;
    
    if (!readRegs(0xE1, buffer, 7)) return false;
    cal.H2 = (buffer[1] << 8) | buffer[0];
    cal.H3 = buffer[2];
    cal.H4 = (buffer[3] << 4) | (buffer[4] & 0x0F);
    cal.H5 = (buffer[5] << 4) | (buffer[4] >> 4);
    cal.H6 = buffer[6];

    // Configure sensor
    if (!writeReg(0xF2, 0x01)) return false; // ctrl_hum
    if (!writeReg(0xF4, 0x27)) return false; // ctrl_meas
    if (!writeReg(0xF5, 0x00)) return false; // config
    
    return true;
}

bool BME280_Mini::sleep() {
    return writeReg(0xF4, 0x24); // Sleep mode
}

bool BME280_Mini::wake() {
    return writeReg(0xF4, 0x27); // Normal mode
}

bool BME280_Mini::read(Data& data) {
    uint8_t buffer[8];
    if (!readRegs(0xF7, buffer, 8)) return false;
    
    int32_t t_fine;
    int32_t adc_T = ((buffer[3] << 12) | (buffer[4] << 4) | (buffer[5] >> 4));
    int32_t adc_P = ((buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4));
    int32_t adc_H = (buffer[6] << 8) | buffer[7];
    
    data.temperature = compensateTemp(adc_T, t_fine) / 100.0 + 167;
    data.pressure = (compensatePress(adc_P, t_fine) - 14300)/100;
    data.humidity = compensateHum(adc_H, t_fine) + 9;
    
    return true;
}


int32_t BME280_Mini::compensateTemp(int32_t adc_T, int32_t& t_fine) {
    int32_t var1 = ((((adc_T / 8) - ((int32_t)cal.T1 * 2))) * ((int32_t)cal.T2)) / 2048;
    int32_t var2 = (((((adc_T / 16) - ((int32_t)cal.T1)) * ((adc_T / 16) - ((int32_t)cal.T1))) / 4096) * ((int32_t)cal.T3)) / 16384;
    t_fine = var1 + var2;
    return (t_fine * 5 + 128) >> 8;
}

float BME280_Mini::compensatePress(int32_t adc_P, int32_t t_fine) {
    int64_t var1 = ((int64_t)t_fine) - 128000;
    int64_t var2 = var1 * var1 * (int64_t)cal.P6;
    var2 = var2 + ((var1 * (int64_t)cal.P5) << 17);
    var2 = var2 + (((int64_t)cal.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)cal.P3) >> 8) + ((var1 * (int64_t)cal.P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)cal.P1) >> 33;
    if (var1 == 0) return 0;
    
    int64_t p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)cal.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)cal.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((int64_t)cal.P7) << 4);
    return (float)p / 256.0f;
}

uint8_t BME280_Mini::compensateHum(int32_t adc_H, int32_t t_fine) {
    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)cal.H4) << 20) - (((int32_t)cal.H5) * v_x1_u32r)) +
                   ((int32_t)16384)) >> 15) *
                 (((((((v_x1_u32r * ((int32_t)cal.H6)) >> 10) * (((v_x1_u32r *
                   ((int32_t)cal.H3)) >> 11) + ((int32_t)32768))) >> 10) +
                   ((int32_t)2097152)) * ((int32_t)cal.H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) *
                   ((int32_t)cal.H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint8_t)((v_x1_u32r >> 12) / 1024);
}
