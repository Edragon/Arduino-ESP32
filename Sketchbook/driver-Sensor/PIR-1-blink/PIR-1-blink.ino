// Define the analog input pin and the LED pin
const int analogPin = A0;
const int ledPin = 13;

// Voltage threshold for turning the LED on (3V in this case)
const float thresholdVoltage = 3.0;

void setup() {
  // Initialize the LED pin as an output
  pinMode(ledPin, OUTPUT);
  
  // Start the serial communication (optional, for debugging)
  Serial.begin(9600);
}

void loop() {
  // Read the analog value from pin A0 (0-1023)
  int analogValue = analogRead(analogPin);

  // Convert the analog value to voltage
  float voltage = (analogValue / 1023.0) * 5.0;

  // Print the voltage to the serial monitor (optional, for debugging)
  Serial.print("Voltage: ");
  Serial.println(voltage);

  // Check if the voltage is greater than the threshold
  if (voltage > thresholdVoltage) {
    digitalWrite(ledPin, HIGH); // Turn on the LED
  } else {
    digitalWrite(ledPin, LOW);  // Turn off the LED
  }

  // Add a small delay to stabilize the readings
  delay(100);
}
