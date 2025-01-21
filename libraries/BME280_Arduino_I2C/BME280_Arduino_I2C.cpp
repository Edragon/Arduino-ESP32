#include <BME280_Arduino_I2C.h>

BME280_Arduino_I2C::BME280_Arduino_I2C(int bmeAddress, TwoWire* customTwoWire) : address(bmeAddress), wire(customTwoWire) {}

BME280_Arduino_I2C::~BME280_Arduino_I2C() {
    delete data;
}

uint8_t BME280_Arduino_I2C::begin() {
    // Verify presence
    wire->begin();
    wire->beginTransmission(address);
    wire->write(BME280_REG_ID);
    wire->endTransmission();
    wire->requestFrom(address, 1);
    if (wire->available()) {
        uint8_t chipID = Wire.read();
        if (chipID != 0x60) {
            return 2;  // Device not found
        }
    } else {
        return 1;  // Wire is not available
    }

    // Set configurations
    applyHumiditySettings();
    applyConfigs();

    // Pull calibration data
    wire->beginTransmission(address);
    wire->write(BME280_REG_CALIB_T1_P9);
    wire->endTransmission();
    wire->requestFrom(address, 24);
    if (Wire.available()) {
        // Temperature calibration
        calibrationData.T1 = Wire.read() | ((unsigned short)Wire.read() << 8);
        calibrationData.T2 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.T3 = Wire.read() | ((short)Wire.read() << 8);
        // Pressure calibration
        calibrationData.P1 = Wire.read() | ((unsigned short)Wire.read() << 8);
        calibrationData.P2 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P3 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P4 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P5 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P6 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P7 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P8 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.P9 = Wire.read() | ((short)Wire.read() << 8);
    } else {
        return 1;
    }
    // Humidity calibration
    wire->beginTransmission(address);
    wire->write(BME280_REG_CALIB_H1);
    wire->endTransmission();
    wire->requestFrom(address, 1);
    if (wire->available()) {
        calibrationData.H1 = (unsigned char)Wire.read();
    } else {
        return 1;
    }
    wire->beginTransmission(address);
    wire->write(BME280_REG_CALIB_H2_H6);
    wire->endTransmission();
    wire->requestFrom(address, 6);
    if (wire->available()) {
        calibrationData.H2 = Wire.read() | ((short)Wire.read() << 8);
        calibrationData.H3 = Wire.read();
        calibrationData.H4 = (short)Wire.read() << 4;
        uint8_t e5 = Wire.read();
        calibrationData.H4 |= e5 & 0x0F;
        calibrationData.H5 = ((short)Wire.read() << 4) | (e5 >> 4);
        calibrationData.H6 = Wire.read();
    } else {
        return 1;
    }

    initialized = true;
    return 0;
}

BME280Data* BME280_Arduino_I2C::read() {
    if (!initialized) {
        return nullptr;
    }

    if (micros() < READ_D || micros() > lastRead + READ_D) {
        wire->beginTransmission(address);
        wire->write(BME280_REG_DATA);
        wire->endTransmission();
        wire->requestFrom(address, 8);
        if (wire->available()) {
            // Get raw data form the register
            BME280_S32_t rawPressure = ((BME280_S32_t)Wire.read() << 12) | ((BME280_S32_t)Wire.read() << 4) | ((BME280_S32_t)Wire.read() >> 4);
            BME280_S32_t rawTemperature = ((BME280_S32_t)Wire.read() << 12) | ((BME280_S32_t)Wire.read() << 4) | ((BME280_S32_t)Wire.read() >> 4);
            BME280_S32_t rawHumidity = ((BME280_S32_t)Wire.read() << 8) | (BME280_S32_t)Wire.read();
            // Compensate and adjust data
            BME280_S32_t t_fine = getFineResolutionTemperature(rawTemperature);
            data->temperature = (float)((t_fine * 5 + 128) >> 8) / 100;
            data->pressure = getPressurePA(t_fine, rawPressure);
            data->humidity = getHumidity(t_fine, rawHumidity);

            lastRead = micros();
        } else {
            return nullptr;
        }
    }
    return data;
}

bool BME280_Arduino_I2C::setHumiditySettings(uint8_t os) {
    // Invalid values
    if (os < 1 || os > 5) {
        return false;
    }

    // No reason to apply settings when nothing changes
    if (os == CTRL_HUM) {
        return true;
    }

    CTRL_HUM = os;
    // Apply configurations only if library is initialized, otherwise it would be applied on begin()
    if (initialized) {
        applyHumiditySettings();
    }
    return true;
}

