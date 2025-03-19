#include "SD_MMC.h"
#include "SPI.h"

#define BUILTIN_LED 4

void setup() {
    Serial.begin(115200);

    // Initialize the SD card
    if (!SD_MMC.begin("/sdcard", true)){
        Serial.println("Failed to mount SD card");
        return;
    }

    // Specify that LED pin
    pinMode(BUILTIN_LED, OUTPUT);
    digitalWrite(BUILTIN_LED, LOW);

    // Check for an SD card
    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }

    // <Put your init code here>
}


void loop() {
  // <Put your main code here, to run repeatedly>
  Serial.println("loop end, 10sec delay");
  delay(10000);
}
