#include <SPI.h>
#include <LoRa.h>

//define the pins used by the LoRa transceiver module
#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 15

#define RST 27
#define DI0 21

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6

int counter = 0;

  
SPIClass spiLoRA;
LoRaClass LoRa2;
  
void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Sender");

  spiLoRA.begin(SCK, MISO, MOSI, SS);
  LoRa2.setSPI(spiLoRA);
  LoRa2.setPins(SS, RST, DI0);


  // SPI.begin(SCK, MISO, MOSI, SS);
  // LoRa.setPins(SS, RST, DIO0);

  if (!LoRa2.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void loop() {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa2.beginPacket();
  LoRa2.print("hello ");
  LoRa2.print(counter);
  LoRa2.endPacket();

  counter++;

  delay(5000);
}
