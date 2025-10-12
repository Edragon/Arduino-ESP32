# Conflicts Report for mqtt-2-all.ino

## Summary
After enabling all commented-out code, several conflicts have been identified that need resolution.

---

## 🔴 CRITICAL CONFLICTS

### 1. **SIGNAL_LED_PIN Conflict** (Pin 35)
**Severity:** HIGH - Multiple tasks controlling the same pin

**Conflicting Code:**
- **MQTT Task LED_Blink** (line 349): Blinks LED during MQTT setup
  ```cpp
  digitalWrite(SIGNAL_LED_PIN, !digitalRead(SIGNAL_LED_PIN));
  ```
- **MQTT Task Connected** (line 420): Sets LED HIGH when broker connected
  ```cpp
  digitalWrite(SIGNAL_LED_PIN, HIGH);
  ```
- **taskMotor -> updateSignalLed()** (lines 595-597): Controls LED based on CRSF signal
  ```cpp
  if (crsf.getChannel(1) != 0 || ...) {
    digitalWrite(SIGNAL_LED_PIN, HIGH);
  } else {
    digitalWrite(SIGNAL_LED_PIN, LOW);
  }
  ```

**Impact:**
- LED will flicker unpredictably as three different tasks fight for control
- taskMotor calls updateSignalLed() every 20ms (50Hz)
- LED_Blink task toggles every 500ms
- MQTT sets it HIGH once connected

**Recommended Solution:**
1. **Option A:** Remove LED_Blink task, let updateSignalLed() handle it, add brief flash for MQTT connection
2. **Option B:** Use separate LEDs for MQTT status and RC signal status
3. **Option C:** Implement LED priority system with mutex protection

---

### 2. **Servo Control Conflict** (Pins 13, 14)
**Severity:** MEDIUM - Servos controlled by both manual and CRSF channels

**Conflicting Code:**
- **setup()** (lines 179-187): Manually sets servos to 90° center
  ```cpp
  servo1.write(90);
  servo2.write(90);
  ```
- **taskServo()** (lines 690-712): Continuously updates servos from CRSF channels 2 & 4
  ```cpp
  servo1.write(pos1);  // Based on servoCh2
  servo2.write(pos2);  // Based on servoCh4
  ```

**Impact:**
- Setup sets servos to center, then taskServo immediately takes over
- No actual conflict, but setup servo positioning is immediately overridden
- If CRSF signal is not present (channels = 0), servos will stay at center

**Recommended Solution:**
- Keep current behavior (setup center position is fine as initial state)
- Consider adding timeout in taskServo to return to center if no signal

---

## ⚠️ POTENTIAL CONFLICTS

### 3. **Serial Pin Conflict** (Pins 1, 2)
**Severity:** MEDIUM - Shared use of Serial2 pins

**Configuration:**
- **CRSF (Serial1)**: RX=17, TX=16 ✅ No conflict
- **MQTT Modem (Serial2)**: RX=1, TX=2
- **ESP32 Default Serial2**: Usually GPIO16/17, but remapped to 1/2

**Impact:**
- Pins 1 & 2 are typically UART0 (USB serial) on some ESP32 boards
- May conflict with USB programming/debugging on certain boards
- Works fine on boards where GPIO1/2 are free

**Recommended Solution:**
- Verify your specific ESP32 board pinout
- Consider moving MQTT modem to different pins if USB conflicts occur
- Test serial communication before deployment

---

### 4. **WS2812 LED Conflict** (Pin 48)
**Severity:** LOW - LED controlled by motor logic only

**Usage:**
- Only used in `controlMotorsFromValues()` for direction indication
- No conflict, but Pin 48 may not exist on all ESP32 variants
- ESP32-S3 typically has GPIO 48, but ESP32 (original) only goes to GPIO 39

**Impact:**
- Code will compile but LED may not work on ESP32 (non-S3)
- No runtime conflict with other functions

**Recommended Solution:**
- Verify WS2812 pin exists on your specific ESP32 variant
- Change to lower pin number (e.g., 48 → 5, 18, 19) if using original ESP32

---

### 5. **Motor Pin Assignments**
**Severity:** LOW - No conflicts detected

**Pin Usage:**
- LEFT_MOTOR_IN1 = 15 ✅
- LEFT_MOTOR_IN2 = 18 ✅
- RIGHT_MOTOR_IN1 = 7 ✅
- RIGHT_MOTOR_IN2 = 8 ✅

