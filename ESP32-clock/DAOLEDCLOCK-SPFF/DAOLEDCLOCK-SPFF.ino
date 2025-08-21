/*
 * ESP32 LED Matrix Clock Project
 * Features: WiFi, NTP time sync, weather display, web interface
 * Hardware: ESP32 + HUB75 LED matrix panel + DHT11 sensor
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <WiFi.h>
#include <TimeLib.h>
#include "time.h"
#include <HTTPClient.h>
#include <WiFiUdp.h>
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <SPI.h>
#include "SPIFFS.h"
#include <FS.h>
#include <SimpleDHT.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <Update.h>
#include "MyFont.h"
#include "Arduino_GB2312_library.h"

// ============================================================================
// CONSTANTS AND DEFINES
// ============================================================================
#define LEDC_CHANNEL_0 0
#define LEDC_TIMER_13_BIT 13
#define LED_PIN  2

// Color constants
#define TFT_DARKGREY 0x7BEF

// Access Point mode IP
IPAddress apIP(192, 168, 4, 1);
const char* AP_SSID  = "DACLOCK";    // AP hotspot name

// HTML templates for web interface
#define ROOT_HTML  "<!DOCTYPE html><html><head><title>WIFI Config by lwang</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"></head><style type=\"text/css\">.input{display: block; margin-top: 10px;}.input span{width: 100px; float: left; float: left; height: 36px; line-height: 36px;}.input input{height: 30px;width: 200px;}.btn{width: 120px; height: 35px; background-color: #000000; border:0px; color:#ffffff; margin-top:15px; margin-left:100px;}</style><body><form method=\"POST\" action=\"configwifi\"><label class=\"input\"><span>WiFi SSID</span><input type=\"text\" name=\"ssid\" value=\"\"></label><label class=\"input\"><span>WiFi PASS</span><input type=\"text\"  name=\"pass\"></label><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submie\"> <p><span> Nearby wifi:</P></form>"

#define SUCCESS_HTML  "<html><body><font size=\"10\">successd,wifi connecting...<br />Please close this page manually.</font></body></html>"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// WiFi settings
String wifi_ssid = "111";
String wifi_pass = "electrodragon";
String scanNetworksID = "";    // Store scanned WiFi IDs

// Display and hardware flags
boolean isnight = false;
boolean soundon = true;
boolean caidaion = true;       // Color animation enabled
boolean twopannel = true;      // Two panel mode
boolean isDoubleBuffer = true;
boolean isnightmode = true;
int star1_x, star1_y;
// Hardware configuration
int scroll_x = 0;
int pinDHT11 = 21;            // DHT11 sensor pin (IO21)
SimpleDHT11 dht11(pinDHT11);
MatrixPanel_I2S_DMA *dma_display = nullptr;

// NTP and time settings
WiFiUDP udp;
const char* ntpServerName = "time1.aliyun.com";
IPAddress timeServerIP;
const int timeZone = 8;

// Time variables
int hou, minu, sec, year1, month1, day1, week, sec_ten, sec_one;

// Weather data variables
char wea_temp1[4] = "";
char * wea_temp2;
char tem_day1_min[4]="";
char tem_day1_max[4]="";
char tem_day2_min[4]="";
char tem_day2_max[4]="";
char tem_day3_min[4]="";
char tem_day3_max[4]="";
char day1_date[11]="";
char day2_date[11]="";
char day3_date[11]="";
char temp_day_date[6]="";

// Weather codes
int wea_code;
int wea_code_day1;
int wea_code_night1;
int wea_code_day2;
int wea_code_night2;
int wea_code_day3;
int wea_code_night3;

// Display control variables
int yy1 = 0, yy2 = 0, yy3 = 0; // Different clock face layouts
int i = 0;                      // Date horizontal scroll
bool isFirst = true;            // First time pixel print flag

// Chinese calendar variables
char * china_year;
char *china_month;
char *china_day;
char* wind_fx;
String jieri = "a520a";         // Festival name
char * jieqi;                   // Solar term
int wind_level;
int  wea_hm;                    // Weather humidity
int light = 0;                  // Brightness

// Date strings
String year_, month_, day_;

// Sensor data
byte temperature = 0;
byte humidity = 0;              // DHT11 readings

// Display colors (RGB565 format)
uint16_t color = 0xf0b0;        // Time color
uint16_t color2 = 0x780F;       // Date color
uint16_t color3 = 0xf000;       // Lunar calendar color
uint16_t color4 = 0xfff0;       // Temperature color
uint16_t color5 = 0xFDA0;
uint16_t colorl = 0xff00;       // Scroll bar color

// Web server configuration
WebServer server(80);
const char* host = "esp32";
const char* username = "admin";
const char* userpass = "000000";

// Display arrays
uint16_t ledtab[128][64];
uint16_t ledtab_old[64][64];
int buffer_id = 0;
int gif_i = 0;
int screen_num = 0;             // Display control for elements jumping between two screens
int star_x[20], star_y[20];
uint16_t star_color[20];
boolean isGeneralStar = true;
// boolean isnightmode = true;

// Animation arrays
const char* laohugif[] = {"/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp","/hlh1.bmp","/hlh2.bmp"}; // Tiger animation
File fsUploadFile;

// Panel configuration
int PANEL_CHAIN = 2;            // Total number of panels chained one to another

// Clock registration system
String macAddr = WiFi.macAddress();
String clockname = "";
int temp_mod = -3;              // Temperature adjustment parameter
int hum_mod = 0;                // Humidity adjustment parameter
String zx_key = "";             // API key
String city = "";               // City name
int len_city;                   // City name length
int len_key;                    // API key length
int netpage_wait = 0;           // Network page wait counter
int starnum = 20;               // Number of stars

// Web interface HTML template
const char* serverIndex=
  "<!DOCTYPE HTML><html><head>"
  "<script src='https://cdn.staticfile.org/jquery/1.10.2/jquery.min.js'></script>"
  "<title>DAOLEDClockV1.0</title><meta name='viewport' content='width=device-width, initial-scale=1' charset='UTF-8'>"
  "<script type='text/javascript'>"
  "function submit2() {      $.ajax({ type: 'GET', url: '/get', data: {reboot:1 }, dataType: 'html', success: function(result) {   alert('Rebooted!');}     });   };"
  "function submit3() {      $.ajax({ type: 'GET', url: '/regclock', data: {clockname: $('#clockname').val()}, dataType: 'html', success: function(result) {   alert('Registered!');}     });   };"
  "</script></head>"
  "<body bgcolor=lightyellow >"
  "<table border='1' bgcolor=lightblue><tr><td border='0' colspan='4'  align=center><h1>DaClock Settings</h1></td>"
  "<tr><td><form method='POST' action='configwifi'><label class='input'><span>WiFi SSID</span></td><td><input type='text' name='ssid' value=''></label></td></tr>"
  "<tr><td><label><span>WiFi PASS</span></td><td><input type='text'  name='pass'></label></td></tr><tr><td><input  type='submit' name='submit' value='Submit'></td><td><input type='submit' value='Restart Clock' onclick='submit2()'></input></td></tr></form>"
   "<tr><td>Clock Nickname:</td><td><input id='clockname' type='text' value=''></td></tr>"
  "<td><input type='submit' value='Register Clock' onclick='submit3()'></input></td><td><a href='http://82.157.26.5' target='view_frame'>More Settings</a></td></tr>"
  "<tr><td><form method='POST' action='/upload' enctype='multipart/form-data'>"
  "GIF:</td><td><input type='file' name='update' id='update'><input type='submit' value='Submit'></form></td></tr>"
  "<tr><td>Update Firmware:</td>"
  "<td><form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form></br>"
  "<div id='prg'>progress: 0%</div><td></tr></table>"
  "</body>"
  "<script>"
  "$('#upload_form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "</script>";

// Union type for saving integers in EEPROM
union para_value {
  int val;
  byte val_b[2];
};
para_value e_int;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Display functions (defined in other files)
void initOLED();
void clearOLED();
void drawText(const char* text, int x, int y);
void showTime();
void onlyShowTime();
void onlyShowTime2();
void showTigger();
void fillScreenTab();
void cleanTab();
void drawBit(int x, int y, const uint8_t* bitmap, int w, int h, uint16_t color);

// WiFi and network functions (defined in other files)
void connectToWiFi(int timeout);
void getWeather();
void get3DayWeather();

// System functions (defined in other files)
void nightMode();
void setBrightness(float voltage);
void refreshData(void* parameter);
void refreshTQ(void* parameter);

// File upload functions (defined in other files)
void uploadFinish();
void handleFileUpload();

// WiFi icon bitmap (example - needs to be defined properly)
extern const uint8_t wifi[];

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

// Load configuration from EEPROM
void myconfig() {
  EEPROM.begin(1024);
  
  // Read temperature adjustment
  e_int.val_b[0] = EEPROM.read(0);
  e_int.val_b[1] = EEPROM.read(1);
  temp_mod = e_int.val;
  if (temp_mod >= 0 && temp_mod < 32768)  temp_mod = temp_mod;
  if (temp_mod >= 32768)  temp_mod = temp_mod - 65536;
  
  // Read humidity adjustment
  e_int.val_b[0] = EEPROM.read(2);
  e_int.val_b[1] = EEPROM.read(3);
  hum_mod = e_int.val;
  if (hum_mod >= 0 && hum_mod < 32768)  hum_mod = hum_mod;
  if (hum_mod >= 32768)  hum_mod = hum_mod - 65536;

  // Read city name length and API key length
  len_city = EEPROM.read(4);
  len_key = EEPROM.read(5);
  city = "";
  zx_key = "";
  
  // Read city name
  for (int i = 0; i < len_city; i++) {
    city += char(EEPROM.read(6 + i));
  }
  
  // Read API key
  for (int i = 0; i < len_key; i++) {
    zx_key += char(EEPROM.read(7 + len_city + i));
  }
  
  // Read brightness setting
  e_int.val_b[0] = EEPROM.read(8 + len_city + len_key);
  e_int.val_b[1] = EEPROM.read(9 + len_city + len_key);
  light = e_int.val;
  if (light >= 0 && light < 32768)  light = light;
  if (light >= 32768)  light = light - 65536;
  
  // Read other settings
  soundon = EEPROM.read(10 + len_city + len_key);
  caidaion = EEPROM.read(11 + len_city + len_key);
  isDoubleBuffer = EEPROM.read(12 + len_city + len_key);
  twopannel = EEPROM.read(13 + len_city + len_key);
  
  if(twopannel){
    PANEL_CHAIN = 2;
  } else {
    PANEL_CHAIN = 1;
  }
  
  isnightmode = EEPROM.read(14 + len_city + len_key);
  
  Serial.print("load config success!");
  Serial.println(city);
  Serial.println(zx_key);
}

// Save configuration to EEPROM
void saveconfig() {
  EEPROM.begin(1024);
  
  // Save temperature adjustment
  e_int.val = temp_mod;
  EEPROM.write(0, e_int.val_b[0]);
  EEPROM.write(1, e_int.val_b[1]);
  
  // Save humidity adjustment
  e_int.val = hum_mod;
  EEPROM.write(2, e_int.val_b[0]);
  EEPROM.write(3, e_int.val_b[1]);

  // Save city and API key lengths
  int i = 0;
  len_city = city.length();
  len_key = zx_key.length();
  EEPROM.write(4, len_city);
  EEPROM.write(5, len_key);
  
  // Save city name - fix buffer size
  char citychar[city.length() + 1]; // Add +1 for null terminator
  strcpy(citychar, city.c_str());
  for (; i < strlen(citychar); i++) {
    EEPROM.write(6 + i, citychar[i]);
  }
  Serial.println(city);
  
  // Save API key - fix buffer size
  char keychar[len_key + 1]; // Add +1 for null terminator
  strcpy(keychar, zx_key.c_str());
  for (int j = 0; j < strlen(keychar); j++) {
    EEPROM.write(7 + len_city + j, keychar[j]);
  }
  
  // Save brightness
  e_int.val = light;
  EEPROM.write(8 + len_city + len_key, e_int.val_b[0]);
  EEPROM.write(9 + len_city + len_key, e_int.val_b[1]);
  
  // Save other settings
  EEPROM.write(10 + len_city + len_key, soundon);
  EEPROM.write(11 + len_city + len_key, caidaion);
  EEPROM.write(12 + len_city + len_key, isDoubleBuffer);
  EEPROM.write(13 + len_city + len_key, twopannel);
  EEPROM.write(14 + len_city + len_key, isnightmode);

  EEPROM.commit();
  Serial.print("save config success!");
}

// ============================================================================
// SENSOR FUNCTIONS
// ============================================================================

// Read DHT11 temperature and humidity sensor
void dht11read() {
  int err = SimpleDHTErrSuccess;
  Serial.print(err);
  dht11.read(&temperature, &humidity, NULL);
}

// ============================================================================
// TIME FUNCTIONS
// ============================================================================

// Get current time and format time variables
void GetTime()
{
  year1 = year();
  month1 = month();
  day1 = day();

  hou = hour();
  minu = minute();
  sec = second();
  week = weekday() - 1;           // Sunday data is 1
  if (week == 0) week = 15;       // Sunday displays as 'day'

  sec_ten = second() / 10;
  sec_one = second() % 10;
}

// ============================================================================
// NTP TIME SYNCHRONIZATION
// ============================================================================

/*-------- NTP code ----------*/
const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; // buffer to hold incoming & outgoing packets

