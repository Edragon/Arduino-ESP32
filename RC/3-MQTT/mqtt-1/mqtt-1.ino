#include <AlfredoCRSF.h>
#include <HardwareSerial.h>
#include <ESP32Servo.h>
// #include <Adafruit_NeoPixel.h>
#include <stdint.h>
#ifdef ARDUINO_ARCH_ESP32
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#endif

// Use Arduino-ESP32 BLE Server (compatible with NimBLE and Bluedroid)
// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>

// BLE UUIDs
// #define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define BATTERY_CHAR_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define THROTTLE_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26a9"
// #define STEERING_CHAR_UUID  "beb5483e-36e1-4688-b7f5-ea07361b26aa"

// Pins
#define PIN_RX 17
#define PIN_TX 16

// MQTT Modem Pins
#define MODEM_RX_PIN 1
#define MODEM_TX_PIN 2

// #define LEFT_MOTOR_IN1 15
// #define LEFT_MOTOR_IN2 18
// #define RIGHT_MOTOR_IN1 7
// #define RIGHT_MOTOR_IN2 8

// #define RELAY1_PIN 5
// #define RELAY2_PIN 6
// #define RELAY3_PIN 19
// #define RELAY4_PIN 20

#define SIGNAL_LED_PIN 35
// #define WS2812_PIN 48
// #define WS2812_NUM_LEDS 1

// Servo pins
#define SERVO1_PIN 13
#define SERVO2_PIN 14

// Servo objects
Servo servo1;
Servo servo2;

// Servo pulse width ranges (microseconds)
int servo1MinUs = 500;
int servo1MaxUs = 2500;
int servo2MinUs = 500;
int servo2MaxUs = 2500;

// Battery sense (adjust to your wiring)
#define BATTERY_ADC_PIN 4
#define BATTERY_DIVIDER_RATIO 4.25f  // Vbatt = Vadc * ratio (390kΩ/120kΩ divider: (390+120)/120 = 4.25)

// Soft start variables
// int pwmLeftIn1 = 0, pwmLeftIn2 = 0, pwmRightIn1 = 0, pwmRightIn2 = 0;
// const int SOFTSTART_STEP = 8;    // PWM increment per loop
// const int SOFTSTART_TARGET = 128; // 50% speed

// Servo control variables
volatile uint16_t servoCh2 = 1500; // CRSF channel 2 for servo 1
volatile uint16_t servoCh4 = 1500; // CRSF channel 4 for servo 2

// Shared control data
struct ControlData {
  uint16_t throttle;   // CRSF ch3
  uint16_t steering;   // CRSF ch1
  uint16_t battery_mv; // battery millivolts
};
volatile ControlData control = {0, 0, 0};

// Concurrency primitives
SemaphoreHandle_t controlMutex;
QueueHandle_t motorCmdQueue; // single-element queue carrying latest control snapshot to motor task

// Peripherals
HardwareSerial crsfSerial(1);
HardwareSerial modemSerial(2); // New serial for MQTT modem
AlfredoCRSF crsf;
// Adafruit_NeoPixel ws2812(WS2812_NUM_LEDS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// ========================= BLE Server (NimBLE/Bluedroid compatible) =========================
// BLEServer *pServer = nullptr;
// BLECharacteristic *pBatteryCharacteristic = nullptr;
// BLECharacteristic *pThrottleCharacteristic = nullptr;
// BLECharacteristic *pSteeringCharacteristic = nullptr;

// Forward declarations
void taskELRS(void*);
void taskServo(void*);
void taskMQTT(void*); // New task for MQTT
void taskMotor(void*);
void taskADC(void*);
// void taskBLE(void*);

void controlMotorsFromValues(uint16_t roll, uint16_t throttle);
void controlRelays();
void updateSignalLed();

