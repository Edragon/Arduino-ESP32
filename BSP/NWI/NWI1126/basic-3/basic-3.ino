#define OB_LED 10 // on module led
#define WS_LED 9 // WS2812 on board, reserved, 

#define addc 0 // ADC pin, reserved 

#define W_LED 4 // white
#define B_LED 5 // blue
#define G_LED 6 // green
#define R_LED 7 //red

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <uri/UriBraces.h>
#include <uri/UriRegex.h>

// Access Point mode configuration
#define AP_SSID "NWI1126-LED-192.168.4.1"     // AP name
#define AP_PASSWORD "electrodragon"    // AP password (min 8 characters)

const char *ap_ssid = AP_SSID;
const char *ap_password = AP_PASSWORD;

WebServer server(80);

// optional: configure PWM parameters
const int LEDC_TIMER_BIT = 8;      // 8‑bit resolution (0–255)
const int LEDC_BASE_FREQ = 5000;   // 5 kHz

// Set to true if LEDs are common-anode (inverted logic: 255=OFF, 0=ON)
const bool LED_INVERTED = false;

// Track PWM state by PIN (pin-based LEDC API in core 3.x)
static uint32_t pwmState_R = 0;
static uint32_t pwmState_G = 0;
static uint32_t pwmState_B = 0;
static uint32_t pwmState_W = 0;

