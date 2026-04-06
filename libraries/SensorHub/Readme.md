# SensorHub Library

## Overview
**SensorHub** is a core communication and data processing hub for I2C-based sensors. It provides essential I2C read and write operations, making it easier to integrate various sensor classes for data acquisition and processing. The library supports multi-byte transactions, configurable communication modes, and device connectivity verification. Additionally, it includes I2S support for ADC-based sensors on platforms that support it.

## Features
- **I2C Communication Support**
  - Read and write operations for single and multi-byte transactions.
  - Support for big-endian and little-endian data formats.
  - Configurable I2C communication mode.
  - Device connectivity verification.

- **I2S Communication Support (Only for SOCs that support ADC over I2S)**
  - Configurable word select (WS), bit clock (BCLK), and data pins.
  - Supports stereo, left, and right channel selection.
  - Customizable sampling rate.

## Compatibility
The `SensorHub` library is compatible with:
- **ESP32** (Supports both I2C and I2S communication)
- **Arduino Due** (Supports only I2C communication)
- Other boards with I2C support

## Installation
1. Download the repository or clone it:
   ```sh
   git clone https://github.com/yourusername/SensorHub.git
   ```
2. Copy the `SensorHub` folder into your Arduino libraries directory:
   ```
   Documents/Arduino/libraries/
   ```
3. Restart the Arduino IDE.
4. Include the library in your code:
   ```cpp
   #include <SensorHub.h>
   ```

## Usage
### 1. Initialize an I2C Sensor
To use an I2C-based sensor, initialize the `SensorHub` object with the sensor's I2C address:
```cpp
#include <SensorHub.h>

SensorHub sensor(0x40);  // Example I2C address

void setup() {
    Serial.begin(115200);
    if (sensor.is_sensor_connected()) {
        Serial.println("Sensor connected!");
    } else {
        Serial.println("Sensor not found.");
    }
}

void loop() {
    // Add sensor reading logic here
}
```

### 2. Writing Data to an I2C Register
```cpp
sensor.i2c_execute(0x01, 0xFF); // Write 0xFF to register 0x01
```

### 3. Reading Data from an I2C Register
```cpp
uint8_t data;
sensor.i2c_readByte(0x02, &data, 1);
Serial.print("Register Value: ");
Serial.println(data, HEX);
```

### 4. Reading Multi-Byte Data (Little-Endian)
```cpp
uint16_t value;
sensor.i2c_read_Xbit_LE(0x03, &value, 16);
Serial.println(value);
```

### 5. Using I2S-Based Sensors (ESP32 Only)
To initialize an I2S-based sensor:
```cpp
#ifdef ARDUINO_ARCH_ESP32
SensorHub i2sSensor(25, 26, 27, 16000, I2S_CHANNEL_LEFT);
#endif
```

## API Reference
### **Constructor**
#### **I2C Sensor Initialization**
```cpp
SensorHub(uint8_t addr);
```
- `addr`: I2C address of the sensor.

#### **I2S Sensor Initialization (ESP32 Only)**
```cpp
SensorHub(uint8_t ws, uint8_t bclock, uint8_t data, uint32_t samplingRate, uint8_t channel);
```
- `ws`: Word select pin.
- `bclock`: Bit clock pin.
- `data`: Data pin.
- `samplingRate`: Desired sampling rate.
- `channel`: `I2S_CHANNEL_LEFT`, `I2S_CHANNEL_RIGHT`, or `I2S_CHANNEL_STEREO`.

### **I2C Methods**
#### **Write Operations**
```cpp
void i2c_execute(uint8_t reg, uint8_t data);
void i2c_execute_16bit(uint8_t reg, uint16_t data);
```
- Writes data to the specified register.

#### **Read Operations**
```cpp
bool i2c_readByte(uint8_t reg, uint8_t *data, uint8_t length);
bool i2c_readByte(uint8_t reg, int8_t *data, uint8_t length);
```
- Reads one or more bytes from the given register.

#### **Multi-Byte Read (Little-Endian & Big-Endian)**
```cpp
template <typename T>
bool i2c_read_Xbit_LE(uint8_t reg, T *data, uint8_t length);

template <typename T>
bool i2c_read_Xbit(uint8_t reg, T *data, uint8_t length);
```
- Reads a multi-bit value from the register.

### **I2C Configuration**
```cpp
void set_i2c_mode(uint8_t mode);
```
- Sets the I2C communication mode.

### **Sensor Connectivity Check**
```cpp
bool is_sensor_connected();
```
- Returns `true` if the sensor is detected over I2C.

## License
This library is open-source and available under the MIT License.

## Author
Developed by **Saurav Sajeev**.

