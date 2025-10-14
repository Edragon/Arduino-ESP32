#include "taskMQTT.h"
#include "taskLogger.h"  // Include logger

// ========================= MQTT Task =========================
void taskMQTT(void* pv) {
  (void)pv;

  // Initialize serial for AT command modem
  Serial.printf("[MQTT] Initializing modem serial on RX:%d TX:%d at 115200 baud\n", MODEM_RX_PIN, MODEM_TX_PIN);
  modemSerial.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
  delay(2000); // Increased delay for modem startup
  
  // Test if modem serial is working
  Serial.println("[MQTT] Testing modem serial availability...");
  if (modemSerial.available()) {
    Serial.printf("[MQTT] Modem has %d bytes available\n", modemSerial.available());
  } else {
    Serial.println("[MQTT] WARNING: No data available from modem");
  }
  
  // Initialize SIGNAL_LED_PIN for MQTT status indication
  pinMode(SIGNAL_LED_PIN, OUTPUT);
  digitalWrite(SIGNAL_LED_PIN, HIGH); // Start with LED on (reversed logic)

  LOG_INFO_MQTT("========================================");
  LOG_INFO_MQTT("         MQTT TASK INITIALIZATION      ");
  LOG_INFO_MQTT("========================================");
  LOG_INFO_MQTT("Modem Serial Initialized");

  // Helper function to send a command and wait for a specific response
  auto sendCmd = [&](const char* cmd, const char* expect, uint32_t timeout) -> bool {
    Serial.print("[MQTT CMD] "); Serial.println(cmd);
    
    // Clear any existing data in the buffer
    while (modemSerial.available()) {
      modemSerial.read();
    }
    
    // Send command with explicit line ending
    modemSerial.print(cmd);
    modemSerial.print("\r\n");
    modemSerial.flush(); // Ensure data is sent
    
    uint32_t start = millis();
    uint32_t lastYield = millis();
    uint32_t lastActivity = millis();
    String response = "";
    int bytesReceived = 0;
    
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
        bytesReceived++;
        lastActivity = millis();
        
        if (response.indexOf(expect) != -1) {
          Serial.println();
          Serial.printf("[MQTT RESP] SUCCESS - received %d bytes\n", bytesReceived);
          return true;
        }
        // Check for error responses
        if (response.indexOf("ERROR") != -1 || response.indexOf("+CME ERROR:") != -1 || response.indexOf("+CMS ERROR:") != -1) {
          Serial.println();
          Serial.println("[MQTT RESP] ERROR DETECTED:");
          Serial.println(response);
          return false;
        }
      }
      
      // Break if no activity for 2 seconds but we have some response
      if (bytesReceived > 0 && (millis() - lastActivity > 2000)) {
        break;
      }
    }
    
    Serial.println();
    if (bytesReceived == 0) {
      Serial.println("[MQTT RESP] TIMEOUT - NO RESPONSE FROM MODEM");
      Serial.println("[MQTT DEBUG] Possible issues:");
      Serial.println("  - Modem not powered");
      Serial.println("  - Wrong baud rate");
      Serial.println("  - RX/TX pins swapped");
      Serial.println("  - Modem in wrong mode");
    } else {
      Serial.printf("[MQTT RESP] TIMEOUT - received %d bytes but no expected response\n", bytesReceived);
    }
    Serial.print("[MQTT RESP] Full response: '");
    Serial.print(response);
    Serial.println("'");
    return false;
  };

  // --- MODEM INITIALIZATION PHASE ---
  LOG_INFO_MQTT("--- MODEM INITIALIZATION ---");
  
  if (sendCmd("AT", "OK", 2000)) {
    LOG_INFO_MQTT("✓ Modem responds");
  } else {
    LOG_WARN_MQTT("✗ Modem not responding - continuing anyway");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  LOG_INFO_MQTT("Checking network registration...");
  if (sendCmd("AT+CREG?", "OK", 5000)) {
    LOG_INFO_MQTT("✓ Network status checked");
  } else {
    LOG_WARN_MQTT("✗ Network check failed - continuing anyway");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  LOG_INFO_MQTT("Checking SIM PIN...");
  if (sendCmd("AT+CPIN?", "OK", 5000)) {
    LOG_INFO_MQTT("✓ PIN OK");
  } else {
    LOG_WARN_MQTT("✗ PIN check failed - skipping");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // --- MQTT SERVICE SETUP PHASE ---
  LOG_INFO_MQTT("--- MQTT SERVICE SETUP ---");

  LOG_INFO_MQTT("Enabling extended error reporting...");
  if (sendCmd("AT+CMEE=2", "OK", 3000)) {
    LOG_INFO_MQTT("✓ Extended error reporting enabled");
  } else {
    LOG_ERROR_MQTT("✗ Failed to enable error reporting");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  LOG_INFO_MQTT("Starting MQTT service...");
  if (sendCmd("AT+CMQTTSTART", "OK", 5000)) {
    LOG_INFO_MQTT("✓ MQTT service started");
  } else {
    LOG_ERROR_MQTT("✗ MQTT start command failed - skipping");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  LOG_INFO_MQTT("Checking MQTT service status...");
  if (sendCmd("AT+CMQTTSTART?", "OK", 3000)) {
    LOG_INFO_MQTT("✓ MQTT service status checked");
  } else {
    LOG_ERROR_MQTT("✗ MQTT service status check failed");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  // --- MQTT CLIENT CONFIGURATION PHASE ---
  LOG_INFO_MQTT("--- MQTT CLIENT CONFIGURATION ---");

  LOG_INFO_MQTT("Checking existing MQTT clients...");
  if (sendCmd("AT+CMQTTACCQ?", "OK", 3000)) {
    LOG_INFO_MQTT("✓ Client status checked");
  } else {
    LOG_ERROR_MQTT("✗ Client status check failed");
  }
  vTaskDelay(pdMS_TO_TICKS(500));

  LOG_INFO_MQTT("Acquiring MQTT client...");
  if (sendCmd("AT+CMQTTACCQ=0,\"a12mmmm\"", "OK", 5000)) {
    LOG_INFO_MQTT("✓ MQTT client configured");
  } else {
    LOG_ERROR_MQTT("✗ MQTT client config failed - skipping");
  }
  vTaskDelay(pdMS_TO_TICKS(1000));

  // --- BROKER CONNECTION PHASE ---
  LOG_INFO_MQTT("--- BROKER CONNECTION ---");
  LOG_INFO_MQTT("Connecting to MQTT broker...");
  if (sendCmd("AT+CMQTTCONNECT=0,\"tcp://206.237.31.27:1883\",20,1,\"electrodragon\",\"electrodragon\"", "+CMQTTCONNECT: 0,0", 10000)) {
    LOG_INFO_MQTT("✓ Connected to broker");
  } else {
    LOG_WARN_MQTT("✗ MQTT connect failed - continuing anyway");
  }

  // --- PUBLISHING LOOP ---
  LOG_INFO_MQTT("========================================");
  LOG_INFO_MQTT("         MQTT PUBLISHING LOOP          ");
  LOG_INFO_MQTT("========================================");

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
    
    LOG_INFO_MQTT("--- PUBLISH SEQUENCE START ---");
    LOG_INFO_MQTT("Battery voltage data: %s", payloadMsg);

    // Set Topic
    LOG_DEBUG_MQTT("Setting topic...");
    if (sendCmd("AT+CMQTTTOPIC=0,4", ">", 2000)) {
      if (sendCmd("test", "OK", 2000)) {
        LOG_INFO_MQTT("✓ Topic set");
      } else {
        LOG_ERROR_MQTT("✗ Failed to set topic payload - skipping");
        continue;
      }
    } else {
      LOG_ERROR_MQTT("✗ Failed to enter topic mode - skipping");
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    // Set Payload with battery voltage
    LOG_DEBUG_MQTT("Setting payload...");
    char payloadCmd[32];
    snprintf(payloadCmd, sizeof(payloadCmd), "AT+CMQTTPAYLOAD=0,%d", payloadLen);
    if (sendCmd(payloadCmd, ">", 2000)) {
      if (sendCmd(payloadMsg, "OK", 2000)) {
        LOG_INFO_MQTT("✓ Payload set with battery data");
      } else {
        LOG_ERROR_MQTT("✗ Failed to set payload content - skipping");
        continue;
      }
    } else {
      LOG_ERROR_MQTT("✗ Failed to enter payload mode - skipping");
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    // Publish
    LOG_DEBUG_MQTT("Publishing message...");
    if (sendCmd("AT+CMQTTPUB=0,0,60", "+CMQTTPUB: 0,0", 5000)) {
      LOG_INFO_MQTT("✓ Publish successful");
      digitalWrite(SIGNAL_LED_PIN, LOW); // Turn off LED for successful publish (reversed logic)
      vTaskDelay(pdMS_TO_TICKS(500)); // Keep LED off for 500ms
      digitalWrite(SIGNAL_LED_PIN, HIGH); // Turn on LED
    } else {
      LOG_ERROR_MQTT("✗ Publish failed");
      digitalWrite(SIGNAL_LED_PIN, HIGH); // Ensure LED is on for failure (reversed logic)
    }
    LOG_INFO_MQTT("--- PUBLISH SEQUENCE END ---");
  }
}
