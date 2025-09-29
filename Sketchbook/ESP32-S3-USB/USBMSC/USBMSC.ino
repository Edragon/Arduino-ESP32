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

  // Mount SPIFFS (format on first boot if needed)
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }

  // Start USB device and expose SPIFFS via MTP (file-level access)
  USB.begin();
  MTP.begin();
  MTP.addFilesystem(SPIFFS, "SPIFFS");
}

void loop() {
  // Handle MTP transfers
  MTP.loop();
}
#endif /* ARDUINO_USB_MODE */
