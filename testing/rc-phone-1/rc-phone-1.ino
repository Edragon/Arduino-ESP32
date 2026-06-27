
#include <WiFi.h>
#define CONFIG_ASYNC_TCP_RUNNING_CORE 1 
#include <AsyncTCP.h>          // Requires installing the AsyncTCP library
#include <ESPAsyncWebServer.h> // Requires installing the ESPAsyncWebServer library

// --- Wi-Fi Credentials ---
const char* ssid = "ESP32_C3_RC_Car";
const char* password = "123456789"; // Minimum 8 characters

// --- Pin Allocations ---
const int M1_PWM = 2; // Left side
const int M2_PWM = 3; // Right side
const int LED_PIN = 8; // Indicator LED

// --- PWM Settings ---
const int PWM_FREQ = 5000;
const int PWM_RES  = 8;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// --- HTML + JavaScript Interface ---
// This file is stored directly in the program memory and served to your phone
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>ESP32-C3 RC Control</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #222; color: #fff; margin: 0; padding: 0; user-select: none; overflow: hidden;}
        h1 { margin-top: 20px; color: #00f3ff; font-size: 24px; }
        #container { position: relative; width: 300px; height: 300px; background: #333; border-radius: 50%; margin: 50px auto; border: 4px solid #555; touch-action: none; }
        #joystick { position: absolute; width: 80px; height: 80px; background: #00f3ff; border-radius: 50%; top: 110px; left: 110px; box-shadow: 0 0 20px #00f3ff; }
        #status { font-size: 18px; color: #aaa; margin-top: 20px; }
    </style>
</head>
<body>
    <h1>ESP32-C3 Driver</h1>
    <div id="container">
        <div id="joystick"></div>
    </div>
    <div id="status">Status: Disconnected</div>

    <script>
        var gateway = `ws://${window.location.hostname}/ws`;
        var websocket;
        var container = document.getElementById('container');
        var joystick = document.getElementById('joystick');
        var statusText = document.getElementById('status');
        
        var centerX = 150 - 40;
        var centerY = 150 - 40;
        var dragging = false;

        window.addEventListener('load', initWebSocket);

        function initWebSocket() {
            websocket = new WebSocket(gateway);
            websocket.onopen = function() { statusText.innerText = "Status: Connected"; statusText.style.color = "#00ff00"; };
            websocket.onclose = function() { statusText.innerText = "Status: Disconnected"; statusText.style.color = "#ff0000"; setTimeout(initWebSocket, 2000); };
        }

        container.addEventListener('touchstart', function(e) { dragging = true; handleMove(e.touches[0]); });
        window.addEventListener('touchmove', function(e) { if (dragging) handleMove(e.touches[0]); }, { passive: false });
        window.addEventListener('touchend', function() { 
            dragging = false; 
            joystick.style.left = centerX + 'px'; 
            joystick.style.top = centerY + 'px'; 
            sendData(0, 0); 
        });

        function handleMove(touch) {
            var rect = container.getBoundingClientRect();
            var x = touch.clientX - rect.left - 40;
            var y = touch.clientY - rect.top - 40;
            
            // Limit to boundary ring
            var dx = x - centerX;
            var dy = y - centerY;
            var dist = Math.sqrt(dx*dx + dy*dy);
            if (dist > 110) {
                x = centerX + (dx / dist) * 110;
                y = centerY + (dy / dist) * 110;
            }
            
            joystick.style.left = x + 'px';
            joystick.style.top = y + 'px';

            // Map values to standard motor boundaries (-255 to 255)
            var throttle = Math.round(((centerY - y) / 110) * 255);
            var steering = Math.round(((x - centerX) / 110) * 255);
            sendData(throttle, steering);
        }

        // Limit transmission frequency to protect the socket buffer
        var lastSend = 0;
        function sendData(t, s) {
            var now = Date.now();
            if (now - lastSend > 40 || (t === 0 && s === 0)) { 
                if (websocket.readyState === WebSocket.OPEN) {
                    websocket.send(t + "," + s);
                    lastSend = now;
                }
            }
        }
    </script>
</body>
</html>
)rawliteral";

// --- Motor Control Routing Engine ---
void driveMotors(int16_t throttle, int16_t steering) {
    int16_t leftSpeed  = throttle + steering;
    int16_t rightSpeed = throttle - steering;

    leftSpeed  = constrain(leftSpeed, -255, 255);
    rightSpeed = constrain(rightSpeed, -255, 255);

    // Motor 1 (Left side, unidirectional PWM)
    analogWrite(M1_PWM, abs(leftSpeed));

    // Motor 2 (Right side, unidirectional PWM)
    analogWrite(M2_PWM, abs(rightSpeed));

    // LED indication for activity (lights up when moving)
    if (abs(throttle) > 10 || abs(steering) > 10) {
        digitalWrite(LED_PIN, HIGH);
    } else {
        digitalWrite(LED_PIN, LOW);
    }
}

// --- WebSocket Event Handler ---
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_DATA) {
        data[len] = '\0';
        char* msg = (char*)data;
        
        // Parse incoming csv formatting: "throttle,steering"
        char* token = strtok(msg, ",");
        if (token != NULL) {
            int16_t throttle = atoi(token);
            token = strtok(NULL, ",");
            if (token != NULL) {
                int16_t steering = atoi(token);
                driveMotors(throttle, steering);
            }
        }
    } else if (type == WS_EVT_DISCONNECT) {
        // Safety feature: stop vehicle instantly if phone disconnects or goes out of range
        driveMotors(0, 0);
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize Motor Control and LED Hardware
    pinMode(M1_PWM, OUTPUT);
    pinMode(M2_PWM, OUTPUT);
    pinMode(LED_PIN, OUTPUT);

    analogWriteFrequency(M1_PWM, PWM_FREQ);
    analogWriteResolution(M1_PWM, PWM_RES);
    analogWriteFrequency(M2_PWM, PWM_FREQ);
    analogWriteResolution(M2_PWM, PWM_RES);

    // Turn off driving pins initially
    driveMotors(0, 0);

    // Initialize Soft Access Point Mode
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // Bind WebSocket handling engine
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // Serve HTML page when phone connects to base address
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/html", index_html);
    });

    server.begin();
    Serial.println("Web server started successfully.");
}

void loop() {
    ws.cleanupClients(); // Clears memory buffers automatically
}