bool BME280_Arduino_I2C::setGeneralSettings(uint8_t tOS, uint8_t pOS, uint8_t m) {
    // Invalid values
    if (tOS < 1 || tOS > 5 || pOS < 1 || pOS > 5 || m < 0 || m > 3) {
        return false;
    }

    // No reason to apply settings when nothing changes
    if (tOS == OSRS_T && pOS == OSRS_P && m == MODE) {
        return true;
    }

    OSRS_T = tOS;
    OSRS_P = pOS;
    MODE = m;
    // Apply configurations only if library is initialized, otherwise it would be applied on begin()
    if (initialized) {
        applyGeneralSettings();
    }
    return true;
}

bool BME280_Arduino_I2C::setConfigs(uint8_t tsb, uint8_t filter) {
    // Invalid values
    if (tsb < 0 || tsb > 7 || filter < 0 || filter > 4) {
        return false;
    }

    // No reason to apply settings when nothing changes
    if (tsb == T_SB && filter == FILTER) {
        return true;
    }

    T_SB = tsb;
    FILTER = filter;

    // Apply configurations only if library is initialized, otherwise it would be applied on begin()
    if (initialized) {
        applyConfigs();
    }
    return true;
}

BME280_S32_t BME280_Arduino_I2C::getFineResolutionTemperature(BME280_S32_t rawTemp) {
    BME280_S32_t var1, var2;
    var1 = ((((rawTemp >> 3) - (((BME280_S32_t)calibrationData.T1) << 1))) * ((BME280_S32_t)calibrationData.T2)) >> 11;
    var2 = (((rawTemp >> 4) - ((BME280_S32_t)calibrationData.T1) * ((rawTemp >> 4) - ((BME280_S32_t)calibrationData.T1))) >> 12) * (((BME280_S32_t)calibrationData.T3) >> 14);
    return var1 + var2;
}

float BME280_Arduino_I2C::getPressurePA(BME280_S32_t t_fine, BME280_S32_t rawPressure) {
    BME280_S64_t var1, var2, p;
    var1 = ((BME280_S64_t)t_fine) - 128000;
    var2 = var1 * var1 * (BME280_S64_t)calibrationData.P6;
    var2 = var2 + ((var1 * (BME280_S64_t)calibrationData.P5) << 17);
    var2 = var2 + (((BME280_S64_t)calibrationData.P4) << 35);
    var1 = (((((BME280_S64_t)1) << 47) + var1)) * ((BME280_S64_t)calibrationData.P1) >> 33;
    if (var1 == 0) {
        return 0;
    }
    p = 1048576 - rawPressure;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((BME280_S64_t)calibrationData.P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((BME280_S64_t)calibrationData.P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BME280_S64_t)calibrationData.P7) << 4);
    return (float)p / 256;
}

uint8_t BME280_Arduino_I2C::getHumidity(BME280_S32_t t_fine, BME280_S32_t rawHumidity) {
    BME280_S32_t v_x1_u32r = (t_fine - ((BME280_S32_t)76800));
    v_x1_u32r = (((((rawHumidity << 14) - (((BME280_S32_t)calibrationData.H4) << 20) - (((BME280_S32_t)calibrationData.H5) * v_x1_u32r)) +
                   ((BME280_S32_t)16384)) >>
                  15) *
                 (((((((v_x1_u32r * ((BME280_S32_t)calibrationData.H6)) >> 10) * (((v_x1_u32r *
                                                                                    ((BME280_S32_t)calibrationData.H3)) >>
                                                                                   11) +
                                                                                  ((BME280_S32_t)32768))) >>
                     10) +
                    ((BME280_S32_t)2097152)) *
                       ((BME280_S32_t)calibrationData.H2) +
                   8192) >>
                  14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((BME280_S32_t)calibrationData.H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);
    return (uint8_t)((v_x1_u32r >> 12) / 1024);
}

void BME280_Arduino_I2C::applyHumiditySettings() {
    writeRegister(BME280_REG_CTRL_HUM, CTRL_HUM);
    // Humidity settings will be applied only after general settings are set as well.
    applyGeneralSettings();
}

void BME280_Arduino_I2C::applyGeneralSettings() {
    writeRegister(BME280_REG_CTRL_MEAS, (OSRS_T << 5) | (OSRS_P << 2) | MODE);
}

void BME280_Arduino_I2C::applyConfigs() {
    writeRegister(BME280_REG_CONFIG, (T_SB << 5) | (FILTER << 2));
    // Update READ_D based on T_SB
    switch (T_SB) {
        case 0:
            READ_D = 500;
            break;
        case 1:
            READ_D = 62500;
            break;
        case 2:
            READ_D = 125000;
            break;
        case 3:
            READ_D = 250000;
            break;
        case 4:
            READ_D = 500000;
            break;
        case 5:
            READ_D = 1000000;
            break;
        case 6:
            READ_D = 10000;
            break;
        case 7:
            READ_D = 20000;
            break;
        default:
            break;
    }
}

void BME280_Arduino_I2C::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(address);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}
