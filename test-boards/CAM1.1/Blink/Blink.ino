#define onboard 33
#define flash 4

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED1, HIGH);   
  //digitalWrite(LED2, HIGH);   
  delay(1000);                      
  digitalWrite(LED1, LOW);  
  //digitalWrite(LED2, LOW); 
  delay(1000);                  
  
}
