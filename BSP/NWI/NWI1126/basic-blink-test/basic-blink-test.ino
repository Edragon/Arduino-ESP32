

#define OB_LED 10 // on module led
#define WS_LED 9 // WS2812

#define addc 0

#define W_LED 4 // white
#define B_LED 5 // blue
#define G_LED 6 // green
#define R_LED 7 //red



void setup(void) {

  pinMode(OM_LED, OUTPUT);  // on module led
  pinMode(OB_LED, OUTPUT); // on board led

  pinMode(B_LED, OUTPUT); // channel B Blue
  pinMode(G_LED, OUTPUT); // channel G Green
  pinMode(W_LED, OUTPUT); // channel W white
  pinMode(R_LED, OUTPUT); // channel R Red

  digitalWrite(OM_LED, HIGH);

  Serial.begin(115200);

}

void loop(void) {
  test_LED ();
  test_LED ();
  test_LED ();
  test_LED ();
}






void test_LED () {

  Serial.println("test ..");

  digitalWrite(OB_LED, LOW);
  digitalWrite (B_LED, LOW);
  digitalWrite (G_LED, LOW);
  digitalWrite (W_LED, LOW);
  digitalWrite (R_LED, LOW);
  delay(1000);

  digitalWrite(OB_LED, HIGH);
  digitalWrite (B_LED, HIGH);
  digitalWrite (G_LED, HIGH);
  digitalWrite (W_LED, HIGH);
  digitalWrite (R_LED, HIGH);
  delay(5000);

}
