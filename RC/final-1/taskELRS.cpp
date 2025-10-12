#include "taskELRS.h"
#include "taskLogger.h"

void initCRSF() {
  // CRSF serial initialization
  LOG_INFO_ELRS("Initializing CRSF on pins RX:%d TX:%d @ %d baud", PIN_RX, PIN_TX, CRSF_BAUDRATE);
  
  crsfSerial.begin(CRSF_BAUDRATE, SERIAL_8N1, PIN_RX, PIN_TX);
  if (!crsfSerial) {
    while (1) { 
      LOG_ERROR_ELRS("ERROR: Invalid crsfSerial configuration"); 
      delay(1000); 
    }
  }
  crsf.begin(crsfSerial);
  LOG_INFO_ELRS("CRSF initialized - waiting for receiver signal...");
  
  // Test serial data availability
  delay(2000);
  if (crsfSerial.available()) {
    LOG_INFO_ELRS("Serial data detected! Bytes available: %d", crsfSerial.available());
  } else {
    LOG_WARN_ELRS("WARNING: No serial data detected from receiver!");
    LOG_WARN_ELRS("Check: 1) Receiver power, 2) Wiring (RX/TX), 3) Receiver is bound and transmitting");
  }
}

void taskELRS(void* pv) {
  (void)pv;
  static uint32_t lastDebugPrint = 0;
  static uint32_t updateCount = 0;
  static uint32_t packetCount = 0;
  static uint32_t lastPacketCount = 0;
  LOG_INFO_ELRS("Task started");

  // Use raw channel variables ch1..ch8 directly (no mirroring)
  uint16_t ch1 = 0, ch2 = 0, ch3 = 0, ch4 = 0, ch5 = 0, ch6 = 0, ch7 = 0, ch8 = 0;
  
  for (;;) {
    // Call update as frequently as possible - no delay in this loop!
    // The AlfredoCRSF library needs frequent updates to process incoming serial data
    crsf.update();
    updateCount++;
    
    // Sample all channels from CRSF receiver
    ch1 = (uint16_t)crsf.getChannel(1); // roll/steering
    ch2 = (uint16_t)crsf.getChannel(2); // x
    ch3 = (uint16_t)crsf.getChannel(3); // throttle
    ch4 = (uint16_t)crsf.getChannel(4); // x
    ch5 = (uint16_t)crsf.getChannel(5); // relay 5
    ch6 = (uint16_t)crsf.getChannel(6); // servo 1
    ch7 = (uint16_t)crsf.getChannel(7); // servo 2
    ch8 = (uint16_t)crsf.getChannel(8); // relay 6

    
    // uint16_t ch6 = (uint16_t)crsf.getChannel(6); // (not used for now)
    // Update shared control data with motor commands AND servo data
    if (xSemaphoreTake(controlMutex, pdMS_TO_TICKS(1)) == pdTRUE) {
      control.steering = ch1;
      control.throttle = ch3;

      control.servoCh1 = ch6; // New servo 1
      control.servoCh2 = ch7; // New servo 2

      // Map ch5 -> relayCh6 and ch8 -> relayCh7 per request
      control.relayCh6 = ch5; // Relay 6 controlled by ch5
      control.relayCh7 = ch8; // Relay 7 controlled by ch8

      xSemaphoreGive(controlMutex);
      
      // Send latest command to motor task queue (non-blocking) - now includes new servo and relay data
      // Use raw channel mapping: ch3=throttle, ch1=steering, ch6=servo1, ch7=servo2, ch5->relayCh6, ch8->relayCh7
      // ControlData layout after adding relayCh7: throttle, steering, relayCh5, relayCh6, relayCh7, servoCh1, servoCh2, battery_mv
      ControlData motorCmd = { ch3, ch1, 0, ch5, ch8, ch6, ch7, control.battery_mv };
      xQueueOverwrite(motorCmdQueue, &motorCmd);
    }
    
    // Check if we received valid data (non-zero channels)
    if (ch6 != 0 || ch7 != 0 || ch1 != 0 || ch3 != 0 || ch5 != 0 || ch8 != 0) {
      packetCount++;
    }
    
    // Debug print every 2 seconds
    if (millis() - lastDebugPrint >= 2000) {
      LOG_DEBUG_ELRS("Updates/sec: %d | Packets/sec: %d | CH1(steering): %d | CH6: %d | CH7: %d | CH3(throttle): %d | CH5(relay7): %d | CH8(relay8): %d", 
                    updateCount / 2, (packetCount - lastPacketCount) / 2, ch1, ch6, ch7, ch3, ch5, ch8);
      lastDebugPrint = millis();
      lastPacketCount = packetCount;
      updateCount = 0;
    }
    
    // Minimal yield to allow other tasks to run, but keep this tight
    vTaskDelay(pdMS_TO_TICKS(1)); // 1 tick = ~1ms, gives ~1000 updates/sec
  }
}