void setup()
{
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor
  Serial.println("\n\n=== RC Rover Control Starting ===");
  Serial.println("COM Serial initialized");
  
  // CRSF serial
  /*
  Serial.print("Initializing CRSF on pins RX:");
  Serial.print(PIN_RX);
  Serial.print(" TX:");
  Serial.print(PIN_TX);
  Serial.print(" @ ");
  Serial.print(CRSF_BAUDRATE);
  Serial.println(" baud");
  
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) {
    while (1) { 
      Serial.println("ERROR: Invalid crsfSerial configuration"); 
      delay(1000); 
    }
  }
  crsf.begin(crsfSerial);
  Serial.println("CRSF initialized - waiting for receiver signal...");
  
  // Test serial data availability
  delay(2000);
  if (crsfSerial.available()) {
    Serial.print("Serial data detected! Bytes available: ");
    Serial.println(crsfSerial.available());
  } else {
    Serial.println("WARNING: No serial data detected from receiver!");
    Serial.println("Check: 1) Receiver power, 2) Wiring (RX/TX), 3) Receiver is bound and transmitting");
  }
  */

  // Motor pins setup
  // pinMode(LEFT_MOTOR_IN1, OUTPUT);
  // pinMode(LEFT_MOTOR_IN2, OUTPUT);
  // pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  // pinMode(RIGHT_MOTOR_IN2, OUTPUT);

  // Set all motor pins LOW at startup
  // analogWrite(LEFT_MOTOR_IN1, 0);
  // analogWrite(LEFT_MOTOR_IN2, 0);
  // analogWrite(RIGHT_MOTOR_IN1, 0);
  // analogWrite(RIGHT_MOTOR_IN2, 0);

  // Relays
  // pinMode(RELAY1_PIN, OUTPUT);
  // pinMode(RELAY2_PIN, OUTPUT);
  // pinMode(RELAY3_PIN, OUTPUT);
  // pinMode(RELAY4_PIN, OUTPUT);
  // digitalWrite(RELAY1_PIN, LOW);
  // digitalWrite(RELAY2_PIN, LOW);
  // digitalWrite(RELAY3_PIN, LOW);
  // digitalWrite(RELAY4_PIN, LOW);

  // Servo setup using ESP32Servo library
  /*
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  
  Serial.print("Attaching servo 1 to pin "); Serial.print(SERVO1_PIN);
  Serial.print(" | Range: "); Serial.print(servo1MinUs);
  Serial.print("-"); Serial.print(servo1MaxUs); Serial.println(" µs");
  servo1.setPeriodHertz(50);    // Standard 50Hz servo
  servo1.attach(SERVO1_PIN, servo1MinUs, servo1MaxUs);
  
  Serial.print("Attaching servo 2 to pin "); Serial.print(SERVO2_PIN);
  Serial.print(" | Range: "); Serial.print(servo2MinUs);
  Serial.print("-"); Serial.print(servo2MaxUs); Serial.println(" µs");
  servo2.setPeriodHertz(50);    // Standard 50Hz servo
  servo2.attach(SERVO2_PIN, servo2MinUs, servo2MaxUs);
  
  // Set servos to center position (90 degrees)
  Serial.println("Setting servos to center position (90°)");
  servo1.write(90);
  servo2.write(90);
  Serial.println("Servos initialized with ESP32Servo library");
  */

  // Status LED + WS2812
  pinMode(SIGNAL_LED_PIN, OUTPUT);
  digitalWrite(SIGNAL_LED_PIN, LOW);
  // ws2812.begin();
  // ws2812.show();

  // ADC config (ESP32): 12-bit default, set attenuation for up to ~3.3V
  #ifdef ARDUINO_ARCH_ESP32
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    Serial.println("Battery ADC initialized on pin 4");
  #endif

  // BLE init (standard BLE server - works with both NimBLE and Bluedroid)
  // Serial.println("Initializing BLE server...");
  // BLEDevice::init("RC-RVR");
  // pServer = BLEDevice::createServer();
  
  // // Create service
  // BLEService *pService = pServer->createService(SERVICE_UUID);
  
  // // Create characteristics
  // pBatteryCharacteristic = pService->createCharacteristic(
  //   BATTERY_CHAR_UUID,
  //   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  // );
  
  // pThrottleCharacteristic = pService->createCharacteristic(
  //   THROTTLE_CHAR_UUID,
  //   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  // );
  
  // pSteeringCharacteristic = pService->createCharacteristic(
  //   STEERING_CHAR_UUID,
  //   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  // );
  
  // // Set initial values
  // uint16_t initialValue = 0;
  // pBatteryCharacteristic->setValue((uint8_t*)&initialValue, 2);
  // pThrottleCharacteristic->setValue((uint8_t*)&initialValue, 2);
  // pSteeringCharacteristic->setValue((uint8_t*)&initialValue, 2);
  
  // // Start service
  // pService->start();
  
  // // Start advertising
  // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(SERVICE_UUID);
  // pAdvertising->setScanResponse(true);
  // pAdvertising->setMinPreferred(0x06);
  // pAdvertising->setMinPreferred(0x12);
  // BLEDevice::startAdvertising();
  // Serial.println("BLE server started - waiting for connections...");

  // RTOS primitives
  controlMutex = xSemaphoreCreateMutex();
  motorCmdQueue = xQueueCreate(1, sizeof(ControlData)); // latest-sample queue
  Serial.println("RTOS mutex and queue created");

  // Create tasks (stack sizes tuned conservatively)
  // Core assignment: ELRS on core 1 (high priority for radio), Servo on core 0, Motor on core 1
  xTaskCreatePinnedToCore(taskELRS,  "ELRS",  4096, nullptr, 4, nullptr, 1);
  xTaskCreatePinnedToCore(taskServo, "Servo", 2048, nullptr, 5, nullptr, 0);
  xTaskCreatePinnedToCore(taskMotor,  "Motor", 4096, nullptr, 3, nullptr, 1);
  xTaskCreatePinnedToCore(taskADC,    "ADC",   3072, nullptr, 1, nullptr, 0);
  // xTaskCreatePinnedToCore(taskBLE,    "BLE",   4096, nullptr, 1, nullptr, 0);
  xTaskCreatePinnedToCore(taskMQTT, "MQTT", 4096, nullptr, 2, nullptr, 0); // MQTT task on core 0
  Serial.println("All tasks created");
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
  /*
  static uint32_t lastDebugPrint = 0;
  static uint32_t updateCount = 0;
  static uint32_t packetCount = 0;
  static uint32_t lastPacketCount = 0;
  */

  for (;;) {
    /*
    // Call update as frequently as possible - no delay in this loop!
    // The AlfredoCRSF library needs frequent updates to process incoming serial data
    crsf.update();
    updateCount++;
    
    // Sample servo channels and store directly to volatile variables
    servoCh2 = (uint16_t)crsf.getChannel(2); // servo 1
    servoCh4 = (uint16_t)crsf.getChannel(4); // servo 2
    
    // Check if we received valid data (non-zero channels)
    if (servoCh2 != 0 || servoCh4 != 0) {
      packetCount++;
    }
    
    // Debug print every 2 seconds
    if (millis() - lastDebugPrint >= 2000) {
      Serial.print("[ELRS] Updates/sec: "); Serial.print(updateCount / 2);
      Serial.print(" | Packets/sec: "); Serial.print((packetCount - lastPacketCount) / 2);
      Serial.print(" | CH2: "); Serial.print(servoCh2);
      Serial.print(" | CH4: "); Serial.println(servoCh4);
      lastDebugPrint = millis();
      lastPacketCount = packetCount;
      updateCount = 0;
    }
    */
    
    // Minimal yield to allow other tasks to run, but keep this tight
    vTaskDelay(pdMS_TO_TICKS(1000)); // 1 tick = ~1ms, gives ~1000 updates/sec
  }
}