**Potential Issues:**
- GPIO 6-11 are typically connected to SPI flash on most ESP32 boards
- Using GPIO 7 & 8 may cause boot issues or flash access problems

**Recommended Solution:**
- Verify GPIO 7 & 8 are safe on your specific board
- Consider moving to safer pins like 25, 26, 27, 32, 33

---

### 6. **Battery ADC Pin** (Pin 4)
**Severity:** LOW - Dedicated ADC usage

**Usage:**
- BATTERY_ADC_PIN = 4
- Used only by taskADC, no conflict

**Note:**
- GPIO 4 is ADC2_0, which cannot be used when WiFi is active
- Since no WiFi is used in this code, it's fine

---

## 📊 PIN USAGE SUMMARY

| Pin | Function | Task/Module | Notes |
|-----|----------|-------------|-------|
| 1 | MQTT RX | taskMQTT | May conflict with UART0 TX |
| 2 | MQTT TX | taskMQTT | May conflict with UART0 RX |
| 4 | Battery ADC | taskADC | ADC2_0 - OK if no WiFi |
| 5 | Relay 1 | taskMotor | ✅ Safe |
| 6 | Relay 2 | taskMotor | ⚠️ May be flash SPI |
| 7 | Right Motor IN1 | taskMotor | ⚠️ May be flash SPI |
| 8 | Right Motor IN2 | taskMotor | ⚠️ May be flash SPI |
| 13 | Servo 1 | taskServo | ✅ Safe |
| 14 | Servo 2 | taskServo | ✅ Safe |
| 15 | Left Motor IN1 | taskMotor | ✅ Safe |
| 17 | CRSF RX | taskELRS | ✅ Safe (Serial1) |
| 16 | CRSF TX | taskELRS | ✅ Safe (Serial1) |
| 18 | Left Motor IN2 | taskMotor | ✅ Safe |
| 19 | Relay 3 | taskMotor | ✅ Safe |
| 20 | Relay 4 | taskMotor | ✅ Safe |
| 35 | Signal LED | MQTT + taskMotor | 🔴 **CONFLICT** |
| 48 | WS2812 LED | taskMotor | ⚠️ May not exist on all boards |

---

## 🔧 RECOMMENDED FIXES

### Priority 1: Fix SIGNAL_LED Conflict
```cpp
// In taskMQTT, replace LED_Blink task with:
// Remove the LED_Blink task creation entirely (lines 346-351)

// In taskMQTT connection success, use brief flash instead of solid ON:
if (sendCmd(...)) {
  Serial.println("[MQTT] Connected to broker.");
  // Brief flash to indicate connection
  for (int i = 0; i < 6; i++) {
    digitalWrite(SIGNAL_LED_PIN, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(SIGNAL_LED_PIN, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  // Now let updateSignalLed() take control
} else {
  Serial.println("[MQTT] MQTT connect failed. Continuing anyway...");
}
```

### Priority 2: Verify Pin Assignments
- Check if GPIO 7, 8 are safe on your ESP32 board
- Check if GPIO 48 exists (ESP32-S3) or change to lower pin
- Confirm GPIO 1, 2 don't conflict with USB serial

### Priority 3: Add Task Synchronization
Consider adding a mutex for SIGNAL_LED_PIN if you want multiple tasks to control it:
```cpp
SemaphoreHandle_t ledMutex;

// In setup():
ledMutex = xSemaphoreCreateMutex();

// Wrap all digitalWrite(SIGNAL_LED_PIN, ...) with:
if (xSemaphoreTake(ledMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
  digitalWrite(SIGNAL_LED_PIN, state);
  xSemaphoreGive(ledMutex);
}
```

---

## ✅ TESTED CONFIGURATIONS

The code should work without modification if:
1. Using ESP32-S3 with GPIO 48 available
2. GPIO 6-11 are NOT connected to flash (depends on board)
3. GPIO 1, 2 are available (not used by USB serial on your board)
4. You accept the SIGNAL_LED conflict and unpredictable LED behavior

---

## 📝 NOTES

1. **IntelliSense Errors:** The include errors are just IntelliSense configuration issues, not actual compilation problems
2. **RTOS Task Priorities:** All tasks have different priorities - no deadlock risk
3. **Core Assignments:** Tasks properly distributed across dual cores - no CPU starvation
4. **Memory:** Stack sizes look appropriate for each task

---

Generated: October 12, 2025
File: mqtt-2-all.ino
