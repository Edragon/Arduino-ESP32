
//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>


//define the pins used by the LoRa transceiver module
#define SCK 14
#define MISO 12
#define MOSI 13
#define SS 15
#define RST 27
#define DIO0 21

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 433E6


int counter = 0;
String LoRaMessage = "test";

//Initialize LoRa module
void startLoRA(){
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    Serial.println("Starting LoRa failed!"); 
  }
  
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}


void sendReadings() {
  LoRa.beginPacket();
  LoRa.print(LoRaMessage);
  LoRa.endPacket();
 
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(9600);
  startLoRA();
}
void loop() {
  sendReadings();
  delay(1000);
}
