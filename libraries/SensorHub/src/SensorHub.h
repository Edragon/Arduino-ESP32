#ifndef SensorHub_H
#define SensorHub_H

#include "Arduino.h"
#include <Wire.h>

#ifdef __cplusplus

#if SOC_I2S_SUPPORTS_ADC
#include <driver/i2s.h>
#endif

/**
 * @class SensorHub
 * @brief Core communication and data processing hub for I2C based sensors.
 *
 * The SensorHub class serves as a foundational interface for sensor communication,
 * providing essential I2C read and write operations. It enables seamless integration
 * with various sensor classes that rely on it for data acquisition and processing.
 * It supports multi-byte transactions, configurable communication modes, and device
 * connectivity verification.
 *
 * @author Saurav Sajeev
 */
class SensorHub
{
public:
#if SOC_I2S_SUPPORTS_ADC

#define I2S_CHANNEL_LEFT 0
#define I2S_CHANNEL_RIGHT 1
#define I2S_CHANNEL_STEREO 2

    /**
     * @brief Constructs a SensorHub object with the specified I2S Configuration.
     * @param ws The word select pin of the sensor.
     * @param bclock The bit clock pin of the sensor.
     * @param data The data pin of the sensor.
     * @param samplingRate The sampling rate of the sensor.
     */
    SensorHub(uint8_t ws, uint8_t bclock, uint8_t data, uint32_t samplingRate, uint8_t channel);

#endif

    /**
     * @brief Constructs a SensorHub object with the specified I2C address.
     * @param addr The I2C address of the sensor.
     */
    SensorHub(uint8_t addr);

    /**
     * @brief Writes a byte of data to a specified register.
     * @param reg The register address to write to.
     * @param data The byte of data to write.
     */
    void i2c_execute(uint8_t reg, uint8_t data);

    /**
     * @brief Writes a byte of data to a specified register.
     * @param reg The register address to write to.
     * @param data The 16bits of data to write.
     */
    void i2c_execute_16bit(uint8_t reg, uint16_t data);

    /**
     * @brief Reads one or more bytes from a specified register.
     * @param reg The register address to read from.
     * @param data Pointer to store the read data.
     * @param length Number of bytes to read.
     * @returns True if the read operation was successful, false otherwise.
     */
    bool i2c_readByte(uint8_t reg, uint8_t *const data, uint8_t length);

    /**
     * @brief Reads one or more signed bytes from a specified register.
     * @param reg The register address to read from.
     * @param data Pointer to store the read data.
     * @param length Number of bytes to read.
     * @returns True if the read operation was successful, false otherwise.
     */
    bool i2c_readByte(uint8_t reg, int8_t *const data, uint8_t length);

    /**
     * @brief Reads a little-endian multi-bit value from a specified register.
     * @tparam T The data type to read (e.g., uint16_t, uint32_t).
     * @param reg The register address to read from.
     * @param data Pointer to store the read data.
     * @param length Number of bits to read.
     * @returns True if the read operation was successful, false otherwise.
     */
    template <typename T>
    bool i2c_read_Xbit_LE(uint8_t reg, T *const data, uint8_t length);

    /**
     * @brief Reads a big-endian multi-bit value from a specified register.
     * @tparam T The data type to read (e.g., uint16_t, uint32_t).
     * @param reg The register address to read from.
     * @param data Pointer to store the read data.
     * @param length Number of bits to read.
     * @returns True if the read operation was successful, false otherwise.
     */
    template <typename T>
    bool i2c_read_Xbit(uint8_t reg, T *const data, uint8_t length);

    /**
     * @brief Sets the I2C communication mode.
     * @param mode The mode to set.
     */
    void set_i2c_mode(uint8_t mode);

    /**
     * @brief Checks if the sensor is connected over I2C.
     * @returns True if the sensor is detected, false otherwise.
     */
    bool is_sensor_connected();

    /**
     * @brief Begins an I2C transmission to a specified register.
     * @param reg The register address to begin transmission to.
     */
    void startTransmission(uint8_t reg);

    /**
     * @brief Destroys the SensorHub object and releases resources.
     */
    ~SensorHub();

private:
    uint8_t mode = 0;

    // I2C
    uint8_t ADDR = 0x00;

    // I2S
    uint32_t samplingRate = 0;
    uint8_t ws = 0, bclock = 0, data = 0;
};

#endif // __cplusplus
#endif // SensorHub_H