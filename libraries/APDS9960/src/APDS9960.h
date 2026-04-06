#ifndef APDS9960_H
#define APDS9960_H

#include <Arduino.h>
#include "APDS9960DEFS.h"
#include <SensorHub.h>
#include <SensorHubTemplates.h>

#ifdef __cplusplus

struct Color;
struct Gesture;

/**
 * @class APDS9960
 * @brief Driver class for the APDS9960 sensor.
 *
 * This class provides an interface for the APDS9960 sensor, which supports
 * color sensing, proximity detection, ambient light measurement, and gesture recognition.
 * It allows users to configure the sensor and retrieve processed sensor data.
 *
 * @author Saurav Sajeev
 */
class APDS9960
{
public:
    /**
     * @brief Constructs an APDS9960 object using the default I2C address.
     */
    APDS9960();

    /**
     * @brief Initializes the sensor with the default setup.
     * @returns True if initialization is successful, false otherwise.
     */
    bool begin();

    /**
     * @brief Initializes the sensor and enables all sensing and interrupt features.
     * @returns True if initialization is successful, false otherwise.
     */
    bool beginAll();

    /**
     * @brief Retrieves the product ID of the APDS9960 sensor.
     * @returns The product ID.
     */
    uint8_t getPID();

    /**
     * @brief Enables or disables all sensing features (light, proximity, gesture).
     * @param state True to enable, false to disable.
     */
    void enableAllSensors(bool state);

    /**
     * @brief Enables or disables all interrupts (light and proximity).
     * @param state True to enable, false to disable.
     */
    void enableAllInterrupts(bool state);

    /**
     * @brief Enables or disables ambient light sensing.
     * @param state True to enable, false to disable.
     */
    void enableLightSensing(bool state);

    /**
     * @brief Enables or disables proximity sensing.
     * @param state True to enable, false to disable.
     */
    void enableProximitySensing(bool state);

    /**
     * @brief Enables or disables gesture sensing.
     * @param state True to enable, false to disable.
     */
    void enableGestureSensing(bool state);

    /**
     * @brief Enables or disables ambient light sensing interrupt.
     * @param state True to enable, false to disable.
     */
    void enableLightInterrupt(bool state);

    /**
     * @brief Enables or disables proximity sensing interrupt.
     * @param state True to enable, false to disable.
     */
    void enableProximityInterrupt(bool state);

    /**
     * @brief Enables or disables the wait timer.
     * @param state True to enable, false to disable.
     * @param waitTime The wait time value.
     * @param WLONG True for long wait time, false for default.
     */
    void enableWaitTimer(bool state, uint8_t waitTime, bool WLONG);

    /**
     * @brief Checks if the APDS9960 sensor is connected and responding.
     * @returns True if the sensor is detected, false otherwise.
     */
    bool isConnected();

    /**
     * @brief Sets the light sensor sensitivity by adjusting the shutter speed.
     * @param shutterSpeed The shutter speed value (0-255).
     */
    void setLightSensitivity(uint8_t shutterSpeed);

    /**
     * @brief Reads the proximity sensor data.
     * @returns Proximity value (0-255).
     */
    uint8_t readProximity();

    /**
     * @brief Corrects proximity sensor offset values.
     * @param upRight Offset for the upper-right proximity sensor.
     * @param downLeft Offset for the lower-left proximity sensor.
     */
    void correctProximity(int8_t upRight, int8_t downLeft);

    /**
     * @brief Sets the interrupt threshold levels for ambient light sensing.
     * @param low Lower threshold value.
     * @param high Upper threshold value.
     */
    void setLightSensingInterruptThreshold(uint16_t low, uint16_t high);

    /**
     * @brief Sets the interrupt threshold levels for proximity sensing.
     * @param low Lower threshold value.
     * @param high Upper threshold value.
     */
    void setProximitySensingInterruptThreshold(uint8_t low, uint8_t high);

    /**
     * @brief Sets the persistence settings for light and proximity interrupts.
     * @param light Light interrupt persistence (0-15).
     * @param proximity Proximity interrupt persistence (0-15).
     */
    void setPersistence(uint8_t light, uint8_t proximity);

    /**
     * @brief Sets the sensitivity level for proximity sensing.
     * @param sensitivity Sensitivity value (0-3).
     */
    void setProximitySensitivity(uint8_t sensitivity);

    /**
     * @brief Sets the proximity sensor LED drive current level.
     * @param level LED current level (0-3).
     */
    void setProximitySensorRange(uint8_t level);

    /**
     * @brief Sets the gain factor for light sensing.
     * @param gainFactor Gain factor (0-3).
     */
    void setLightGain(uint8_t gainFactor);

    /**
     * @brief Reads the color data (Clear, Red, Green, Blue).
     * @returns A Color object containing the RGB and clear values.
     */
    Color readColorData();

    /**
     * @brief Sets the gain factor for gesture sensing.
     * @param gainFactor Gain factor (0-3).
     */
    void setGestureGain(uint8_t gainFactor);

    /**
     * @brief Sets the pulse length and count for gesture detection.
     * @param pulseLength Pulse length (0-3).
     * @param pulseCount Pulse count (0-63).
     */
    void setGestureSensitivity(uint8_t pulseLength, uint8_t pulseCount);

    /**
     * @brief Sets the mode of the gesture detector.
     * @param mode Gesture detector mode (0-3).
     */
    void setGestureDetectorMode(uint8_t mode);

    /**
     * @brief Enables or disables gesture sensing interrupts.
     * @param state True to enable, false to disable.
     */
    void enableGestureInterrupt(bool state);

    /**
     * @brief Reads raw gesture data from the FIFO buffer.
     * @returns A Gesture object containing the gesture data.
     */
    Gesture readGesture();

    /**
     * @brief Interprets gesture data and resolves it into a named direction.
     * @param gesture The Gesture object containing raw sensor readings.
     * @param threshold Threshold percentage for secondary gestures.
     * @returns A string representing the detected gesture.
     */
    String resolveGesture(Gesture gesture, uint8_t threshold);
	
	/**
     * @brief Clears all previous interrupots.
     */
	void clearInterrupts();

private:
    SensorHub sensorHub;
    uint8_t setupByte = 0x01, ctrl = 0x00, gesCtrl = 0x00;
    bool printLogs = true;

    void printLog(String log);
};

/**
 * @brief Struct representing RGB color data from the sensor.
 */
struct Color
{
public:
    uint16_t red, green, blue, clear;
};

/**
 * @brief Struct representing gesture movement data.
 */
struct Gesture
{
public:
    uint8_t up, down, left, right;
};

#endif // __cplusplus
#endif // APDS9960_H