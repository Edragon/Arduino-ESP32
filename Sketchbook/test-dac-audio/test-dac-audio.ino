#include <Arduino.h>
#include <WiFi.h>
#include <driver/dac.h>

const char* ssid     = "123";
const char* password = "electrodragon";
const char* host     = "192.168.8.1"; 

WiFiClient client;

hw_timer_t * timer = NULL; 
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED; 

#define BUFFFERMAX 8000

uint8_t dataBuffer[BUFFFERMAX];
int readPointer = 0, writePointer = 1;

bool play = false;

void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  
  // play data: 
  if (play) {
    dac_output_voltage(DAC_CHANNEL_1, dataBuffer[readPointer]);

    readPointer++;
    if (readPointer == BUFFFERMAX) {
      readPointer = 0;
    }

    if ( getAbstand() == 0 ) {
      Serial.println("Buffer underrun!!!");
      play = false;
    }
  }


  portEXIT_CRITICAL_ISR(&timerMux);
}

int getAbstand() {
  int abstand = 0;
  if (readPointer < writePointer ) abstand =  BUFFFERMAX - writePointer + readPointer;
  else if (readPointer > writePointer ) abstand = readPointer - writePointer;
  return abstand;
}

void setup() {
  Serial.begin(115200);

  dac_output_enable(DAC_CHANNEL_1);
  pinMode(33, INPUT_PULLUP);
  pinMode(32, INPUT_PULLUP);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());


  const int port = 4444;
  while (!client.connect(host, port)) {
    Serial.println("connection failed");
    delay(1000);
  }

  timer = timerBegin(0, 2, true); // use a prescaler of 2
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 5000, true);
  timerAlarmEnable(timer);


}

void loop() {
  int abstand = getAbstand();
  if (abstand <= 800) play = true;

  if ( abstand >= 800) {
    client.write( B11111111 ); // send the command to send new data
    
    // read new data: 
    while (client.available() == 0);
    while (client.available() >= 1) {
      uint8_t value = client.read();
      dataBuffer[writePointer] = value;
      writePointer++;
      if (writePointer == BUFFFERMAX) writePointer = 0;
    }

  }

}
