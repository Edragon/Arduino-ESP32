

#define switch1 12
#define switch2 19  // not solder 

void setup() {

  pinMode(switch1, OUTPUT);
  pinMode(switch2, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(switch1, HIGH);   
  digitalWrite(switch2, HIGH);   
  delay(3000);   
                     
  digitalWrite(switch1, LOW);   
  digitalWrite(switch2, LOW);   
  delay(3000);                      
}