// ========================= MQTT Task =========================
void taskMQTT(void* pv) {
  (void)pv;

  // Initialize serial for AT command modem
  modemSerial.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  delay(1000);
  Serial.println("[MQTT] Modem Serial Initialized");

  // Helper function to send a command and wait for a specific response
  auto sendCmd = [&](const char* cmd, const char* expect, uint32_t timeout) -> bool {
    Serial.print("[MQTT] Sending: "); Serial.println(cmd);
    modemSerial.println(cmd);
    
    uint32_t start = millis();
    uint32_t lastYield = millis();
    String response = "";
    while (millis() - start < timeout) {
      // Yield to watchdog every 100ms
      if (millis() - lastYield > 100) {
        vTaskDelay(pdMS_TO_TICKS(1));
        lastYield = millis();
      }
      
      if (modemSerial.available()) {
        char c = modemSerial.read();
        Serial.write(c); // Echo modem response to main serial
        response += c;
        if (response.indexOf(expect) != -1) {
          Serial.println(); // Newline for clarity
          return true;
        }
      }
    }
    Serial.println("\n[MQTT] Timeout!");
    return false;
  };

  // --- One-time setup (skip errors, continue anyway) ---
  // Indicate setup is in progress (slow blink)
  xTaskCreate([](void* pv) {
    for (;;) {
      digitalWrite(SIGNAL_LED_PIN, !digitalRead(SIGNAL_LED_PIN));
      vTaskDelay(pdMS_TO_TICKS(500));
    }
  }, "LED_Blink", 1024, NULL, 1, NULL);

  // Basic AT test
  Serial.println("[MQTT] Testing modem communication...");
  if (sendCmd("AT", "OK", 2000)) {
    Serial.println("[MQTT] Modem responds.");
  } else {
    Serial.println("[MQTT] Modem not responding. Continuing...");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  // Check registration status
  Serial.println("[MQTT] Checking network registration...");
  if (sendCmd("AT+CREG?", "OK", 5000)) {
    Serial.println("[MQTT] Network status checked.");
  } else {
    Serial.println("[MQTT] Network check failed. Continuing...");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  // PIN check (skip if fails)
  Serial.println("[MQTT] Checking SIM PIN...");
  if (sendCmd("AT+CPIN?", "OK", 5000)) {
    Serial.println("[MQTT] PIN OK.");
  } else {
    Serial.println("[MQTT] PIN check failed. Skipping...");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Start MQTT service (some modems need this)
  Serial.println("[MQTT] Starting MQTT service...");
  if (sendCmd("AT+CMQTTSTART", "OK", 5000)) {
    Serial.println("[MQTT] MQTT service started.");
  } else {
    Serial.println("[MQTT] MQTT start command failed. Skipping...");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Stop any existing MQTT client (clean slate)
  Serial.println("[MQTT] Stopping any existing MQTT client...");
  if (sendCmd("AT+CMQTTSTOP", "OK", 5000)) {
    Serial.println("[MQTT] MQTT client stopped.");
  } else {
    Serial.println("[MQTT] MQTT stop command failed. Skipping...");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // Release MQTT client (in case it's still acquired)
  Serial.println("[MQTT] Releasing MQTT client...");
  if (sendCmd("AT+CMQTTREL=0", "OK", 3000)) {
    Serial.println("[MQTT] MQTT client released.");
  } else {
    Serial.println("[MQTT] MQTT release failed. Skipping...");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  // MQTT client config (skip if fails)
  Serial.println("[MQTT] Acquiring MQTT client...");
  if (sendCmd("AT+CMQTTACCQ=0,\"a12mmmm\",0", "OK", 5000)) {
    Serial.println("[MQTT] MQTT client configured.");
  } else {
    Serial.println("[MQTT] MQTT client config failed. Skipping...");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // MQTT broker connect (skip if fails)
  if (sendCmd("AT+CMQTTCONNECT=0,\"tcp://206.237.31.27:1883\",20,1,\"electrodragon\",\"electrodragon\"", "+CMQTTCONNECT: 0,0", 10000)) {
    Serial.println("[MQTT] Connected to broker.");
    digitalWrite(SIGNAL_LED_PIN, HIGH); // Solid ON for connected
  } else {
    Serial.println("[MQTT] MQTT connect failed. Continuing anyway...");
  }

  // --- Publishing loop (skip errors, keep trying) ---
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10000)); // Wait 10 seconds

    // Read battery voltage from shared control data
    uint16_t battery_mv = 0;
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      battery_mv = control.battery_mv;
      xSemaphoreGive(controlMutex);
    }

    // Format payload message with battery voltage
    char payloadMsg[32];
    snprintf(payloadMsg, sizeof(payloadMsg), "Batt:%umV", battery_mv);
    int payloadLen = strlen(payloadMsg);
    
    Serial.println("\n[MQTT] --- Starting publish sequence ---");
    Serial.print("[MQTT] Publishing battery voltage: ");
    Serial.println(payloadMsg);

    // Set Topic (skip if fails)
    if (sendCmd("AT+CMQTTTOPIC=0,4", ">", 2000)) {
      if (sendCmd("test", "OK", 2000)) {
        Serial.println("[MQTT] Topic set.");
      } else {
        Serial.println("[MQTT] Failed to set topic payload. Skipping...");
        continue;
      }
    } else {
      Serial.println("[MQTT] Failed to enter topic mode. Skipping...");
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    // Set Payload with battery voltage
    char payloadCmd[32];
    snprintf(payloadCmd, sizeof(payloadCmd), "AT+CMQTTPAYLOAD=0,%d", payloadLen);
    if (sendCmd(payloadCmd, ">", 2000)) {
      if (sendCmd(payloadMsg, "OK", 2000)) {
        Serial.println("[MQTT] Payload set with battery data.");
      } else {
        Serial.println("[MQTT] Failed to set payload content. Skipping...");
        continue;
      }
    } else {
      Serial.println("[MQTT] Failed to enter payload mode. Skipping...");
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    // Publish (skip if fails)
    if (sendCmd("AT+CMQTTPUB=0,0,60", "+CMQTTPUB: 0,0", 5000)) {
      Serial.println("[MQTT] Publish command successful.");
    } else {
      Serial.println("[MQTT] Publish command failed. Skipping...");
    }
  }
}

void taskMotor(void* pv)
{
  (void)pv;
  Serial.println("[MOTOR] Task started");
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
        xSemaphoreGive(controlMutex);
      } else {
        cmd.throttle = 0; cmd.steering = 0;
      }
    }

    // Drive motors using latest values
    controlMotorsFromValues(cmd.steering, cmd.throttle);

    // Relays + status LED from current CRSF state
    controlRelays();
    updateSignalLed();

    vTaskDelayUntil(&lastWake, period);
  }
}

void taskADC(void* pv)
{
  (void)pv;
  Serial.println("[ADC] Task started");
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
      
      // Debug output
      Serial.print("[ADC] Raw ADC: ");
      Serial.print(adc_mv);
      Serial.print(" mV | Battery: ");
      Serial.print(batt_mv);
      Serial.println(" mV");
    }

    vTaskDelay(pdMS_TO_TICKS(5000)); // Read every 5 seconds
  }
}