static inline void pwmAttachPin(uint8_t pin) {
  const int ch = ledcAttach(pin, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
  Serial.print("PWM attach pin ");
  Serial.print(pin);
  Serial.print(" -> channel ");
  Serial.println(ch);
}

static inline void pwmWritePin(uint8_t pin, uint32_t duty) {
  uint32_t actualDuty = LED_INVERTED ? (255 - duty) : duty;
  ledcWrite(pin, actualDuty); // core 3.x: pin-based write

  if (pin == R_LED) pwmState_R = duty;
  else if (pin == G_LED) pwmState_G = duty;
  else if (pin == B_LED) pwmState_B = duty;
  else if (pin == W_LED) pwmState_W = duty;
}

static inline bool anyRgbwOn() {
  return pwmState_R > 0 || pwmState_G > 0 || pwmState_B > 0 || pwmState_W > 0;
}

static inline void updateObLed() {
  // Requirement:
  // 1) OB_LED ON while any RGBW channel is ON
  // 2) OB_LED ON when WiFi AP is active
  const bool on = anyRgbwOn() || (WiFi.getMode() == WIFI_AP);
  digitalWrite(OB_LED, on ? HIGH : LOW);
}

// the setup function runs once when you press reset or power the board
void setup() {
  // Start serial FIRST so we can see any errors
  Serial.begin(115200);
  delay(2000);  // wait for serial monitor
  Serial.println("\n\n=== NWI1126 Starting ===");
  Serial.print("LED mode: ");
  Serial.println(LED_INVERTED ? "INVERTED (common-anode)" : "NORMAL (common-cathode)");
  
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(OB_LED, OUTPUT);
  digitalWrite(OB_LED, LOW);
  Serial.println("OB_LED pin configured");

  // Attach PWM to pins (core 3.x)
  Serial.println("\nAttaching PWM pins...");
  pwmAttachPin(R_LED);
  pwmAttachPin(G_LED);
  pwmAttachPin(B_LED);
  pwmAttachPin(W_LED);
  Serial.println("PWM attach done\n");

  // Start LED test (OB_LED will be updated based on RGBW state)
  test_repeat();

  // Configure as Access Point
  Serial.println("\nStarting Access Point mode...");
  WiFi.mode(WIFI_AP);
  
  // Start AP with custom SSID and password
  bool apStarted = WiFi.softAP(ap_ssid, ap_password);
  
  if (apStarted) {
    Serial.println("Access Point started successfully");
    Serial.print("AP SSID: ");
    Serial.println(ap_ssid);
    Serial.print("AP Password: ");
    Serial.println(ap_password);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Failed to start Access Point!");
  }

  // Keep OB_LED on after WiFi AP is active
  updateObLed();

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on(F("/"), []() {
    server.send(200, "text/plain", "NWI1126 LED Board!");
  });

  server.on(UriRegex("^\\/io\\/([0-9]+)\\/val\\/([0-9]+)$"), []() {
    String esp_io = server.pathArg(0);
    String esp_val = server.pathArg(1);

    int ctrl_io = esp_io.toInt();
    int ctrl_val = esp_val.toInt();

    // send feedback
    server.send(200, "text/plain", "io: '" + esp_io + "' and val: '" + esp_val + "'");

    // If user is controlling one of the RGBW pins, use PWM (0/1 => off/on)
    if (ctrl_io == R_LED || ctrl_io == G_LED || ctrl_io == B_LED || ctrl_io == W_LED) {
      pwmWritePin((uint8_t)ctrl_io, ctrl_val ? 255 : 0);
      updateObLed();
      return;
    }

    // Otherwise, fall back to digital write
    digitalWrite(ctrl_io, ctrl_val);
  });

  server.begin();
  Serial.println("HTTP server started");
}


// the loop function runs over and over again forever
void loop() {
  server.handleClient();
  updateObLed();
}


void test_repeat() {
  Serial.println("Testing LEDs .. ");
  test_LED();
}

void test_LED() {
  Serial.println("Brightness ramp R → G → B → W");

  // make sure all off first
  Serial.println("Turning all LEDs OFF...");
  pwmWritePin(R_LED, 0);
  pwmWritePin(G_LED, 0);
  pwmWritePin(B_LED, 0);
  pwmWritePin(W_LED, 0);
  updateObLed();
  delay(1000);

  // ramp parameters
  const int maxBrightness = 255;
  const int step = 8;
  const int stepDelay = 20;

  Serial.println("RED ramp up");
  for (int v = 0; v <= maxBrightness; v += step) {
    pwmWritePin(R_LED, v);
    updateObLed();
    delay(stepDelay);
  }
  Serial.println("RED at max");
  delay(500);

  Serial.println("GREEN ramp up");
  for (int v = 0; v <= maxBrightness; v += step) {
    pwmWritePin(G_LED, v);
    updateObLed();
    delay(stepDelay);
  }
  Serial.println("GREEN at max");
  delay(500);

  Serial.println("BLUE ramp up");
  for (int v = 0; v <= maxBrightness; v += step) {
    pwmWritePin(B_LED, v);
    updateObLed();
    delay(stepDelay);
  }
  Serial.println("BLUE at max");
  delay(500);

  Serial.println("WHITE ramp up");
  for (int v = 0; v <= maxBrightness; v += step) {
    pwmWritePin(W_LED, v);
    updateObLed();
    delay(stepDelay);
  }
  Serial.println("WHITE at max");
  delay(500);

  Serial.println("Blink all RGBW 5 times");
  const int blinkCount = 5;
  const int blinkDelayMs = 2000;

  for (int i = 0; i < blinkCount; i++) {
    Serial.print("Blink ");
    Serial.print(i + 1);
    Serial.println(" - ON");
    pwmWritePin(R_LED, maxBrightness);
    pwmWritePin(G_LED, maxBrightness);
    pwmWritePin(B_LED, maxBrightness);
    pwmWritePin(W_LED, maxBrightness);
    updateObLed();
    delay(blinkDelayMs);

    Serial.print("Blink ");
    Serial.print(i + 1);
    Serial.println(" - OFF");
    pwmWritePin(R_LED, 0);
    pwmWritePin(G_LED, 0);
    pwmWritePin(B_LED, 0);
    pwmWritePin(W_LED, 0);
    updateObLed();
    delay(blinkDelayMs);
  }

  Serial.println("Test complete - all OFF");
  updateObLed();
}
