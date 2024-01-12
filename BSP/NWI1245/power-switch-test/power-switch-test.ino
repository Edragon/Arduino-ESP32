#define CAN0_TX 23 
#define CAN0_RX 22

#define TXD1 17
#define RXD1 16 

#define CTRL1 12 
#define CTRL2 19  // chip is not soldered by default 
#define STATUS1 13
#define STATUS2 18

#define P_ADC 33
#define B_LED 5

// the setup function runs once when you press reset or power the board
void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(CTRL1, OUTPUT);
  pinMode(CTRL2, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(CTRL1, HIGH);   // turn the LED on (HIGH is the voltage level)
  digitalWrite(CTRL2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(3000);                       // wait for a second
  
  digitalWrite(CTRL1, LOW);    // turn the LED off by making the voltage LOW
  digitalWrite(CTRL2, LOW);    // turn the LED off by making the voltage LOW
  delay(3000);                       // wait for a second
}
