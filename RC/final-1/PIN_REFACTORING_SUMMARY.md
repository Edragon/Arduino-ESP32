# Pin Configuration Refactoring Summary

## Overview
All duplicate pin definitions have been consolidated into a single shared header file `PinsConfig.h`.

## Created File

### **PinsConfig.h**
Centralized pin configuration file containing all hardware pin definitions and constants:

- **CRSF/ELRS Serial Pins**: PIN_RX (17), PIN_TX (16)
- **MQTT Modem Pins**: MODEM_RX_PIN (1), MODEM_TX_PIN (2)
- **Motor Control Pins**: LEFT_MOTOR_IN1/2, RIGHT_MOTOR_IN1/2
- **Relay Pins**: RELAY1_PIN through RELAY4_PIN
- **LED Pins**: SIGNAL_LED_PIN (35), WS2812_PIN (48)
- **Servo Pins**: SERVO1_PIN (13), SERVO2_PIN (14)
- **ADC Configuration**: BATTERY_ADC_PIN (4), BATTERY_DIVIDER_RATIO (4.25f)
- **Motor Constants**: SOFTSTART_STEP (8), SOFTSTART_TARGET (128)

## Modified Files

### **mqtt-2-all.ino**
- ✅ Added `#include "PinsConfig.h"` at the top
- ✅ Removed 25+ lines of duplicate pin definitions
- ✅ Removed duplicate constant definitions (SOFTSTART_STEP, SOFTSTART_TARGET, BATTERY_ADC_PIN, BATTERY_DIVIDER_RATIO)
- ✅ File reduced from ~220 lines to ~195 lines

### **taskMQTT.h**
- ✅ Added `#include "PinsConfig.h"`
- ✅ Removed duplicate definitions: MODEM_RX_PIN, MODEM_TX_PIN, SIGNAL_LED_PIN

### **taskMotor.h**
- ✅ Added `#include "PinsConfig.h"`
- ✅ Removed duplicate motor pin definitions (8 pins)
- ✅ Removed duplicate relay pin definitions (4 pins)
- ✅ Removed duplicate SIGNAL_LED_PIN
- ✅ Removed duplicate SOFTSTART_STEP and SOFTSTART_TARGET constants

### **taskServo.h**
- ✅ Added `#include "PinsConfig.h"`
- ✅ Removed duplicate servo pin definitions: SERVO1_PIN, SERVO2_PIN

### **taskADC.h**
- ✅ Added `#include "PinsConfig.h"`
- ✅ Removed duplicate ADC pin and ratio definitions

## Benefits

1. **Single Source of Truth**: All pin definitions in one place
2. **Easier Maintenance**: Change a pin once, updates everywhere
3. **No Conflicts**: Eliminates risk of mismatched pin definitions
4. **Cleaner Code**: Removed ~50 lines of duplicate definitions across all files
5. **Better Organization**: Clear separation between configuration and implementation
6. **Compile Safety**: Changing pins in one place ensures consistency

## Usage

To change any pin assignment, simply edit `PinsConfig.h` and all tasks will automatically use the new configuration.

## File Structure

```
mqtt-2-all/
├── PinsConfig.h         ← NEW: Centralized pin configuration
├── ControlData.h        ← Shared data structures
├── mqtt-2-all.ino       ← Main file (now includes PinsConfig.h)
├── taskELRS.h/cpp       ← CRSF receiver task
├── taskMQTT.h/cpp       ← MQTT communication (uses PinsConfig.h)
├── taskMotor.h/cpp      ← Motor control (uses PinsConfig.h)
├── taskServo.h/cpp      ← Servo control (uses PinsConfig.h)
├── taskADC.h/cpp        ← ADC monitoring (uses PinsConfig.h)
└── taskBLE.h/cpp        ← BLE telemetry (optional)
```

## Migration Complete ✅

All duplicate definitions have been removed and the code is now using a single, centralized configuration file.
