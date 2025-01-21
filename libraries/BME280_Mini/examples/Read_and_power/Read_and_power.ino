#include <BME280_Mini.h>

// Pin definitions
const uint8_t SDA_PIN = 2;
const uint8_t SCL_PIN = 3;
const uint8_t LED_PIN = 13;

// Create BME280_Mini instance
BME280_Mini bme(SDA_PIN, SCL_PIN);

// Measurement interval (in milliseconds)
const unsigned long MEASURE_INTERVAL = 60000;  // 1 minute

// Threshold values for alerts
const float TEMP_HIGH_THRESHOLD = 30.0;    // °C
const float PRESSURE_LOW_THRESHOLD = 980.0; // hPa
const uint8_t HUMIDITY_HIGH_THRESHOLD = 70; // %

// Variables to store last measurement time
unsigned long lastMeasureTime = 0;

// Function prototypes
void checkThresholds(const BME280_Mini::Data& data);
void blinkLED(int times);
void printMeasurements(const BME280_Mini::Data& data);

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  
  // Initialize sensor with error checking
  Serial.print("Initializing BME280 sensor... ");
  
  // Try to initialize sensor up to 5 times
  for (int i = 0; i < 5; i++) {
    if (bme.begin()) {
      Serial.println("SUCCESS!");
      blinkLED(3);  // Success indication
      return;
    }
    delay(1000);
  }
  
  // If we get here, initialization failed
  Serial.println("FAILED!");
  while (1) {
    blinkLED(1);  // Error indication
    delay(2000);
  }
}

void loop() {
  unsigned long currentTime = millis();
  
  // Check if it's time to take a measurement
  if (currentTime - lastMeasureTime >= MEASURE_INTERVAL) {
    BME280_Mini::Data data;
    
    // Wake up the sensor
    bme.wake();
    delay(10);  // Give sensor time to wake up
    
    // Read sensor data
    if (bme.read(data)) {
      // Print measurements
      printMeasurements(data);
      
      // Check if any thresholds are exceeded
      checkThresholds(data);
      
      // Put sensor back to sleep to save power
      bme.sleep();
    } else {
      Serial.println("Error reading sensor data!");
      blinkLED(2);  // Error indication
    }
    
    lastMeasureTime = currentTime;
  }
}

void printMeasurements(const BME280_Mini::Data& data) {
  // Print timestamp
  Serial.print("Time since start: ");
  Serial.print(millis() / 1000);
  Serial.println(" seconds");
  
  // Print temperature
  Serial.print("Temperature: ");
  Serial.print(data.temperature, 1);
  Serial.println(" °C");
  
  // Print pressure
  Serial.print("Pressure: ");
  Serial.print(data.pressure, 1);
  Serial.println(" hPa");
  
  // Print humidity
  Serial.print("Humidity: ");
  Serial.print(data.humidity);
  Serial.println(" %");
  
  Serial.println("------------------------");
}

void checkThresholds(const BME280_Mini::Data& data) {
  bool alert = false;
  
  // Check temperature
  if (data.temperature > TEMP_HIGH_THRESHOLD) {
    Serial.println("ALERT: High temperature!");
    alert = true;
  }
  
  // Check pressure
  if (data.pressure < PRESSURE_LOW_THRESHOLD) {
    Serial.println("ALERT: Low pressure!");
    alert = true;
  }
  
  // Check humidity
  if (data.humidity > HUMIDITY_HIGH_THRESHOLD) {
    Serial.println("ALERT: High humidity!");
    alert = true;
  }
  
  // If any threshold was exceeded, blink LED
  if (alert) {
    blinkLED(5);
  }
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
  }
}