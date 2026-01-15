// add IO35 == LED, IO48 == WS2812 to code, and apply functions: LED for motors, IO48 for servos 

// for my servo, 90 degree is stop, 180 degree and 0 degree are two directions, set servo default output pwm at 90 degree for stop

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include <Adafruit_NeoPixel.h>

// ===== WiFi 配置 =====
const char* ssid = "motor_control";
const char* password = "electrodragon";

// ===== DRV8871 / GPIO 定义 (ESP32) =====
// DRV8871 has two inputs IN1 and IN2. Set both to control direction.
// Forward: IN1=HIGH, IN2=LOW
// Reverse: IN1=LOW, IN2=HIGH
// Stop: IN1=LOW, IN2=LOW (or both HIGH for brake)
const int IN1_PIN = 15;   // IN1 -> IO15
const int IN2_PIN = 18;   // IN2 -> IO18

// Servo pins (GPIO numbers)
const int SERVO1_PIN = 11;
const int SERVO2_PIN = 12;
const int SERVO3_PIN = 13;
const int SERVO4_PIN = 14;

// Relay pins (GPIO numbers) for ON/OFF control
const int RELAY1_PIN = 5;
const int RELAY2_PIN = 6;
const int RELAY3_PIN = 19;
const int RELAY4_PIN = 20;

// LED pin for motor status (active LOW - on when LOW)
const int MOTOR_STATUS_PIN = 35; // IO35 LED output

// New: WS2812 (NeoPixel) for servo status
const int WS2812_PIN = 48; // user requested IO48
const int NUM_PIXELS = 1;
Adafruit_NeoPixel pixels(NUM_PIXELS, WS2812_PIN, NEO_GRB + NEO_KHZ800);

// ===== Web Server =====
WebServer server(80);

// Servo objects
Servo servo1;
Servo servo2;
Servo servo3;
Servo servo4;

