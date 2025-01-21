#ifndef BME280_ARDUINO_I2C_H
#define BME280_ARDUINO_I2C_H

#include <Arduino.h>
#include <Wire.h>

// BME280 default address
#define BME280_DEFAULT_ADDRESS 0x76

// Settings
#define BME280_OVERSAMPLING_X1 0x01
#define BME280_OVERSAMPLING_X2 0x02
#define BME280_OVERSAMPLING_X4 0x03
#define BME280_OVERSAMPLING_X8 0x04
#define BME280_OVERSAMPLING_X16 0x05

typedef long signed int BME280_S32_t;
typedef long unsigned int BME280_U32_t;
typedef long long signed int BME280_S64_t;

struct BME280Data {
    float temperature;
    float pressure;
    uint8_t humidity;
};

class BME280_Arduino_I2C {
   public:
    BME280_Arduino_I2C(int bmeAddress = BME280_DEFAULT_ADDRESS, TwoWire* customTwoWire = &Wire);
    ~BME280_Arduino_I2C();
    uint8_t begin();
    BME280Data* read();
    /*
    - os (humidity oversampling) between 1 and 5, see CTRL_HUM or BME280 datasheet for details
    */
    bool setHumiditySettings(uint8_t os = BME280_OVERSAMPLING_X1);
    /*
    - tOS (temperature oversampling) between 1 and 5, see OSRS_T or BME280 datasheet for details
    - pOS (pressure oversampling) between 1 and 5, see OSRS_P or BME280 datasheet for details
    - mode (sensor mode) between 0 and 3, see MODE or BME280 datasheet for details
    */
    bool setGeneralSettings(uint8_t tOS = BME280_OVERSAMPLING_X1, uint8_t pOS = BME280_OVERSAMPLING_X1, uint8_t m = 3);
    /*
    - tsb (standby time), between 0 and 7, see T_SB or BME280 datasheet for details
    - filter (IIR filter), between 0 and 4, see FILTER or BME280 datasheet for details
    */
    bool setConfigs(uint8_t tsb = 5, uint8_t filter = 0);

   private:
    unsigned long lastRead = 0;
    bool initialized = false;
    int address;
    TwoWire* wire;
    BME280Data* data = new BME280Data();
    // Registries
    uint8_t BME280_REG_ID = 0xD0;           // chip_id[7:0], should be 0xB6. Available as soon as power-reset cycle is over
    uint8_t BME280_REG_CTRL_HUM = 0xF2;     // Humidity data aquisition options. Applies only after CTRL_MEAS. Oversampling settings.
    uint8_t BME280_REG_CTRL_MEAS = 0xF4;    // Pressure and temp data configs/oversampling. Applies hummidity as well.
    uint8_t BME280_REG_CONFIG = 0xF5;       // Rate, filter and interface options.
    uint8_t BME280_REG_DATA = 0xF7;         // Pressure most significant bit output. 0xF8, 0xF9 are less and xless bits
    uint8_t BME280_REG_CALIB_T1_P9 = 0x88;  // Temperature and pressure of the calibration registers, 24 bytes, grouped by 2
    uint8_t BME280_REG_CALIB_H1 = 0xA1;     // Single char for humidity calibration
    uint8_t BME280_REG_CALIB_H2_H6 = 0xE1;  // Main humidity calibration sequence register, contains 7 bytes
    // Configurations
    uint8_t CTRL_HUM = BME280_OVERSAMPLING_X1;  // Humidity oversampling. 1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16
    uint8_t OSRS_P = BME280_OVERSAMPLING_X1;    // Pressure oversampling.  1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16
    uint8_t OSRS_T = BME280_OVERSAMPLING_X1;    // Temperature oversampling. 1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16
    uint8_t MODE = 3;                           // Mode. 0 - Sleep, 1 and 2 - Forced, 3 - Normal
    uint8_t T_SB = 5;                           // Standby time. 0 - 0.5ms, 1 - 62.5ms, 2 - 125ms, 3 - 250ms, 4 - 500ms, 5 - 1s, 6 - 10ms, 7 - 20ms
    uint8_t FILTER = 0;                         // IIR filter. 0 - off, 1 - 2, 2 - 4, 3 - 8, 4 - 16
    // Calibration values
    struct {
        unsigned short T1, P1;
        short T2, T3, P2, P3, P4, P5, P6, P7, P8, P9, H2, H4, H5;
        unsigned char H1, H3;
        char H6;
    } calibrationData;
    BME280_S32_t getFineResolutionTemperature(BME280_S32_t rawTemp);
    float getPressurePA(BME280_S32_t t_fine, BME280_S32_t rawPressure);
    uint8_t getHumidity(BME280_S32_t t_fine, BME280_S32_t rawHumidity);
    void applyHumiditySettings();
    void applyGeneralSettings();
    void applyConfigs();
    uint32_t READ_D = 1000000;  // Reading delay, calculated from T_SB
    void writeRegister(uint8_t reg, uint8_t value);
};

#endif