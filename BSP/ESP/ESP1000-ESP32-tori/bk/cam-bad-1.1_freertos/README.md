# ESP32-CAM Simple Web Camera Server

A simplified web camera server for ESP32-CAM with OV2640 sensor featuring a single capture button.

## Features

- 📷 Single button image capture via web interface
- ⚡ GPIO3 external trigger input (pull-down, rising edge)
- 🌐 Clean web interface
- 📱 Mobile-friendly responsive design
- ⚡ LED flash support
- 🔧 Auto PSRAM detection

## Hardware Required

- ESP32-CAM module with OV2640 camera sensor (AI-Thinker board)
- USB-to-Serial adapter for programming
- Jumper wires

## Setup Instructions

1. **Board Selection**: Select "AI Thinker ESP32-CAM" in Arduino IDE
2. **Partition Scheme**: Choose "Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)"
3. **Upload Settings**:
   - Upload Speed: 115200
   - Flash Frequency: 80MHz
   - Flash Mode: QIO

## WiFi Configuration

Update these lines in the code with your WiFi credentials:
```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

## How to Use

### Web Interface:
1. Upload the code to your ESP32-CAM
2. Open Serial Monitor at 115200 baud
3. Wait for WiFi connection
4. Note the IP address displayed in Serial Monitor
5. Open the IP address in your web browser
6. Click "📸 Capture Image" button to take photos

### GPIO3 Trigger:
1. Connect a button or sensor to GPIO3 (with pull-down enabled)
2. When GPIO3 goes HIGH (3.3V), it automatically captures an image
3. Image capture is logged to Serial Monitor
4. No web interface needed for GPIO trigger

## Pin Configuration (AI-Thinker ESP32-CAM)

- PWDN: GPIO32
- RESET: -1 (not used)
- XCLK: GPIO0
- SIOD (SDA): GPIO26
- SIOC (SCL): GPIO27
- Y9: GPIO35
- Y8: GPIO34
- Y7: GPIO39
- Y6: GPIO36
- Y5: GPIO21
- Y4: GPIO19
- Y3: GPIO18
- Y2: GPIO5
- VSYNC: GPIO25
- HREF: GPIO23
- PCLK: GPIO22
- LED Flash: GPIO4
- **Trigger Input: GPIO3** (with internal pull-down)

## Trigger Input Wiring

### Simple Button Connection:
```
GPIO3 ----[Button]---- 3.3V
         |
        GND (via internal pull-down)
```

### External Sensor Connection:
```
GPIO3 ----[Sensor Signal]---- (3.3V when triggered)
         |
        GND (via internal pull-down)
```

**Note**: GPIO3 has internal pull-down enabled, so it defaults to LOW (0V) and triggers on HIGH (3.3V).

## Features

### Auto PSRAM Detection
The code automatically detects if PSRAM is available and adjusts settings accordingly:
- **With PSRAM**: Higher quality, dual frame buffers
- **Without PSRAM**: Optimized for internal DRAM

### Web Interface
- Clean, modern design
- Single capture button
- Real-time status updates
- Captured images display instantly
- Mobile-responsive layout

### Camera Settings
- Initial resolution: QVGA (320x240) for fast loading
- JPEG format for web compatibility
- Automatic flash LED control
- VGA/SVGA resolution support

## Troubleshooting

1. **Camera init failed**: Check wiring connections
2. **WiFi not connecting**: Verify SSID and password
3. **No image display**: Check if camera module is properly seated
4. **Poor image quality**: Ensure adequate lighting

## Code Structure

- `setup()`: Initializes camera, WiFi, and web server
- `handleRoot()`: Serves the main web page with capture button
- `handleCapture()`: Captures image and sends to browser
- `loop()`: Handles web server requests

This simplified version removes unnecessary complexity while maintaining all essential camera functionality.
