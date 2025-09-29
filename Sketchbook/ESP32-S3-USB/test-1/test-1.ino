#include "USB.h"
#include "USBMSC.h"
#include "SPIFFS.h"

USBMSC MSC;

bool msc_read(uint32_t lba, void* buffer, uint32_t bufsize) {
  File f = SPIFFS.open("/disk.bin");
  f.seek(lba * 512);
  f.read((uint8_t*)buffer, bufsize);
  f.close();
  return true;
}

bool msc_write(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  File f = SPIFFS.open("/disk.bin", FILE_WRITE);
  f.seek(lba * 512);
  f.write(buffer, bufsize);
  f.close();
  return true;
}

void setup() {
  SPIFFS.begin(true);

  MSC.vendorID("ESP32S3");
  MSC.productID("FlashDisk");
  MSC.productRevision("1.0");

  MSC.onRead(msc_read);
  MSC.onWrite(msc_write);

  USB.begin();
  MSC.begin();
}

void loop() {}