// void taskBLE(void* pv)
// {
//   (void)pv;
//   for (;;) {
//     ControlData snapshot;
//     if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
//       snapshot.battery_mv = control.battery_mv;
//       snapshot.throttle = control.throttle;
//       snapshot.steering = control.steering;
//       xSemaphoreGive(controlMutex);
//     } else {
//       snapshot.battery_mv = 0;
//       snapshot.throttle = 0;
//       snapshot.steering = 0;
//     }

//     // Debug output
//     Serial.print("[BLE] Battery: ");
//     Serial.print(snapshot.battery_mv);
//     Serial.print(" mV | Throttle: ");
//     Serial.print(snapshot.throttle);
//     Serial.print(" | Steering: ");
//     Serial.println(snapshot.steering);

//     // Update BLE characteristics if server is created
//     if (pBatteryCharacteristic && pThrottleCharacteristic && pSteeringCharacteristic) {
//       pBatteryCharacteristic->setValue((uint8_t*)&snapshot.battery_mv, 2);
//       pBatteryCharacteristic->notify();
      
//       pThrottleCharacteristic->setValue((uint8_t*)&snapshot.throttle, 2);
//       pThrottleCharacteristic->notify();
      
//       pSteeringCharacteristic->setValue((uint8_t*)&snapshot.steering, 2);
//       pSteeringCharacteristic->notify();
//     }

