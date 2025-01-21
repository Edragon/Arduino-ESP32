# BME280 Arduino I2C

This is a simple and straightforward library for the BME280 temperature, pressure and humidity sensor.

Library is using I2C interface to communicate with the sensor.

The main purpose of this library is to make an easy-to-follow example of communication with BME280 over I2C.

## Current state and limitations
* Configurable BME280 I2C address. Default is `0x76`. Some sensors may have `0x77`.
* Configurable `TwoWires` instance if needed.
* Configurable humidity, temperature and pressure oversampling
    * **Default: x1**
    * For convenience you can use `BME280_OVERSAMPLING_X1`, `BME280_OVERSAMPLING_X2`, `BME280_OVERSAMPLING_X4`, `BME280_OVERSAMPLING_X8`, `BME280_OVERSAMPLING_X16`
* Configurable sensor mode
    * **Default: Normal**
    * Normal, Sleep, Forced modes are supported
* Configurable sensor standby time
    * **Default: 1000**
    * Per BME280 datasheet, 0.5, 62.5, 125, 250, 500, 1000, 10, 20 msec
* Configurable IIR filter
    * **Default: Off**
    * Off, 2, 4, 8, 16

# Installation
Look for `BME280_Arduino_I2C` library in Arduino library manager or PlatformIO library manager.

## Dependencies
None

# Measurements
The library provides a consumer with 3 measurements:
* Temperature: `float`, 2 decimals, `° C`
    * You can convert `° C` to `° F` use `F = 1.8 * C + 32` formula.
* Pressure: `float`, `Pa`
    * You can convert to the preferred unit from Pascals. Example: `mmHg = Pa * 0.00750062`
* Humidity: `uint8_t`, `%`

# Interface
* Constructor supports 2 optional arguments
    * `int bmeAddress`, I2C address of a sensor, default is `0x76`
    * `TwoWire* customTwoWire`, a pointer to the `TwoWire` instance, default to `&Wire`
* `uint8_t begin()` - initialization method. Should be called before any readings can be performed, recommended to call in a `setup` function
    * returns `0` if sensor is found, calibration data is received and configuration is sent to the sensor
    * returns `1` if `TwoWire` request failed
    * returns `2` if sensor is not found
* `bool setHumiditySettings(uint8_t os = BME280_OVERSAMPLING_X1)` - set humidity oversampling
    * `os` can be between 1 and 5 inclusively, where 1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16.
    * returns `false` if arguments are invalid, `true` otherwise
* `bool setGeneralSettings(uint8_t tOS = BME280_OVERSAMPLING_X1, uint8_t pOS = BME280_OVERSAMPLING_X1, uint8_t m = 3)` - sets temperature and pressure oversampling, sensor mode
    * `tOS` can be between 1 and 5 inclusively, where 1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16.
    * `pOS` can be between 1 and 5 inclusively, where 1 - x1, 2 - x2, 3 - x4, 4 - x8, 5 - x16.
    * `m` can be between 0 and 3 inclusively, where 0 - Sleep, 1 and 2 - Forced, 3 - Normal.
    * returns `false` if arguments are invalid, `true` otherwise
* `bool setConfigs(uint8_t tsb = 5, uint8_t filter = 0)` - sets general sensor configuration
    * `tsb` can be between 0 and 7 inclusively, where 0 - 0.5ms, 1 - 62.5ms, 2 - 125ms, 3 - 250ms, 4 - 500ms, 5 - 1s, 6 - 10ms, 7 - 20ms.
    * `filter` can be between 0 and 4 inclusively, where 0 - off, 1 - 2, 2 - 4, 3 - 8, 4 - 16.
    * returns `false` if arguments are invalid, `true` otherwise
* `BME280Data* read()` - reads temperature, humidity and pressure and returns a pointer to the data structure
    * returns `nullptr` if sensor has not been initialized (`begin` method had not been called) or `TwoWire` request failed
    * returns `BME280Data*` with `float temperature`, `float pressure` and `uint8_t humidity` fields if reading is successful. _NOTE: if `read` function is called more frequently than current sensor standby, previous values will be returned_ 

# Usage
Make sure you wire your `SCL` and `SCA` inputs to the supported GPIOs.

## Wiring
TODO
## Code
```cpp
#include <BME280_Arduino_I2C.h>

BME280_Arduino_I2C bme;

void setup() {
    if (bme.begin() == 0) {
        // Initialized
    } else {
        /*
            Failed to initialize:
            Returning code 1: Wire is not available
            Returning code 2: Device has not been found
        */
    }
}

void loop() {
    BME280Data* data = bme.read();

    // nullptr if read failed
     if (data != nullptr) {
        // data->temperature
        // data->humidity
        // data->pressure
    }
}

```

# Support
Tested on Arduino Nano (`nanoatmega328`), ESP32 (`espressif32 esp32dev`). Most likely works on any other boards with `I2C` support.