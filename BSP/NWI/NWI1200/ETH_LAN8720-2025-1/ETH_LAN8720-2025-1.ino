/*
    This sketch shows the Ethernet event usage

*/

#define OP1 4
#define OP2 5

// Important to be defined BEFORE including ETH.h for ETH.begin() to work.
// Example RMII LAN8720 (Olimex, etc.)
#ifndef ETH_PHY_TYPE
#define ETH_PHY_TYPE  ETH_PHY_LAN8720
#define ETH_PHY_ADDR  1
#define ETH_PHY_MDC   23
#define ETH_PHY_MDIO  18
#define ETH_PHY_POWER 2
#define ETH_CLK_MODE  ETH_CLOCK_GPIO0_IN
#endif

#include <ETH.h>

static bool eth_connected = false;

// WARNING: onEvent is called from a separate FreeRTOS task (thread)!
void onEvent(arduino_event_id_t event) {
  switch (event) {
    case ARDUINO_EVENT_ETH_START:
      Serial.println("ETH Started");
      // The hostname must be set after the interface is started, but needs
      // to be set before DHCP, so set it from the event handler thread.
      ETH.setHostname("esp32-ethernet");
      break;
    case ARDUINO_EVENT_ETH_CONNECTED: Serial.println("ETH Connected"); break;
    case ARDUINO_EVENT_ETH_GOT_IP:
      Serial.println("ETH Got IP");
      Serial.println(ETH);
      eth_connected = true;
      break;
    case ARDUINO_EVENT_ETH_LOST_IP:
      Serial.println("ETH Lost IP");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
      Serial.println("ETH Disconnected");
      eth_connected = false;
      break;
    case ARDUINO_EVENT_ETH_STOP:
      Serial.println("ETH Stopped");
      eth_connected = false;
      break;
    default: break;
  }
}

void testClient(const char *host, uint16_t port) {
  Serial.print("\nconnecting to ");
  Serial.println(host);

  NetworkClient client;
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

  pinMode(OP1, INPUT);
  pinMode(OP2, INPUT);
  Serial.begin(115200);

  Network.onEvent(onEvent);

  if (ETH.begin() == true)
  {
    Serial.println("ETH setup done ");
  }
  //
  //  if (ETH.begin(ETH_PHY_ADDR, ETH_PHY_POWER, ETH_PHY_MDC, ETH_PHY_MDIO, ETH_PHY_TYPE, ETH_CLK_MODE) == true)
  //  {
  //    Serial.println("ETH setup done ");
  //  }
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
