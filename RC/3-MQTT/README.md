# mqtt-2-all Project Structure

This project implements a complete RC rover control system with MQTT telemetry, motor control, servo control, and battery monitoring using ESP32 and FreeRTOS.

## File Structure

### Main Files
- **mqtt-2-all.ino** - Main program file containing:
  - Setup and initialization
  - taskELRS - CRSF receiver communication
  - taskMotor - Motor and relay control
  - taskServo - Servo control
  - taskADC - Battery voltage monitoring
  - Helper functions (controlMotorsFromValues, controlRelays, updateSignalLed)

### Task Modules
- **taskMQTT.h** - Header file for MQTT task
  - Declares taskMQTT function
  - Defines external references to shared variables
  - Pin definitions for MQTT modem

- **taskMQTT.cpp** - MQTT task implementation
  - AT command communication with MQTT modem
  - Broker connection management
  - Battery voltage publishing every 10 seconds
  - LED status indication

### Configuration Files
- **CONFLICTS_REPORT.md** - Detailed analysis of potential hardware and software conflicts

## Task Architecture

### Core 0 (Application Processor)
- **taskServo** (Priority 5) - 50Hz servo control
- **taskADC** (Priority 1) - Battery monitoring every 5 seconds
- **taskMQTT** (Priority 2) - MQTT communication every 10 seconds

### Core 1 (Protocol Processor)
- **taskELRS** (Priority 4) - High-priority CRSF radio updates (~1000Hz)
- **taskMotor** (Priority 3) - Motor control at 50Hz

## Shared Resources

### Mutexes
- **controlMutex** - Protects ControlData structure

### Queues
- **motorCmdQueue** - Motor command queue (1 element)

### Shared Data
- **ControlData** structure:
  - `throttle` - CRSF channel 3
  - `steering` - CRSF channel 1
  - `battery_mv` - Battery voltage in millivolts

## Pin Assignments

### Communication
- **GPIO 17** - CRSF RX (Serial1)
- **GPIO 16** - CRSF TX (Serial1)
- **GPIO 1** - MQTT Modem RX (Serial2)
- **GPIO 2** - MQTT Modem TX (Serial2)

### Motors
- **GPIO 15** - Left Motor IN1
- **GPIO 18** - Left Motor IN2
- **GPIO 7** - Right Motor IN1
- **GPIO 8** - Right Motor IN2

### Servos
- **GPIO 13** - Servo 1
- **GPIO 14** - Servo 2

### Relays
- **GPIO 5** - Relay 1
- **GPIO 6** - Relay 2
- **GPIO 19** - Relay 3
- **GPIO 20** - Relay 4

### Status Indicators
- **GPIO 35** - Signal LED (⚠️ CONFLICT - see CONFLICTS_REPORT.md)
- **GPIO 48** - WS2812 RGB LED

### Sensors
- **GPIO 4** - Battery ADC (ADC2_0)

## MQTT Configuration

### Broker Settings
- **Host:** tcp://206.237.31.27:1883
- **Client ID:** a12mmmm
- **Username:** electrodragon
- **Password:** electrodragon
- **Topic:** test
- **Publish Interval:** 10 seconds
- **Payload Format:** "Batt:XXXXXmV"

## Building and Uploading

### Required Libraries
- AlfredoCRSF
- ESP32Servo
- Adafruit_NeoPixel

### Arduino IDE Setup
1. Install ESP32 board support
2. Install required libraries via Library Manager
3. Select your ESP32 board variant
4. Configure upload speed (115200 recommended)
5. Upload

### PlatformIO Setup
All files in the same directory will be automatically included in the build.

## Known Issues

### Critical
- **SIGNAL_LED_PIN Conflict** - Pin 35 is controlled by multiple tasks
  - LED_Blink task (MQTT setup)
  - MQTT connection status
  - updateSignalLed (CRSF signal status)
  - See CONFLICTS_REPORT.md for solutions

### Warnings
- GPIO 7, 8 may conflict with SPI flash on some boards
- GPIO 48 doesn't exist on original ESP32 (only ESP32-S3)
- GPIO 1, 2 may conflict with USB serial on some boards

## Debug Output

The system provides extensive serial debug output:
- `[ELRS]` - CRSF receiver status and channel values
- `[SERVO]` - Servo positions and channel mapping
- `[MOTOR]` - Motor control status (via behavior functions)
- `[ADC]` - Battery voltage readings
- `[MQTT]` - AT commands, responses, and connection status

Serial baud rate: **115200**

## Extending the System

### Adding New Tasks
1. Create separate `.h` and `.cpp` files
2. Include the header in `mqtt-2-all.ino`
3. Add task creation in `setup()`
4. Use appropriate RTOS primitives for synchronization

### Modifying MQTT Behavior
Edit `taskMQTT.cpp`:
- Change broker settings at connection
- Modify payload format in publishing loop
- Add new AT commands as needed
- Adjust publish interval timing

### Adding Sensors
1. Add ADC readings in `taskADC()`
2. Update ControlData structure
3. Modify MQTT payload to include new data

## Version Information

- **Created:** October 12, 2025
- **Architecture:** FreeRTOS on ESP32 dual-core
- **MQTT Task:** Separated into external module for maintainability
