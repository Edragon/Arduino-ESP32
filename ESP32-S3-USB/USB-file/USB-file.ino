#ifndef ARDUINO_USB_MODE
#error This ESP32 SoC has no Native USB interface
#elif ARDUINO_USB_MODE == 1
#warning This sketch should be used when USB is in OTG mode
void setup() {}
void loop() {}
#else
#include "USB.h"
#include "MTP.h"
#include <FS.h>
#include <SPIFFS.h>

void setup() {
  Serial.begin(115200);
  delay(10);
  Serial.println("ESP32-S3 USB SPIFFS via MTP");

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }

  USB.begin();
  MTP.begin();
  // Register SPIFFS so the host can browse and modify files directly
  MTP.addFilesystem(SPIFFS, "SPIFFS");

  Serial.println("MTP started: SPIFFS exposed to host");
}

void loop() {
  MTP.loop();
}
#endif /* ARDUINO_USB_MODE */
