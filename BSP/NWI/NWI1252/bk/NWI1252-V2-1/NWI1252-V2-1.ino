
// read only 

 /*   Multiple Serial test for XIAO ESP32C3 */
HardwareSerial RS485(0);   //Create a new HardwareSerial class.
HardwareSerial RS232(1);   //Create a new HardwareSerial class.

void setup() {
  // initialize both serial ports:
  Serial.begin(9600);
  // Serial.setDebugOutput(true);
  RS485.begin(9600, SERIAL_8N1, 6, 7); // at CPU Freq is 40MHz, work half speed of defined.
  RS232.begin(9600, SERIAL_8N1, 4, 5); // at CPU Freq is 40MHz, work half speed of defined.
}

void loop() {

  if (RS485.available()) {
    String str  = RS485.readStringUntil('\r');
    
    Serial.println(str);
  }

  if (RS232.available()) {
    String str  = RS232.readStringUntil('\r');
    
    Serial.println(str);
  }

  if (Serial.available()) {
    String str = Serial.readStringUntil('\r');

    RS232.println(str);
    RS485.println(str);
  }

}
