# BME280_Mini Arduino Library

A lightweight Arduino library for the Bosch BME280 environmental sensor using software I2C implementation. This library allows you to read temperature, pressure, and humidity data without requiring the Wire library, making it ideal for projects where hardware I2C is unavailable or when using non-standard pins.

## Features

- Software I2C implementation - use any digital pins for communication
- No dependency on Wire library
- Low memory footprint
- Simple API for reading environmental data
- Power management functions (sleep/wake)
- Supports both 0x76 and 0x77 I2C addresses

## Hardware Setup

Connect your BME280 sensor to your Arduino as follows:

```
BME280  ->  Arduino
VDD     ->  5V/3.3V (3.3 preferred)
GND     ->  GND
SDI     ->  Any digital pin (SDA)
SCK     ->  Any digital pin (SCL)
```

Note: The BME280 is a 3.3V device. While it does work at 5V, it's recommended to use 3.3V for reliable operation and to prevent damage to the sensor.

## Manual Installation

1. Download the ZIP file of this repository
2. In the Arduino IDE, go to Sketch > Include Library > Add .ZIP Library
3. Select the downloaded ZIP file
4. Restart the Arduino IDE

## Usage

### Basic Example

```cpp
#include <BME280_Mini.h>

// Create BME280_Mini instance
// Parameters: SDA pin, SCL pin, I2C address (default 0x76)
BME280_Mini bme(2, 3);  // Using pins 2 (SDA) and 3 (SCL)

void setup() {
  Serial.begin(9600);
  
  // Initialize the sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor!");
    while (1);
  }
}

void loop() {
  BME280_Mini::Data data;
  
  // Read sensor data
  if (bme.read(data)) {
    Serial.print("Temperature: ");
    Serial.print(data.temperature);
    Serial.println(" °C");
    
    Serial.print("Pressure: ");
    Serial.print(data.pressure);
    Serial.println(" hPa");
    
    Serial.print("Humidity: ");
    Serial.print(data.humidity);
    Serial.println(" %");
  } else {
    Serial.println("Failed to read sensor data!");
  }
  
  delay(2000);
}
```

### Power Management Example

```cpp
#include <BME280_Mini.h>

BME280_Mini bme(2, 3);

void setup() {
  Serial.begin(9600);
  
  if (!bme.begin()) {
    Serial.println("Sensor initialization failed!");
    while (1);
  }
}

void loop() {
  BME280_Mini::Data data;
  
  // Wake up the sensor
  bme.wake();
  
  // Wait for the sensor to be ready
  delay(10);
  
  // Read data
  if (bme.read(data)) {
    Serial.print("Temperature: ");
    Serial.println(data.temperature);
  }
  
  // Put sensor to sleep to save power
  bme.sleep();
  
  // Wait for 5 seconds before next reading
  delay(5000);
}
```

## API Reference

### Constructor

- `BME280_Mini(uint8_t sda, uint8_t scl, uint8_t addr = 0x76)`
  - `sda`: Data pin number
  - `scl`: Clock pin number
  - `addr`: I2C address (default 0x76, some could have 0x77)

### Methods

- `bool begin()`: Initialize the sensor. Returns true if successful.
- `bool sleep()`: Put the sensor into sleep mode. Returns true if successful.
- `bool wake()`: Wake up the sensor from sleep mode. Returns true if successful.
- `bool read(Data& data)`: Read sensor data into the provided Data structure. Returns true if successful.

### Data Structure

```cpp
struct Data {
    float temperature;  // Temperature in degrees Celsius
    float pressure;     // Pressure in hPa
    uint8_t humidity;   // Humidity in percentage
};
```

## Contributing

Contributions to improve the library are welcome. Please submit a pull request or create an issue to discuss proposed changes.

## Credits

Created by Asha Geyon (Natpol50) 🦊 
No other contributions for now

## License

This project is licensed under the Creative Commons Attribution-ShareAlike 4.0 International License (CC BY-SA 4.0) - see the LICENSE file for details. This means you are free to:

Share: Copy and redistribute the material in any medium or format Adapt: Remix, transform, and build upon the material for any purpose, even commercially

Under these terms:

Attribution: You must give appropriate credit, provide a link to the license, and indicate if changes were made ShareAlike: If you modify the material, you must distribute your contributions under the same license as the original
