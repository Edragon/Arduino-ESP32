#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Servo.h>
#include <stdint.h>
#ifdef ARDUINO_ARCH_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#endif

// Use Arduino-ESP32 BLE Server (compatible with NimBLE and Bluedroid)
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// BLE UUIDs
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BATTERY_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define THROTTLE_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define STEERING_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// Pins
#define PIN_RX 17
#define PIN_TX 16

#define LEFT_MOTOR_IN1 15
#define LEFT_MOTOR_IN2 18
#define RIGHT_MOTOR_IN1 7
#define RIGHT_MOTOR_IN2 8

#define RELAY1_PIN 5
#define RELAY2_PIN 6
#define RELAY3_PIN 19
#define RELAY4_PIN 20

#define SIGNAL_LED_PIN 35
#define WS2812_PIN 48
#define WS2812_NUM_LEDS 1

// Servo pins
#define SERVO1_PIN 13
#define SERVO2_PIN 14

// Invert servo rotation (1 = invert, 0 = normal)
#define SERVO1_INVERT 0
#define SERVO2_INVERT 1

// Battery sense (adjust to your wiring)
#define BATTERY_ADC_PIN 36
#define BATTERY_DIVIDER_RATIO 4.25f  // Vbatt = Vadc * ratio (390kΩ/120kΩ divider: (390+120)/120 = 4.25)

// Soft start variables
int pwmLeftIn1 = 0, pwmLeftIn2 = 0, pwmRightIn1 = 0, pwmRightIn2 = 0;
const int SOFTSTART_STEP = 8;    // PWM increment per loop
const int SOFTSTART_TARGET = 128; // 50% speed

// Shared control data
struct ControlData {
  uint16_t throttle;   // CRSF ch3
  uint16_t steering;   // CRSF ch1
  uint16_t battery_mv; // battery millivolts
  uint16_t servo_ch2;  // raw/channel or angle for servo on RF channel 2
  uint16_t servo_ch4;  // raw/channel or angle for servo on RF channel 4
};
volatile ControlData control = {0, 0, 0, 0, 0};

// Concurrency primitives
SemaphoreHandle_t controlMutex;
QueueHandle_t motorCmdQueue; // single-element queue carrying latest control snapshot to motor task

// Peripherals
HardwareSerial crsfSerial(1);
AlfredoCRSF crsf;
Adafruit_NeoPixel ws2812(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);
Servo servo1;
Servo servo2;

// ========================= BLE Server (NimBLE/Bluedroid compatible) =========================
BLEServer *pServer = nullptr;
BLECharacteristic *pBatteryCharacteristic = nullptr;
BLECharacteristic *pThrottleCharacteristic = nullptr;
BLECharacteristic *pSteeringCharacteristic = nullptr;

// Forward declarations
void taskELRS(void*);
void taskMotor(void*);
void taskADC(void*);
void taskBLE(void*);

void controlMotorsFromValues(uint16_t roll, uint16_t throttle);
void controlRelays();
void updateSignalLed();
void initServos();
void controlServos(uint16_t ch2, uint16_t ch4);

