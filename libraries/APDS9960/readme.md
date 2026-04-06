# APDS9960 Arduino Library

The **APDS9960 Arduino Library** is a feature packed driver for interfacing with the APDS9960 sensor, a versatile device capable of ambient light sensing, color detection (RGB), proximity sensing, and gesture recognition. This library provides an easy-to-use interface to configure the sensor and retrieve processed data, making it ideal for Arduino-based projects requiring environmental sensing or touchless interaction.

This library supports a wide range of functionalities, including enabling/disabling sensors, setting sensitivity levels, configuring interrupts, and interpreting gesture data.

## Features
- **Ambient Light Sensing**: Measure ambient light levels and set interrupt thresholds.
- **Color Detection**: Read RGB and clear channel data for color analysis.
- **Proximity Sensing**: Detect nearby objects with adjustable sensitivity and offsets.
- **Gesture Recognition**: Interpret directional gestures (up, down, left, right) with customizable settings.
- **Interrupt Support**: Enable interrupts for light, proximity, and gesture events.
- **Flexible Configuration**: Adjust gain, sensitivity, LED drive, and timing parameters.

## Requirements
- **Hardware**: 
  - Arduino-compatible board (e.g., Uno, Mega, ESP32, etc.)
  - APDS9960 sensor module
- **Software**:
  - Arduino IDE (1.8.x or later)
  - [Wire library](https://www.arduino.cc/en/Reference/Wire) (included with Arduino IDE)
  - `SensorHub.h` (dependency, ensure it’s included in your project)

## Installation
1. **Download the Library**:
   - Clone this repository or download it as a ZIP file:
     ```
     git clone https://github.com/<your-username>/APDS9960-Arduino-Library.git
     ```
   - Alternatively, download the ZIP from the releases page.

2. **Install in Arduino IDE**:
   - Open the Arduino IDE.
   - Go to `Sketch > Include Library > Add .ZIP Library...`.
   - Select the downloaded ZIP file or the folder containing `APDS9960.h` and `APDS9960DEFS.h`.

3. **Verify Dependencies**:
   - Ensure `SensorHub.h` is available in your project or library folder. If not provided, contact the author or check the repository for additional dependencies.

## Usage
### Basic Example
This example initializes the APDS9960 sensor and reads proximity data:

```cpp
#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  if (sensor.begin()) {
    Serial.println("APDS9960 initialized successfully!");
  } else {
    Serial.println("Failed to initialize APDS9960. Check connections.");
  }
  sensor.enableProximitySensing(true); // Enable proximity sensing
}

void loop() {
  uint8_t proximity = sensor.readProximity();
  Serial.print("Proximity: ");
  Serial.println(proximity);
  delay(500);
}
```

### Gesture Detection Example
This example demonstrates gesture recognition:

```cpp
#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  if (sensor.beginAll()) { // Initialize with all features enabled
    Serial.println("APDS9960 ready with all features!");
  }
  sensor.enableGestureSensing(true); // Enable gesture detection
}

void loop() {
  Gesture gesture = sensor.readGesture();
  String direction = sensor.resolveGesture(gesture, 50); // 50% threshold
  if (direction != "") {
    Serial.print("Gesture detected: ");
    Serial.println(direction);
  }
  delay(100);
}
```

### Color Sensing Example
This example reads RGB color data:

```cpp
#include <APDS9960.h>

APDS9960 sensor;

void setup() {
  Serial.begin(9600);
  sensor.begin();
  sensor.enableLightSensing(true); // Enable light sensing
}

void loop() {
  Color color = sensor.readColorData();
  Serial.print("R: "); Serial.print(color.red);
  Serial.print(" G: "); Serial.print(color.green);
  Serial.print(" B: "); Serial.print(color.blue);
  Serial.print(" Clear: "); Serial.println(color.clear);
  delay(1000);
}
```

## API Overview
### Initialization
- `APDS9960()`: Constructor with default I2C address (0x39).
- `bool begin()`: Initialize with default settings.
- `bool beginAll()`: Initialize and enable all sensing and interrupt features.
- `uint8_t getPID()`: Retrieve the product ID of the sensor.
- `bool isConnected()`: Check if the sensor is connected and responding.

### Sensor Control
- `void enableAllSensors(bool state)`: Enable/disable all sensors (light, proximity, gesture).
- `void enableLightSensing(bool state)`: Enable/disable ambient light sensing.
- `void enableProximitySensing(bool state)`: Enable/disable proximity sensing.
- `void enableGestureSensing(bool state)`: Enable/disable gesture sensing.
- `void enableAllInterrupts(bool state)`: Enable/disable all interrupts (light, proximity).
- `void enableLightInterrupt(bool state)`: Enable/disable ambient light interrupt.
- `void enableProximityInterrupt(bool state)`: Enable/disable proximity interrupt.
- `void enableGestureInterrupt(bool state)`: Enable/disable gesture sensing interrupt.
- `void enableWaitTimer(bool state, uint8_t waitTime, bool WLONG)`: Enable/disable wait timer with specified wait time and long wait option.

### Data Retrieval
- `uint8_t readProximity()`: Get proximity value (0-255).
- `Color readColorData()`: Get RGB and clear channel data.
- `Gesture readGesture()`: Get raw gesture data from FIFO.
- `String resolveGesture(Gesture gesture, uint8_t threshold)`: Interpret gesture direction with a percentage threshold.

### Configuration
- `void setLightSensitivity(uint8_t shutterSpeed)`: Adjust light sensor sensitivity (0-255).
- `void correctProximity(int8_t upRight, int8_t downLeft)`: Correct proximity sensor offsets.
- `void setLightSensingInterruptThreshold(uint16_t low, uint16_t high)`: Set light interrupt thresholds.
- `void setProximitySensingInterruptThreshold(uint8_t low, uint8_t high)`: Set proximity interrupt thresholds.
- `void setPersistence(uint8_t light, uint8_t proximity)`: Set persistence for light and proximity interrupts (0-15).
- `void setProximitySensitivity(uint8_t sensitivity)`: Set proximity sensitivity (0-3).
- `void setProximitySensorRange(uint8_t level)`: Set proximity LED drive current level (0-3).
- `void setLightGain(uint8_t gainFactor)`: Set gain factor for light sensing (0-3).
- `void setGestureGain(uint8_t gainFactor)`: Set gain factor for gesture sensing (0-3).
- `void setGestureSensitivity(uint8_t pulseLength, uint8_t pulseCount)`: Set pulse length (0-3) and count (0-63) for gesture detection.
- `void setGestureDetectorMode(uint8_t mode)`: Set gesture detector mode (0-3).

For detailed parameter ranges, refer to `APDS9960.h` and `APDS9960DEFS.h`.

## Troubleshooting
- **Sensor Not Detected**: Check I2C connections (SDA, SCL, VCC, GND) and ensure the address is correct (0x39).
- **Invalid Readings**: Verify sensor configuration (gain, sensitivity) and check for obstructions.
- **Errors**: Enable logging by modifying the private `printLogs` member to `true` for debug output.

## Contributing
Contributions are welcome! Please:
1. Fork the repository.
2. Create a feature branch (`git checkout -b feature/YourFeature`).
3. Commit changes (`git commit -m "Add YourFeature"`).
4. Push to the branch (`git push origin feature/YourFeature`).
5. Open a pull request.

## License
This library is released under the [MIT License](LICENSE). Feel free to use, modify, and distribute it as needed.

## Acknowledgments
- Author: **Saurav Sajeev**
- Built for the Arduino community 