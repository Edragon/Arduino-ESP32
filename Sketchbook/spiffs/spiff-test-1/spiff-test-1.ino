#include <Arduino.h>
#include "SPIFFS.h"
#include "FS.h"
#include "esp_partition.h"


void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\r\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }

  Serial.println("- read from file:");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void printPartitionTable() {
  Serial.println("\n--- Partition Table ---");
  esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  
  while (it != NULL) {
    const esp_partition_t* part = esp_partition_get(it);
    Serial.printf("Label: %-10s Type: 0x%02x SubType: 0x%02x Address: 0x%06x Size: 0x%06x\n",
                  part->label, part->type, part->subtype, part->address, part->size);
    it = esp_partition_next(it);
  }
  esp_partition_iterator_release(it);
}

void checkRawFlashData(uint32_t address, size_t length) {
  Serial.printf("\n--- Raw Flash Data at 0x%06x ---\n", address);
  
  uint8_t buffer[256];
  size_t toRead = length > 256 ? 256 : length;
  
  if (esp_flash_read(NULL, buffer, address, toRead) == ESP_OK) {
    bool allFF = true;
    bool allZero = true;
    
    for (size_t i = 0; i < toRead; i++) {
      if (buffer[i] != 0xFF) allFF = false;
      if (buffer[i] != 0x00) allZero = false;
    }
    
    if (allFF) {
      Serial.println("Flash appears to be erased (all 0xFF)");
    } else if (allZero) {
      Serial.println("Flash appears to be empty (all 0x00)");
    } else {
      Serial.println("Flash contains data. First 64 bytes:");
      for (int i = 0; i < 64 && i < toRead; i += 16) {
        Serial.printf("%06x: ", address + i);
        for (int j = 0; j < 16 && (i + j) < toRead; j++) {
          Serial.printf("%02x ", buffer[i + j]);
        }
        Serial.println();
      }
    }
  } else {
    Serial.println("Failed to read raw flash data");
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }
  Serial.println();
  Serial.println("ESP32 SPIFFS Reader");
  
  // Print partition table first to see what's available
  printPartitionTable();
  
  // Check if SPIFFS partition exists
  const esp_partition_t* spiffs_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, NULL);
  if (spiffs_partition != NULL) {
    Serial.printf("SPIFFS partition found at: 0x%06x, size: 0x%06x\n", spiffs_partition->address, spiffs_partition->size);
    
    // Check what's actually in the flash at this address
    checkRawFlashData(spiffs_partition->address, 1024);
  } else {
    Serial.println("No SPIFFS partition found in partition table!");
  }
  
  // Initialize SPIFFS without formatting (to preserve flashed data)
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed without formatting. Trying with format...");
    if (!SPIFFS.begin(true)) {
      Serial.println("An Error has occurred while mounting SPIFFS");
      Serial.println("This could mean:");
      Serial.println("1. No SPIFFS partition defined in partition table");
      Serial.println("2. SPIFFS image not flashed to correct address");
      Serial.println("3. SPIFFS image is corrupted");
      return;
    } else {
      Serial.println("SPIFFS mounted successfully after formatting (your flashed data was overwritten)");
    }
  } else {
    Serial.println("SPIFFS mounted successfully without formatting");
  }
  
  Serial.println("SPIFFS mounted successfully");
  
  // Get SPIFFS information
  size_t totalBytes = SPIFFS.totalBytes();
  size_t usedBytes = SPIFFS.usedBytes();
  Serial.printf("SPIFFS Total: %u bytes, Used: %u bytes\n", totalBytes, usedBytes);
  
  // List all files and folders in the root directory
  Serial.println("\n--- File and Folder List ---");
  listDir(SPIFFS, "/", 1);
  
  // Read specific file: test.txt
  Serial.println("\n--- Reading test.txt ---");
  if (SPIFFS.exists("/test.txt")) {
    readFile(SPIFFS, "/test.txt");
  } else {
    Serial.println("File /test.txt not found in SPIFFS");
  }
  
  Serial.println("\nDone reading SPIFFS image.");
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