void setup()
{
  Serial.begin(115200);
  Serial.println("COM Serial initialized");
  
  // CRSF serial
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) while (1) { Serial.println("Invalid crsfSerial configuration"); delay(1000); }
  crsf.begin(crsfSerial);

  // Motor pins setup
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2, OUTPUT);

  // Set all motor pins LOW at startup
  analogWrite(LEFT_MOTOR_IN1, 0);
  analogWrite(LEFT_MOTOR_IN2, 0);
  analogWrite(RIGHT_MOTOR_IN1, 0);
  analogWrite(RIGHT_MOTOR_IN2, 0);

  // Relays
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);

  // Servo pins - initialize with hardware PWM
  initServos();

  // Status LED + WS2812
  pinMode(SIGNAL_LED_PIN, OUTPUT);
  digitalWrite(SIGNAL_LED_PIN, LOW);
  ws2812.begin();
  ws2812.show();

  // ADC config (ESP32): 12-bit default, set attenuation for up to ~3.3V
  #ifdef ARDUINO_ARCH_ESP32
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
  #endif

  // BLE init (standard BLE server - works with both NimBLE and Bluedroid)
  Serial.println("Initializing BLE server...");
  BLEDevice::init("RC-RVR");
  pServer = BLEDevice::createServer();
  
  // Create service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // Create characteristics
  pBatteryCharacteristic = pService->createCharacteristic(
    BATTERY_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pThrottleCharacteristic = pService->createCharacteristic(
    THROTTLE_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  pSteeringCharacteristic = pService->createCharacteristic(
    STEERING_CHAR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // Set initial values
  uint16_t initialValue = 0;
  pBatteryCharacteristic->setValue((uint8_t*)&initialValue, 2);
  pThrottleCharacteristic->setValue((uint8_t*)&initialValue, 2);
  pSteeringCharacteristic->setValue((uint8_t*)&initialValue, 2);
  
  // Start service
  pService->start();
  
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("BLE server started - waiting for connections...");

  // RTOS primitives
  controlMutex = xSemaphoreCreateMutex();
  motorCmdQueue = xQueueCreate(1, sizeof(ControlData)); // latest-sample queue

  // Create tasks (stack sizes tuned conservatively)
  // Core assignment: ELRS/Motor on core 1, ADC/BLE on core 0
  xTaskCreatePinnedToCore(taskELRS,  "ELRS",  4096, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(taskMotor,  "Motor", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(taskADC,    "ADC",   3072, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(taskBLE,    "BLE",   4096, nullptr, 1, nullptr, 0);
}

void loop()
{
  // All work is done in tasks; keep loop idle to let IDLE tasks run (for WDT/housekeeping)
  vTaskDelay(pdMS_TO_TICKS(1000));
}

// ========================= Tasks =========================
void taskELRS(void* pv)
{
  (void)pv;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(5); // process/update at ~200 Hz

  for (;;) {
    // Process CRSF incoming data
    crsf.update();

    // Sample channels of interest
    ControlData snapshot;
    snapshot.steering = (uint16_t)crsf.getChannel(1); // roll
    snapshot.throttle = (uint16_t)crsf.getChannel(3); // throttle
    snapshot.servo_ch2 = (uint16_t)crsf.getChannel(2);
    snapshot.servo_ch4 = (uint16_t)crsf.getChannel(4);

    // Keep last battery in snapshot from shared (optional)
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      snapshot.battery_mv = control.battery_mv;
      // Update global shared state
      control.throttle = snapshot.throttle;
      control.steering = snapshot.steering;
      control.servo_ch2 = snapshot.servo_ch2;
      control.servo_ch4 = snapshot.servo_ch4;
      xSemaphoreGive(controlMutex);
    }

    // Push the latest to motor queue (overwrite if full)
    xQueueOverwrite(motorCmdQueue, &snapshot);

    vTaskDelayUntil(&lastWake, period);
  }
}

void taskMotor(void* pv)
{
  (void)pv;
  TickType_t lastWake = xTaskGetTickCount();
  const TickType_t period = pdMS_TO_TICKS(20); // 50 Hz control loop

  for (;;) {
    // Get latest command (non-blocking); fallback to shared state if none
    ControlData cmd;
    bool haveNew = (xQueueReceive(motorCmdQueue, &cmd, 0) == pdPASS);
    if (!haveNew) {
      if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
        cmd.throttle = control.throttle;
        cmd.steering = control.steering;
        cmd.servo_ch2 = control.servo_ch2;
        cmd.servo_ch4 = control.servo_ch4;
        xSemaphoreGive(controlMutex);
      } else {
        cmd.throttle = 0; cmd.steering = 0; cmd.servo_ch2 = 0; cmd.servo_ch4 = 0;
      }
    }

    // Drive motors using latest values
    controlMotorsFromValues(cmd.steering, cmd.throttle);

    // Control servos from the snapshot (channels 2 and 4)
    controlServos(cmd.servo_ch2, cmd.servo_ch4);

    // Relays + status LED from current CRSF state
    controlRelays();
    updateSignalLed();

    vTaskDelayUntil(&lastWake, period);
  }
}

void taskADC(void* pv)
{
  (void)pv;
  for (;;) {
    // Read battery voltage (mV at ADC pin)
    uint32_t adc_mv = 0;
    #ifdef ARDUINO_ARCH_ESP32
      adc_mv = analogReadMilliVolts(BATTERY_ADC_PIN);
    #else
      int raw = analogRead(BATTERY_ADC_PIN);
      // Fallback conversion if milliVolts API not available (assuming 3.3V ref, 12-bit)
      adc_mv = (uint32_t)((raw * 3300UL) / 4095UL);
    #endif

    uint32_t batt_mv = (uint32_t)(adc_mv * BATTERY_DIVIDER_RATIO);

    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      control.battery_mv = (uint16_t)min(batt_mv, (uint32_t)65535);
      xSemaphoreGive(controlMutex);
    }

    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 Hz
  }
}

void taskBLE(void* pv)
{
  (void)pv;
  for (;;) {
    ControlData snapshot;
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
      snapshot.battery_mv = control.battery_mv;
      snapshot.throttle = control.throttle;
      snapshot.steering = control.steering;
      snapshot.servo_ch2 = control.servo_ch2;
      snapshot.servo_ch4 = control.servo_ch4;
      xSemaphoreGive(controlMutex);
    } else {
      snapshot.battery_mv = 0;
      snapshot.throttle = 0;
      snapshot.steering = 0;
      snapshot.servo_ch2 = 0;
      snapshot.servo_ch4 = 0;
    }

    // Debug output
    Serial.print("[BLE] Battery: ");
    Serial.print(snapshot.battery_mv);
    Serial.print(" mV | Throttle: ");
    Serial.print(snapshot.throttle);
    Serial.print(" | Steering: ");
    Serial.print(snapshot.steering);
    Serial.print(" | Servo CH2: ");
    Serial.print(snapshot.servo_ch2);
    Serial.print(" | Servo CH4: ");
    Serial.println(snapshot.servo_ch4);

    // Update BLE characteristics if server is created
    if (pBatteryCharacteristic && pThrottleCharacteristic && pSteeringCharacteristic) {
      pBatteryCharacteristic->setValue((uint8_t*)&snapshot.battery_mv, 2);
      pBatteryCharacteristic->notify();
      
      pThrottleCharacteristic->setValue((uint8_t*)&snapshot.throttle, 2);
      pThrottleCharacteristic->notify();
      
      pSteeringCharacteristic->setValue((uint8_t*)&snapshot.steering, 2);
      pSteeringCharacteristic->notify();
    }

    vTaskDelay(pdMS_TO_TICKS(500)); // update every 500ms (2 Hz)
  }
}

// ========================= Behavior =========================
void updateSignalLed() {
  // LED on if any channel 1-4 is nonzero
  if (crsf.getChannel(1) != 0 || crsf.getChannel(2) != 0 || crsf.getChannel(3) != 0 || crsf.getChannel(4) != 0) {
    digitalWrite(SIGNAL_LED_PIN, HIGH);
  } else {
    digitalWrite(SIGNAL_LED_PIN, LOW);
  }
}

void controlMotorsFromValues(uint16_t roll, uint16_t throttle)
{
  // Failsafe: if no RC signal (both are zero), stop motors and turn off WS2812
  if (roll == 0 && throttle == 0) {
    pwmLeftIn1 = pwmLeftIn2 = pwmRightIn1 = pwmRightIn2 = 0;
    analogWrite(LEFT_MOTOR_IN1, 0);
    analogWrite(LEFT_MOTOR_IN2, 0);
    analogWrite(RIGHT_MOTOR_IN1, 0);
    analogWrite(RIGHT_MOTOR_IN2, 0);
    ws2812.setPixelColor(0, ws2812.Color(0,0,0));
    ws2812.show();
    return;
  }

  // Center values for CRSF are typically 992-1504-2011
  const int center = 1500;
  const int deadband = 200; // Increased deadband for larger no-control zone

  // Determine direction for each motor
  bool leftForward = false, leftBackward = false;
  bool rightForward = false, rightBackward = false;

  // Forward/backward logic
  if (throttle > center + deadband) {
    leftForward = true;
    rightForward = true;
  } else if (throttle < center - deadband) {
    leftBackward = true;
    rightBackward = true;
  }

  // Steering logic (turn overrides if outside deadband)
  if (roll > center + deadband) {
    // turn right: left motor backward, right motor forward
    leftBackward = true;
    rightForward = true;
    leftForward = false;
    rightBackward = false;
  } else if (roll < center - deadband) {
    // turn left: right motor backward, left motor forward
    rightBackward = true;
    leftForward = true;
    leftBackward = false;
    rightForward = false;
  }

  // Determine target PWM for each pin
  int targetLeftIn1  = leftForward  ? SOFTSTART_TARGET : 0;
  int targetLeftIn2  = leftBackward ? SOFTSTART_TARGET : 0;
  int targetRightIn1 = rightForward ? SOFTSTART_TARGET : 0;
  int targetRightIn2 = rightBackward? SOFTSTART_TARGET : 0;

  // Soft start ramping
  if (pwmLeftIn1  < targetLeftIn1)  pwmLeftIn1  = min(pwmLeftIn1  + SOFTSTART_STEP, targetLeftIn1);  else if (pwmLeftIn1  > targetLeftIn1)  pwmLeftIn1  = max(pwmLeftIn1  - SOFTSTART_STEP, targetLeftIn1);
  if (pwmLeftIn2  < targetLeftIn2)  pwmLeftIn2  = min(pwmLeftIn2  + SOFTSTART_STEP, targetLeftIn2);  else if (pwmLeftIn2  > targetLeftIn2)  pwmLeftIn2  = max(pwmLeftIn2  - SOFTSTART_STEP, targetLeftIn2);
  if (pwmRightIn1 < targetRightIn1) pwmRightIn1 = min(pwmRightIn1 + SOFTSTART_STEP, targetRightIn1); else if (pwmRightIn1 > targetRightIn1) pwmRightIn1 = max(pwmRightIn1 - SOFTSTART_STEP, targetRightIn1);
  if (pwmRightIn2 < targetRightIn2) pwmRightIn2 = min(pwmRightIn2 + SOFTSTART_STEP, targetRightIn2); else if (pwmRightIn2 > targetRightIn2) pwmRightIn2 = max(pwmRightIn2 - SOFTSTART_STEP, targetRightIn2);

  // Apply PWM
  analogWrite(LEFT_MOTOR_IN1,  pwmLeftIn1);
  analogWrite(LEFT_MOTOR_IN2,  pwmLeftIn2);
  analogWrite(RIGHT_MOTOR_IN1, pwmRightIn1);
  analogWrite(RIGHT_MOTOR_IN2, pwmRightIn2);

  // WS2812 LED direction indication
  if (leftForward && rightForward) {
    ws2812.setPixelColor(0, ws2812.Color(0,255,0)); // Green: forward
  } else if (leftBackward && rightBackward) {
    ws2812.setPixelColor(0, ws2812.Color(255,0,0)); // Red: backward
  } else if (leftForward && rightBackward) {
    ws2812.setPixelColor(0, ws2812.Color(255,255,0)); // Yellow: right
  } else if (leftBackward && rightForward) {
    ws2812.setPixelColor(0, ws2812.Color(0,0,255)); // Blue: left
  } else {
    ws2812.setPixelColor(0, ws2812.Color(0,0,0)); // Off
  }
  ws2812.show();
}

void controlRelays()
{
  // If no RC signal (all channels 1-4 are zero), keep all relays OFF (LOW)
  if (crsf.getChannel(1) == 0 && crsf.getChannel(2) == 0 && crsf.getChannel(3) == 0 && crsf.getChannel(4) == 0) {
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    digitalWrite(RELAY3_PIN, LOW);
    digitalWrite(RELAY4_PIN, LOW);
    return;
  }
  // Otherwise, control relays by channels 5-8
  digitalWrite(RELAY1_PIN, crsf.getChannel(5) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY2_PIN, crsf.getChannel(6) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY3_PIN, crsf.getChannel(7) > 1500 ? HIGH : LOW);
  digitalWrite(RELAY4_PIN, crsf.getChannel(8) > 1500 ? HIGH : LOW);
}

// ========================= Servo Control =========================
// Initialize servos using ESP32Servo library
void initServos() {
  // Allow allocation of all timers for servo library
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Set servo PWM frequency (50Hz standard for servos)
  servo1.setPeriodHertz(50);
  servo2.setPeriodHertz(50);
  
  // Attach servos to pins with min/max pulse width in microseconds
  // Standard servo: 500µs (0°) to 2500µs (180°)
  servo1.attach(SERVO1_PIN, 500, 2500);
  servo2.attach(SERVO2_PIN, 500, 2500);
  
  // Center servos at startup (90 degrees)
  servo1.write(90);
  servo2.write(90);

  // Initialize shared servo state to middle (setup runs before RTOS primitives)
  control.servo_ch2 = 90;
  control.servo_ch4 = 90;
}

void controlServos(uint16_t ch2, uint16_t ch4) {
  // Debug raw channel values
  Serial.print("[Servo] raw CH2="); Serial.print(ch2);
  Serial.print(" CH4="); Serial.println(ch4);

  // Update servo1 if there's a valid value
  if (ch2 != 0) {
    int angle1 = map(ch2, 988, 2012, 0, 180);
    angle1 = constrain(angle1, 0, 180);
    if (SERVO1_INVERT) angle1 = 180 - angle1;
    servo1.write(angle1);
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      control.servo_ch2 = (uint16_t)angle1;
      xSemaphoreGive(controlMutex);
    }
  }

  // Update servo2 if there's a valid value
  if (ch4 != 0) {
    int angle2 = map(ch4, 988, 2012, 0, 180);
    angle2 = constrain(angle2, 0, 180);
    if (SERVO2_INVERT) angle2 = 180 - angle2;
    servo2.write(angle2);
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(2)) == pdTRUE) {
      control.servo_ch4 = (uint16_t)angle2;
      xSemaphoreGive(controlMutex);
    }
  }
}

// ========================= BLE helpers =========================
// BLE characteristics are updated in the taskBLE function above