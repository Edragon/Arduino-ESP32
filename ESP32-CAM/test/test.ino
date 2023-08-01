#define SCL 13
#define SDA 4
#define BAT_LED 2
#define KEY 0
#define BAT_LVL 12
#define PIR 14

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(115200);
}

// the loop routine runs over and over again forever:
void loop() {
  // read the input on analog pin 0:
  int sensorValue = analogRead( BAT_LVL );
  // print out the value you read:
  Serial.println(sensorValue / 1000 - 2);
  delay(50);        // delay in between reads for stability
}
