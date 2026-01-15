#include <WiFi.h>
#include <WebServer.h>

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

// ===== Web Server =====
WebServer server(80);

// ===== HTML Web 控制界面 =====
const char html_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html><html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Motor Control (DRV8871)</title>
<style>
body {
  font-family: Arial, sans-serif;
  max-width: 600px;
  margin: 0 auto;
  padding: 20px;
  background-color: #f0f0f0;
}
  
h2 {
  text-align: center;
  color: #333;
}
.control-panel {
  background: white;
  padding: 20px;
  border-radius: 10px;
  box-shadow: 0 2px 10px rgba(0,0,0,0.1);
}
.section {
  margin-bottom: 25px;
}
.section-title {
  font-weight: bold;
  margin-bottom: 10px;
  font-size: 18px;
  color: #555;
}
.speed-buttons {
  display: grid;
  grid-template-columns: repeat(6, 1fr);
  gap: 8px;
  margin-bottom: 10px;
}
.speed-btn {
  padding: 15px 10px;
  font-size: 16px;
  font-weight: bold;
  border: 2px solid #4CAF50;
  background-color: white;
  color: #4CAF50;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
}
.speed-btn:active {
  background-color: #4CAF50;
  color: white;
  transform: scale(0.95);
}
.dir-buttons {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 10px;
}
.dir-btn {
  padding: 20px;
  font-size: 18px;
  font-weight: bold;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
}
.forward-btn {
  background-color: #2196F3;
  color: white;
}
.reverse-btn {
  background-color: #FF9800;
  color: white;
}
.dir-btn:active {
  transform: scale(0.95);
  opacity: 0.8;
}
.stop-btn {
  width: 100%;
  padding: 20px;
  font-size: 20px;
  font-weight: bold;
  background-color: #f44336;
  color: white;
  border: none;
  border-radius: 8px;
  cursor: pointer;
  margin-top: 20px;
}
.stop-btn:active {
  transform: scale(0.98);
  opacity: 0.8;
}
</style>
</head>
<body>
<h2>Motor Control</h2>
<div class="control-panel">
  <div class="section">
    <div class="section-title">Speed Control (0-10)</div>
    <div class="speed-buttons">
      <button class="speed-btn" onclick="setSpeed(0)">0</button>
      <button class="speed-btn" onclick="setSpeed(1)">1</button>
      <button class="speed-btn" onclick="setSpeed(2)">2</button>
      <button class="speed-btn" onclick="setSpeed(3)">3</button>
      <button class="speed-btn" onclick="setSpeed(4)">4</button>
      <button class="speed-btn" onclick="setSpeed(5)">5</button>
      <button class="speed-btn" onclick="setSpeed(6)">6</button>
      <button class="speed-btn" onclick="setSpeed(7)">7</button>
      <button class="speed-btn" onclick="setSpeed(8)">8</button>
      <button class="speed-btn" onclick="setSpeed(9)">9</button>
      <button class="speed-btn" onclick="setSpeed(10)">10</button>
    </div>
  </div>
  <div class="section">
    <div class="section-title">Direction Control</div>
    <div class="dir-buttons">
      <button class="dir-btn forward-btn" onclick="setDirection('fw')">Forward</button>
      <button class="dir-btn reverse-btn" onclick="setDirection('rv')">Reverse</button>
    </div>
  </div>
  <button class="stop-btn" onclick="stopMotor()">STOP</button>
</div>
<script>
let currentSpeed = 0;
let currentDir = 'fw';
function setSpeed(speed) {
  currentSpeed = speed;
  sendControl();
}
function setDirection(dir) {
  currentDir = dir;
  sendControl();
}
function sendControl() {
  let pwmValue = Math.round((currentSpeed / 10) * 1023);
  fetch('/control?speed=' + pwmValue + '&dir=' + currentDir)
    .then(response => response.text())
    .catch(error => console.error('Error:', error));
}
function stopMotor() {
  currentSpeed = 0;
  fetch('/control?speed=0&dir=' + currentDir)
    .then(response => response.text())
    .catch(error => console.error('Error:', error));
}
</script>
</body></html>
)rawliteral";

// ===== 处理 Web 请求 =====
void handleRoot() {
  server.send(200, "text/html", html_page);
}

void handleControl() {
  // Check for direction control
  if (server.hasArg("dir")) {
    String d = server.arg("dir");
    if (d == "fw") {
      // Forward: IN1=HIGH, IN2=LOW
      digitalWrite(IN1_PIN, HIGH);
      digitalWrite(IN2_PIN, LOW);
      Serial.println("Motor: Forward");
    } else if (d == "rv") {
      // Reverse: IN1=LOW, IN2=HIGH
      digitalWrite(IN1_PIN, LOW);
      digitalWrite(IN2_PIN, HIGH);
      Serial.println("Motor: Reverse");
    }
  }

  // Check for stop command
  if (server.hasArg("stop") || (server.hasArg("speed") && server.arg("speed").toInt() == 0)) {
    // Stop: both LOW
    digitalWrite(IN1_PIN, LOW);
    digitalWrite(IN2_PIN, LOW);
    Serial.println("Motor: Stop");
  }
  
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

// ===== 初始化 =====
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("ESP32 DRV8871 AP Motor Controller starting...");
  
  // Configure GPIO pins as outputs
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // 默认停止 (both LOW)
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);

  // 启动为 WiFi AP 模式，使用固定 IP
  Serial.println("Starting WiFi AP...");
  
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  delay(100);
  
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