// Get time from NTP server
time_t getNtpTime()
{
  IPAddress ntpServerIP; // NTP server's ip address
  while (udp.parsePacket() > 0) ; // discard any previously received packets
  WiFi.hostByName(ntpServerName, ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  return 0; // return 0 if unable to get the time
}

// Send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

// ============================================================================
// LUNAR CALENDAR FUNCTIONS
// ============================================================================

// Get lunar calendar data (Chinese traditional calendar)
void getNongli(String nian, String yue, String ri) {
  HTTPClient http;
  // We now create a URI for the request
//  String url = "http://www.autmone.com/openapi/icalendar/queryDate?date=" + nian + "-" + yue + "-" + ri;
//  Serial.print(url);
//  http.begin(url.c_str());
//  String payload ;
//  // Send HTTP GET request
//  int httpResponseCode = http.GET();
//
//  if (httpResponseCode > 0) {
//    Serial.print("HTTP Response code: ");
//    Serial.println(httpResponseCode);
//    payload = http.getString();
//    Serial.println(payload);
//    //  drawText(payload,0,20);
//
//  }
//  else {
//    Serial.print("Error code: ");
//    Serial.println(httpResponseCode);
//  }
//  // Free resources
//  http.end();


  // Use ArduinoJson library to parse weather JSON data returned by Xinzhi API
  // Can use https://arduinojson.org/v6/assistant/ ArduinoJson assistant to generate relevant JSON parsing code - very convenient!!!
//  StaticJsonDocument<768> doc;
//  DeserializationError error = deserializeJson(doc, payload);
//  if (error) {
//    Serial.print(F("deserializeJson() failed: "));
//    Serial.println(error.f_str());
//    return;
//  }

//  int code = doc["code"]; // 0
//  const char* msg = doc["msg"]; // "SUCCESS"
//  long long time = doc["time"]; // 1631449659945
//  JsonObject data = doc["data"];
//  int data_iYear = data["iYear"]; // 2021
//  int data_iMonth = data["iMonth"]; // 11
//  int data_iDay = data["iDay"]; // 6
//  const char* data_iMonthChinese = data["iMonthChinese"]; // "十一月"
//  const char* data_iDayChinese = data["iDayChinese"]; // "初六"
//  int data_sYear = data["sYear"]; // 2021
//  int data_sMonth = data["sMonth"]; // 12
//  int data_sDay = data["sDay"]; // 9
//  const char* data_cYear = data["cYear"]; // "辛丑年"

  // Static data for demonstration (normally would be fetched from API)
  int data_iYear = 2021; // 2021
  int data_iMonth = 11; // 11
  int data_iDay = 6; // 6
  const char* data_iMonthChinese = "十一月"; // "November"
  const char* data_iDayChinese = "初六"; // "6th day"
  int data_sYear = 2021; // 2021
  int data_sMonth = 12; // 12
  int data_sDay = 9; // 9
  const char* data_cYear = "辛丑年"; // "Year of Metal Ox"
  
  china_year = new char[strlen(data_cYear) + 1];
  strcpy(china_year, data_cYear);
  china_month = new char[strlen(data_iMonthChinese) + 1];
  strcpy(china_month, data_iMonthChinese);
  china_day = new char[strlen(data_iDayChinese) + 1];
  strcpy(china_day, data_iDayChinese);
  Serial.print(china_year);
  Serial.print(china_month);
  Serial.print(china_day);
  
//  const char* data_solarFestival = data["solarFestival"]; // " World Football Day"
  const char* data_solarFestival = "World Football Day"; // " World Football Day"
  jieri = data_solarFestival;
  jieri.replace(" ", "");
  
//  const char* data_solarTerms = data["solarTerms"]; // nullptr solar term
//  const char* data_lunarFestival = data["lunarFestival"]; // nullptr
  const char* data_solarTerms = "nullptr_solar_term"; // nullptr solar term
  const char* data_lunarFestival = "nullptr"; // nullptr
  jieqi = new char[strlen(data_solarTerms) + 1];
  strcpy(jieqi, data_solarTerms);
  
//  const char* data_week = data["week"]; // "Thursday"
  const char* data_week = "Thursday"; // "Thursday"
  
  // Convert month names for special lunar months
  if (strcmp(china_month, "十一月") == 0) {
    china_month = "冬月";  // Winter month
  }
  if (strcmp(china_month, "十二月") == 0) {
    china_month = "腊月";  // Twelfth lunar month
  }
}

// ============================================================================
// WEB SERVER FUNCTIONS
// ============================================================================

// Handle login authentication for web interface
void dologin() {
  if (!server.authenticate(username, userpass)) {
    return server.requestAuthentication();
  }
  server.sendHeader("Connection", "close");
  server.send(200, "text/html", serverIndex);
}

// Initialize and configure web server
void updateServer() {
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  
  /*return index page which is stored in serverIndex */
  server.on("/", dologin);
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  server.on("/upload", HTTP_POST, []() {
    uploadFinish();
  }, handleFileUpload);
  
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.on("/get", HTTP_GET, handleSet);
  server.on("/regclock", HTTP_GET, handlereg);
  server.begin();
}

// Handle clock registration
void handlereg(){
  HTTPClient http;
  // We now create a URI for the request
  if (server.hasArg("clockname")) {
    clockname = server.arg("clockname");
  }
  //http.addHeader("", "-------");
  String url = "http://82.157.26.5/registerClock?clockid="+macAddr+"&clockname="+clockname;
  Serial.print(url);
  http.begin(url.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode==200) {
  server.send(200, "text/plain", "alert('Registration successful!'"+macAddr+")");
  } 
}

// Handle settings and commands (callback function)
void handleSet()
{
  if (server.hasArg("reboot")) {
    ESP.restart();
  }
  server.send(200, "text/plain", "alert('Rebooted!')");
}
 
// ============================================================================
// SENSOR READING FUNCTIONS
// ============================================================================

// Read light sensor value and convert to voltage
float sensor_Read() {
  int sensorValue = analogRead(34); // Read input from analog pin 34
  float voltage = sensorValue * (10 / 1023.0); // Convert analog reading (0-1023) to voltage (0-5V)
  //  Serial.print("Voltage：");
  //  Serial.println(voltage); // Print the analog reading value
  if(voltage < 1){
    voltage = 1;
  }
  return voltage;
}

// ============================================================================
// MAIN SETUP AND LOOP
// ============================================================================

void setup()
{
  Serial.begin(115200);
  myconfig();
  initOLED();
  int i = 0;
  
  // Connect to WiFi
  drawText("Try to connect WIFI!", 64, 0);

  connectToWiFi(15);   //apConfig();

  if (WiFi.status() == WL_CONNECTED) {
    updateServer();
    clearOLED();
    Serial.println(" CONNECTED");
    drawText("CONNECTED!", 64, 0);
    drawText(WiFi.localIP().toString().c_str(),0, 20);
    Serial.print(WiFi.localIP());
    
    // Initialize and get the time
    setSyncProvider(getNtpTime);
    GetTime();
    year_ = String(year1);
    month_ = String(month1);
    day_ = String(day1);
    getNongli( year_, month_, day_);
    getWeather();
    get3DayWeather();
//    getBirth();
//    dht11read();
    showTime();
    //   drawHanziS(0,9,china_year,0xff0a);
  }
  
  SPIFFS.begin();
  while (!SPIFFS.begin(true))
  {
    Serial.print("...");
  }
  Serial.println("SPIFFS OK!");
}

void loop()
{
  server.handleClient();
//  dht11read();
  
  // Refresh brightness
  if (sensor_Read() < 1.2) {
    nightMode();
  } else {
    isnight = false;
  }
  
  // Sync time every hour
  if ( minu == 0 && sec == 0) {
    setSyncProvider(getNtpTime);
    GetTime();
    year_ = String(year1);
    month_ = String(month1);
    day_ = String(day1);
    getNongli( year_, month_, day_);
  }
  
  // Update every 2 minutes (except every 10 minutes)
  if (minu % 2 == 0 && sec == 0 && minu%10!=0) {
//      getBirth();
  } 
  
  // Update every 10 seconds
  if(sec%10==0){
//      getConf();
  }
  
  setBrightness(sensor_Read());
  cleanTab();
  
  if (WiFi.status() == WL_CONNECTED || netpage_wait > 50) {
    xTaskCreate(
      refreshData,
      "refreshData",
      100000,
      NULL,
      1,
      NULL);
      
    if(isnightmode){
      if (hour() > 5 && hour() < 23) {
        // Update weather
        if (minu % 10 == 0 && sec == 0 && minu!=0) {
         xTaskCreate(refreshTQ,"refreshTQ",10000,NULL,1,NULL);
        } 
        GetTime();
        showTime();
        showTigger();
      } else { // Night mode - only show time
        if(twopannel){
          screen_num=0;
        onlyShowTime();
        }else{
         screen_num=0;
         onlyShowTime2();
        }
        isnight=true;
      }
    } else {
      GetTime();
      showTime();
      showTigger();
    }
    fillScreenTab();
  } else {
    drawBit(116, 52, wifi, 12, 12, TFT_DARKGREY);
    connectToWiFi(15); 
  }
  
  if (netpage_wait < 52) {
    netpage_wait++;
  }
  delay(350);
}
