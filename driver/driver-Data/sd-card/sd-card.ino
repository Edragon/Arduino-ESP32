#include "FS.h"
#include "SD.h"
#include "SPI.h"

SPIClass spiSD(HSPI);
#define SD_CS 27

void setup() {
  Serial.begin(115200);
  
  spiSD.begin(14, 12, 13, SD_CS ); //SCK, MISO, MOSI, SS //HSPI1 // hspi.begin(HSPI_CLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);

  if (!SD.begin( SD_CS, spiSD )) {
    Serial.println("Card Mount Failed");
    return;
  }
}

void loop() {
}
