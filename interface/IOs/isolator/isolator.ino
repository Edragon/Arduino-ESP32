#define ch1 23
#define ch2 22
#define ch3 21
#define ch4 19

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(ch1, OUTPUT);
  pinMode(ch2, OUTPUT);
  pinMode(ch3, OUTPUT);
  pinMode(ch4, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(ch1, HIGH);
  digitalWrite(ch2, HIGH);
  digitalWrite(ch3, HIGH);
  digitalWrite(ch4, HIGH);
  delay(1000);                       // wait for a second
  digitalWrite(ch1, LOW);   
  digitalWrite(ch2, LOW);  
  digitalWrite(ch3, LOW);  
  digitalWrite(ch4, LOW);   
  delay(1000);                       // wait for a second


  
}
