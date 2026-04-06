#include <APDS9960.h>

APDS9960 sensor;

// Interrupt pin (adjust based on your board, e.g., 2 for Uno)
const int interruptPin = 2;
volatile bool interruptTriggered = false;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  // Set up interrupt pin
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, FALLING);

  // Initialize the sensor with all features
  if (sensor.beginAll()) {
    Serial.println("APDS9960 initialized with all features!");
  } else {
    Serial.println("Failed to initialize APDS9960. Check connections.");
    while (1);
  }

  // Enable all sensors and interrupts
  sensor.enableAllSensors(true);
  sensor.enableAllInterrupts(true);
  
  // Configure thresholds
  sensor.setLightSensingInterruptThreshold(500, 1000); // Clear channel low/high
  sensor.setProximitySensingInterruptThreshold(50, 150); // Proximity low/high
  
  // Set persistence to reduce false triggers
  sensor.setPersistence(2, 2); // 2 cycles for light and proximity
}

void loop() {
  if (interruptTriggered) {
    Serial.println("Interrupt triggered!");
    
    // Check proximity
    uint8_t proximity = sensor.readProximity();
    Serial.print("Proximity: "); Serial.println(proximity);
    
    // Check color/light
    Color color = sensor.readColorData();
    Serial.print("Clear: "); Serial.println(color.clear);
    
    // Check gesture
    Gesture gesture = sensor.readGesture();
    String direction = sensor.resolveGesture(gesture, 50);
    if (direction != "") {
      Serial.print("Gesture: "); Serial.println(direction);
    }
    
    interruptTriggered = false; // Reset flag
  }
  
  delay(100); // Main loop delay
}

// Interrupt service routine
void handleInterrupt() {
  interruptTriggered = true;
}