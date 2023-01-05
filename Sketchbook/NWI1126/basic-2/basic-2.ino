
#define onboardLED 10
#define addc 0
#define W 4
#define B 5 
#define G 6
#define R 7
#define WS2 9

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(onboardLED, OUTPUT);
  pinMode(W, OUTPUT);
  pinMode(B, OUTPUT);
  pinMode(G, OUTPUT);
  pinMode(R, OUTPUT);
  
  Serial.begin(9600);
}

// the loop function runs over and over again forever
void loop() {
  
  digitalWrite(onboardLED, HIGH);  
  digitalWrite(W, HIGH);  
  digitalWrite(B, HIGH);  
  digitalWrite(G, HIGH);  
  digitalWrite(R, HIGH);  
  delay(1000);

  int sensorValue = analogRead(addc);
  float voltage = sensorValue * (5.0 / 1023.0);
  Serial.println(voltage);
  
  digitalWrite(onboardLED, LOW);  
  digitalWrite(W, LOW);  
  digitalWrite(B, LOW);  
  digitalWrite(G, LOW);  
  digitalWrite(R, LOW);  
  delay(1000); 
                       
}