// ===== HTML Web 控制界面 =====
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Motor & Relay Control</title>
<style>
body { font-family: Arial, sans-serif; max-width: 640px; margin: 0 auto; padding: 20px; background-color: #f0f0f0; }
h2 { text-align:center; color:#333; }
.container { display:flex; flex-direction:column; gap:14px; }
.control-panel { background:white; padding:20px; border-radius:10px; box-shadow:0 2px 8px rgba(0,0,0,0.08); display:flex; gap:10px; justify-content:center; }
.servo-panel { background:white; padding:14px; border-radius:10px; box-shadow:0 2px 8px rgba(0,0,0,0.04); display:flex; gap:8px; justify-content:center; }
.relay-panel { background:white; padding:14px; border-radius:10px; box-shadow:0 2px 8px rgba(0,0,0,0.04); display:flex; gap:8px; flex-wrap:wrap; justify-content:center; }
.dir-btn { padding:16px 22px; font-size:18px; font-weight:bold; border:none; border-radius:8px; cursor:pointer; }
.sml-btn { padding:10px 14px; font-size:14px; font-weight:bold; border:none; border-radius:6px; cursor:pointer; }
.relay-btn { padding:10px 12px; font-size:14px; font-weight:bold; border:none; border-radius:6px; cursor:pointer; }
.forward-btn { background:#2196F3; color:white; }
.reverse-btn { background:#FF9800; color:white; }
.stop-btn { background:#f44336; color:white; }
.s90-btn { background:#4CAF50; color:white; }
.s180-btn { background:#9C27B0; color:white; }
.on-btn { background:#4CAF50; color:white; }
.off-btn { background:#f44336; color:white; }
.dir-btn:active, .sml-btn:active, .relay-btn:active { transform:scale(0.97); opacity:0.9; }
.relay-item { display:flex; gap:6px; align-items:center; }
@media (max-width:420px){ .control-panel{flex-direction:column;} }
</style>
</head>
<body>
<h2>Motor & Relay Control</h2>
<div class="container">
  <div class="control-panel">
    <button class="dir-btn forward-btn" onclick="sendCmd('fw')">Forward</button>
    <button class="dir-btn stop-btn" onclick="sendCmd('st')">STOP</button>
    <button class="dir-btn reverse-btn" onclick="sendCmd('rv')">Reverse</button>
  </div>
  <div class="servo-panel">
    <div style="align-self:center;font-weight:bold">Servo14:</div>
    <button class="sml-btn s90-btn" onclick="sendCmd('s90')">90°</button>
    <button class="sml-btn s180-btn" onclick="sendCmd('s180')">180°</button>
    <button class="sml-btn stop-btn" onclick="sendCmd('s0')">0°</button>
  </div>
  <div class="relay-panel">
    <div class="relay-item"><div style="font-weight:bold">Relay1:</div><button class="relay-btn on-btn" onclick="sendCmd('r1_on')">ON</button><button class="relay-btn off-btn" onclick="sendCmd('r1_off')">OFF</button></div>
    <div class="relay-item"><div style="font-weight:bold">Relay2:</div><button class="relay-btn on-btn" onclick="sendCmd('r2_on')">ON</button><button class="relay-btn off-btn" onclick="sendCmd('r2_off')">OFF</button></div>
    <div class="relay-item"><div style="font-weight:bold">Relay3:</div><button class="relay-btn on-btn" onclick="sendCmd('r3_on')">ON</button><button class="relay-btn off-btn" onclick="sendCmd('r3_off')">OFF</button></div>
    <div class="relay-item"><div style="font-weight:bold">Relay4:</div><button class="relay-btn on-btn" onclick="sendCmd('r4_on')">ON</button><button class="relay-btn off-btn" onclick="sendCmd('r4_off')">OFF</button></div>
  </div>
</div>
<script>
function sendCmd(cmd){
  fetch('/control?cmd='+cmd)
    .then(r=>{})
    .catch(e=>console.error(e));
}
</script>
</body></html>
)rawliteral";

// ===== 处理 Web 请求 =====
void handleRoot() {
  server.send(200, "text/html", html_page);
}

void handleControl() {
  // Simple command-based control: cmd=fw|rv|st|s90|s180|s0|rX_on|rX_off
  if (server.hasArg("cmd")) {
    String c = server.arg("cmd");
    if (c == "fw") {
      // Forward: IN1=HIGH, IN2=LOW
      digitalWrite(IN1_PIN, HIGH);
      digitalWrite(IN2_PIN, LOW);
      Serial.println("Motor: Forward");
      // Turn on motor status LED (active LOW)
      digitalWrite(MOTOR_STATUS_PIN, LOW);
    } else if (c == "rv") {
      // Reverse: IN1=LOW, IN2=HIGH
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, HIGH);
      Serial.println("Motor: Reverse");
      // Turn on motor status LED (active LOW)
      digitalWrite(MOTOR_STATUS_PIN, LOW);
    } else if (c == "st") {
      // Stop: both LOW
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, LOW);
      Serial.println("Motor: Stop");
      // Turn off motor status LED (active LOW, so HIGH = OFF)
      digitalWrite(MOTOR_STATUS_PIN, HIGH);
    } else if (c == "s90") {
      // Servo4 to 90 degrees
      servo4.write(90);
      Serial.println("Servo14: 90°");
      // indicate servo action on WS2812 (green)
      setServoPixel(0, 200, 0);
    } else if (c == "s180") {
      // Servo4 to 180 degrees
      servo4.write(180);
      Serial.println("Servo14: 180°");
      // indicate servo action on WS2812 (purple)
      setServoPixel(150, 0, 150);
    } else if (c == "s0") {
      // Servo4 to 0 degrees (direction)
      servo4.write(0);
      Serial.println("Servo14: 0°");
      // turn pixel off
      setServoPixel(0, 0, 0);
    } else if (c == "r1_on") {
      digitalWrite(RELAY1_PIN, HIGH);
      Serial.println("Relay1: ON");
    } else if (c == "r1_off") {
      digitalWrite(RELAY1_PIN, LOW);
      Serial.println("Relay1: OFF");
    } else if (c == "r2_on") {
      digitalWrite(RELAY2_PIN, HIGH);
      Serial.println("Relay2: ON");
    } else if (c == "r2_off") {
      digitalWrite(RELAY2_PIN, LOW);
      Serial.println("Relay2: OFF");
    } else if (c == "r3_on") {
      digitalWrite(RELAY3_PIN, HIGH);
      Serial.println("Relay3: ON");
    } else if (c == "r3_off") {
      digitalWrite(RELAY3_PIN, LOW);
      Serial.println("Relay3: OFF");
    } else if (c == "r4_on") {
      digitalWrite(RELAY4_PIN, HIGH);
      Serial.println("Relay4: ON");
    } else if (c == "r4_off") {
      digitalWrite(RELAY4_PIN, LOW);
      Serial.println("Relay4: OFF");
    }
  }

  server.send(200, "text/plain", "OK");
}

// helper: set single pixel color
void setServoPixel(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

// ===== 初始化 =====
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 DRV8871 AP Motor Controller starting...");
  
  // Configure GPIO pins as outputs
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Relay pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(RELAY4_PIN, OUTPUT);

  // Motor status LED pin as output
  pinMode(MOTOR_STATUS_PIN, OUTPUT);

  // New: init NeoPixel for servo status
  pixels.begin();
  pixels.show(); // clear

  // Default: stop motor and relays OFF (LOW)
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(RELAY3_PIN, LOW);
  digitalWrite(RELAY4_PIN, LOW);
  
  // Motor status LED OFF (active LOW, so HIGH = OFF)
  digitalWrite(MOTOR_STATUS_PIN, HIGH);

  // Attach servos and set to stop position (90°)
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);
  servo4.attach(SERVO4_PIN);

  // Set servos to stop position (90°) by default
  servo1.write(90);
  servo2.write(90);
  servo3.write(90);
  servo4.write(90);

  // 启动为 WiFi AP 模式，使用固定 IP
  
  bool apStarted = WiFi.softAP(ssid, password);
  if (!apStarted) {
    Serial.println("AP start failed! Retrying...");
    delay(1000);
    WiFi.softAP(ssid, password);
  }
  
  delay(1000);
  IPAddress apIP(192, 168, 50, 1);
  IPAddress apGateway(192, 168, 50, 1);
  IPAddress apSubnet(255, 255, 255, 0);
  if (!WiFi.softAPConfig(apIP, apGateway, apSubnet)) {
    Serial.println("softAPConfig failed");
  }
  
  delay(500);
  Serial.println("");
  Serial.println("==========================");
  Serial.print("AP SSID: ");
  Serial.println(ssid);
  Serial.print("AP Password: ");
  Serial.println(password);
  Serial.print("AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("==========================");

  // Web 服务
  server.on("/", handleRoot);
  server.on("/control", handleControl);
  server.begin();
}

void loop() {
  server.handleClient();
}