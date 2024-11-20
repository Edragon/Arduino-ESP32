

#define OM_LED 10 // on module led
#define WS_LED 10 // WS2812
#define OB_LED 2 // on board led

#define RF_LK 9 // wireless control, signal to LOW, default and boot HIGH

#define B_LED 4 // channel B Blue
#define G_LED 5 // channel G Blue
#define W_LED 3 // channel W Blue
#define R_LED 8 // channel R Blue

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

#ifndef STASSID
#define STASSID "111"
#define STAPSK  "electrodragon"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

WebServer server(80);

void setup(void) {

  pinMode(OM_LED, OUTPUT);  // on module led
  pinMode(OB_LED, OUTPUT); // on board led

  pinMode(B_LED, OUTPUT); // channel B Blue
  pinMode(G_LED, OUTPUT); // channel G Green
  pinMode(W_LED, OUTPUT); // channel W white
  pinMode(R_LED, OUTPUT); // channel R Red



  digitalWrite(OM_LED, HIGH);

  Serial.begin(115200);

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
    server.send(200, "text/plain", "NWI1124 LED Board!");
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

void loop(void) {
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
  digitalWrite (B_LED, LOW);
  digitalWrite (G_LED, LOW);
  digitalWrite (W_LED, LOW);
  digitalWrite (R_LED, LOW);
  delay(1000);

  digitalWrite(OB_LED, HIGH);
  digitalWrite (B_LED, HIGH);
  digitalWrite (G_LED, HIGH);
  digitalWrite (W_LED, HIGH);
  digitalWrite (R_LED, HIGH);
  delay(1000);

}
