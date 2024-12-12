

#define sensor 0

#define RL_1 6 // relay 1
#define RL_2 7 // relay 2

#define OB_LED 10 // LED

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

#ifndef STASSID
#define STASSID "111" // local wifi spot name
#define STAPSK  "electrodragon"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WebServer server(80);

// the setup function runs once when you press reset or power the board
void setup() {

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(RL_1, OUTPUT);
  pinMode(RL_2, OUTPUT);

  pinMode(OB_LED, OUTPUT);

  Serial.begin(115200);

  digitalWrite(OB_LED, HIGH);
  test_repeat();

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on(F("/"), []() {
    server.send(200, "text/plain", "NWI1126 LED Board!");
  });

  server.on(UriRegex("^\\/io\\/([0-9]+)\\/val\\/([0-9]+)$"), []() {
    String esp_io = server.pathArg(0);
    String esp_val = server.pathArg(1);

    Serial.println(esp_io[0]);
    Serial.println(esp_val[0]);

    Serial.println(esp_io.toInt());
    Serial.println(esp_val.toInt());

    int ctrl_io = esp_io.toInt();
    int ctrl_val = esp_val.toInt();

    // example http://192.168.8.103/io/16/val/1
    //         http://192.168.8.103/io/2/val/0
    // send feedback


    server.send(200, "text/plain", "io: '" + esp_io + "' and val: '" + esp_val + "'");

    digitalWrite(ctrl_io, ctrl_val);

  });

  server.begin();
  Serial.println("HTTP server started");
}



// the loop function runs over and over again forever
void loop() {
  server.handleClient();

}





void test_repeat() {
  Serial.println("Testing LEDs .. ");
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
}


void test_LED () {
  Serial.println("test ..");
  digitalWrite(OB_LED, LOW);
  digitalWrite (RL_1, LOW);
  digitalWrite (RL_2, LOW);
  delay(2000);

  digitalWrite(OB_LED, HIGH);
  digitalWrite (RL_1, HIGH);
  digitalWrite (RL_2, HIGH);
  delay(2000);

}