//     vTaskDelay(pdMS_TO_TICKS(500)); // update every 500ms (2 Hz)
//   }
// }

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
    // pwmLeftIn1 = pwmLeftIn2 = pwmRightIn1 = pwmRightIn2 = 0;
    // analogWrite(LEFT_MOTOR_IN1, 0);
    // analogWrite(LEFT_MOTOR_IN2, 0);
    // analogWrite(RIGHT_MOTOR_IN1, 0);
    // analogWrite(RIGHT_MOTOR_IN2, 0);
    // ws2812.setPixelColor(0, ws2812.Color(0,0,0));
    // ws2812.show();
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
  // int targetLeftIn1  = leftForward  ? SOFTSTART_TARGET : 0;
  // int targetLeftIn2  = leftBackward ? SOFTSTART_TARGET : 0;
  // int targetRightIn1 = rightForward ? SOFTSTART_TARGET : 0;
  // int targetRightIn2 = rightBackward? SOFTSTART_TARGET : 0;

  // Soft start ramping
  // if (pwmLeftIn1  < targetLeftIn1)  pwmLeftIn1  = min(pwmLeftIn1  + SOFTSTART_STEP, targetLeftIn1);  else if (pwmLeftIn1  > targetLeftIn1)  pwmLeftIn1  = max(pwmLeftIn1  - SOFTSTART_STEP, targetLeftIn1);
  // if (pwmLeftIn2  < targetLeftIn2)  pwmLeftIn2  = min(pwmLeftIn2  + SOFTSTART_STEP, targetLeftIn2);  else if (pwmLeftIn2  > targetLeftIn2)  pwmLeftIn2  = max(pwmLeftIn2  - SOFTSTART_STEP, targetLeftIn2);
  // if (pwmRightIn1 < targetRightIn1) pwmRightIn1 = min(pwmRightIn1 + SOFTSTART_STEP, targetRightIn1); else if (pwmRightIn1 > targetRightIn1) pwmRightIn1 = max(pwmRightIn1 - SOFTSTART_STEP, targetRightIn1);
  // if (pwmRightIn2 < targetRightIn2) pwmRightIn2 = min(pwmRightIn2 + SOFTSTART_STEP, targetRightIn2); else if (pwmRightIn2 > targetRightIn2) pwmRightIn2 = max(pwmRightIn2 - SOFTSTART_STEP, targetRightIn2);

  // Apply PWM
  // analogWrite(LEFT_MOTOR_IN1,  pwmLeftIn1);
  // analogWrite(LEFT_MOTOR_IN2,  pwmLeftIn2);
  // analogWrite(RIGHT_MOTOR_IN1, pwmRightIn1);
  // analogWrite(RIGHT_MOTOR_IN2, pwmRightIn2);

  // WS2812 LED direction indication
  // if (leftForward && rightForward) {
  //   ws2812.setPixelColor(0, ws2812.Color(0,255,0)); // Green: forward
  // } else if (leftBackward && rightBackward) {
  //   ws2812.setPixelColor(0, ws2812.Color(255,0,0)); // Red: backward
  // } else if (leftForward && rightBackward) {
  //   ws2812.setPixelColor(0, ws2812.Color(255,255,0)); // Yellow: right
  // } else if (leftBackward && rightForward) {
  //   ws2812.setPixelColor(0, ws2812.Color(0,0,255)); // Blue: left
  // } else {
  //   ws2812.setPixelColor(0, ws2812.Color(0,0,0)); // Off
  // }
  // ws2812.show();
}

