void setup() {
  Serial.begin(115200);
  StartPWM();

}

void StartPWM() {
  ledcSetup(0, 500000, 3);
  ledcAttachPin(16, 0);
  ledcWrite(0, 4);

  // Enable fast discharge.
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
}

int ReadSoilMoisture() {
  delay(10);
  const int n = 1;
  int sum = 0;
  for (int i = 0; i < n; i++) {
    sum += analogRead(36);
  }
  int avg_raw = sum / n;
  return avg_raw;
}


void loop() {
  int dat = ReadSoilMoisture();
  Serial.println(dat);
}
