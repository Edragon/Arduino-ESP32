/*
    This sketch shows how to configure different external or internal clock sources for the Ethernet PHY
*/

//#define XTAL_EN1 2
//#define XTAL_EN2 15
#define OP1 4
#define OP2 5

#include <ETH.h>
#include <WiFi.h>
 
/*
     ETH_CLOCK_GPIO0_IN   - default: external clock from crystal oscillator
     ETH_CLOCK_GPIO0_OUT  - 50MHz clock from internal APLL output on GPIO0 - possibly an inverter is needed for LAN8720
     ETH_CLOCK_GPIO16_OUT - 50MHz clock from internal APLL output on GPIO16 - possibly an inverter is needed for LAN8720
     ETH_CLOCK_GPIO17_OUT - 50MHz clock from internal APLL inverted output on GPIO17 - tested with LAN8720
*/
#ifdef ETH_CLK_MODE
#undef ETH_CLK_MODE
#endif

#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN

// Pin# of the enable signal for the external crystal oscillator (-1 to disable for internal APLL source)
#define ETH_POWER_PIN   2

// Type of the Ethernet PHY (LAN8720 or TLK110)
#define ETH_TYPE        ETH_PHY_LAN8720

// I²C-address of Ethernet PHY (0 or 1 for LAN8720, 31 for TLK110)
#define ETH_ADDR        1

// Pin# of the I²C clock signal for the Ethernet PHY
#define ETH_MDC_PIN     23

// Pin# of the I²C IO signal for the Ethernet PHY
#define ETH_MDIO_PIN    18


static bool eth_connected = false;

void WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_ETH_START:
      Serial.println("ETH Started");
      //set eth hostname here
      ETH.setHostname("esp32-ethernet");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("ETH Connected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.print("ETH MAC: ");
      Serial.print(ETH.macAddress());
      Serial.print(", IPv4: ");
      Serial.print(ETH.localIP());
      if (ETH.fullDuplex()) {
        Serial.print(", FULL_DUPLEX");
      }
      Serial.print(", ");
      Serial.print(ETH.linkSpeed());
      Serial.println("Mbps");
      eth_connected = true;
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default:
      break;
  }
}

void testClient(const char * host, uint16_t port) {
  Serial.print("\nconnecting to ");
  Serial.println(host);

  WiFiClient client;
  if (!client.connect(host, port)) {
    Serial.println("connection failed");
    return;
  }
  client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
  while (client.connected() && !client.available());
  while (client.available()) {
    Serial.write(client.read());
  }

  Serial.println("closing connection\n");
  client.stop();
}

void setup() {
//  pinMode(XTAL_EN1, OUTPUT);
//  pinMode(XTAL_EN2, OUTPUT);
  pinMode(OP1, INPUT);
  pinMode(OP2, INPUT);

//  delay(1000);
//  digitalWrite (XTAL_EN1, HIGH);
  //digitalWrite (XTAL_ctrl2, HIGH);
  delay(1000);
  
  Serial.begin(115200);
  Serial.println("ETH setup start ");

  WiFi.onEvent(WiFiEvent);

  if (ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE) == true)
  {
    Serial.println("ETH setup done ");
  }

}


void loop() {
  
  int OP1_status = digitalRead(OP1);
  int OP2_status = digitalRead(OP2);
  
  Serial.print("OP1 status: ");
  Serial.print(OP1_status);
  Serial.print("; OP2 status: ");
  Serial.println(OP2_status);  
  
  //  if (eth_connected) {
  //    testClient("163.com", 80);
  //  }

  testClient("baidu.com", 80);
  // testClient("163.com", 80);
  
  Serial.print("ETH status: ");
  Serial.print(eth_connected);

  Serial.print(", ETH MAC: ");
  Serial.print(ETH.macAddress());
  Serial.print(", IPv4: ");
  Serial.print(ETH.localIP());

  if (ETH.fullDuplex()) {
    Serial.print(", FULL_DUPLEX");
  }
  Serial.print(", ");
  Serial.print(ETH.linkSpeed());
  Serial.println("Mbps");
  Serial.println("");
  delay(10000);
}