void controlRelays()
{
  // If no RC signal (all channels 1-4 are zero), keep all relays OFF (LOW)
  if (crsf.getChannel(1) == 0 && crsf.getChannel(2) == 0 && crsf.getChannel(3) == 0 && crsf.getChannel(4) == 0) {
    // digitalWrite(RELAY1_PIN, LOW);
    // digitalWrite(RELAY2_PIN, LOW);
    // digitalWrite(RELAY3_PIN, LOW);
    // digitalWrite(RELAY4_PIN, LOW);
    return;
  }
  // Otherwise, control relays by channels 5-8
  // digitalWrite(RELAY1_PIN, crsf.getChannel(5) > 1500 ? HIGH : LOW);
  // digitalWrite(RELAY2_PIN, crsf.getChannel(6) > 1500 ? HIGH : LOW);
  // digitalWrite(RELAY3_PIN, crsf.getChannel(7) > 1500 ? HIGH : LOW);
  // digitalWrite(RELAY4_PIN, crsf.getChannel(8) > 1500 ? HIGH : LOW);
}

// ========================= Servo Task =========================
void taskServo(void* pv) {
  (void)pv;
  static uint32_t lastServoDebug = 0;
  Serial.println("[SERVO] Task started");
  
  for (;;) {
    // Read current channel values (atomic reads from volatile)
    uint16_t ch2 = servoCh2;
    uint16_t ch4 = servoCh4;
    
    // Convert CRSF channel values (988-2012) to servo degrees (0-180)
    // Default to center if no signal
    int pos1 = 90; // Center position
    int pos2 = 90; // Center position
    
    if (ch2 >= 988 && ch2 <= 2012) {
      pos1 = map(ch2, 988, 2012, 0, 180);
      pos1 = constrain(pos1, 0, 180);
    }
    
    if (ch4 >= 988 && ch4 <= 2012) {
      pos2 = map(ch4, 988, 2012, 0, 180);
      pos2 = constrain(pos2, 0, 180);
    }
    
    // Apply to servos using ESP32Servo library
    servo1.write(pos1);
    servo2.write(pos2);
    
    // Debug output every 2 seconds
    if (millis() - lastServoDebug >= 2000) {
      Serial.print("[SERVO] CH2: "); Serial.print(ch2);
      Serial.print(" -> Pos1: "); Serial.print(pos1); Serial.print("°");
      Serial.print(" | CH4: "); Serial.print(ch4);
      Serial.print(" -> Pos2: "); Serial.print(pos2); Serial.println("°");
      lastServoDebug = millis();
    }
    
    // Update at 50Hz to match servo frequency
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ========================= BLE helpers (commented out) =========================