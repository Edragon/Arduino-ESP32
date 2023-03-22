
#define relay1 6
#define relay2 7
#define led 10


/* Create a WiFi access point and provide a web server on it. */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiAP.h>

#ifndef APSSID
#define APSSID "ESPap"
#define APPSK  "thereisnospoon"
#endif

/* Set these to your desired credentials. */
const char *ssid = APSSID;
const char *password = APPSK;

ESP8266WebServer server(80);

/* Just a little test message.  Go to http://192.168.4.1 in a web browser
   connected to this access point to see it.
*/
void handleRoot() {
  server.send(200, "text/html", "<h1>You are connected</h1>");
}

void blink() {
  server.send(200, "text/html", "<h1>I am blinking ! </h1>");
  digitalWrite(relay1, HIGH);  
  digitalWrite(relay2, HIGH);   
  digitalWrite(led, HIGH);  
  delay(3000);

  digitalWrite(relay1, LOW);   
  digitalWrite(relay2, LOW);   
  digitalWrite(led, LOW);   
  delay(3000);    

}

void setup() {
  
  blink();
  blink();
  
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/blink", blink);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
