

#include <ESP32Servo.h>

// create two servo objects 
Servo servo1;
Servo servo2;

// Published values for SG90 servos; adjust if needed
int servo1MinUs = 500;
int servo1MaxUs = 2500;
int servo2MinUs = 500;
int servo2MaxUs = 2500;

// GPIO pins for servos
int servo1Pin = 13;
int servo2Pin = 14;

int pos = 0;      // position in degrees

void setup() {
	// Allow allocation of all timers
	ESP32PWM::allocateTimer(0);
	ESP32PWM::allocateTimer(1);
	Serial.begin(115200);
	while (!Serial) { delay(10); } // Wait for Serial to be ready
	
	Serial.println("\n=== ESP32 Servo Controller Started ===");
	Serial.print("Servo1 Pin: ");
	Serial.print(servo1Pin);
	Serial.print(" | Range: ");
	Serial.print(servo1MinUs);
	Serial.print("-");
	Serial.print(servo1MaxUs);
	Serial.println(" µs");
	Serial.print("Servo2 Pin: ");
	Serial.print(servo2Pin);
	Serial.print(" | Range: ");
	Serial.print(servo2MinUs);
	Serial.print("-");
	Serial.print(servo2MaxUs);
	Serial.println(" µs");
	Serial.println("======================================\n");
	
	servo1.setPeriodHertz(50);      // Standard 50hz servo
	servo2.setPeriodHertz(50);      // Standard 50hz servo
	
	// Attach servos
	servo1.attach(servo1Pin, servo1MinUs, servo1MaxUs);
	servo2.attach(servo2Pin, servo2MinUs, servo2MaxUs);
	
	// Set to center position
	servo1.write(90);
	servo2.write(90);
	
	Serial.println("\nServos ready!");
	Serial.println("Commands:");
	Serial.println("  1 <angle>  - Set servo1 position (0-180)");
	Serial.println("  2 <angle>  - Set servo2 position (0-180)");
	Serial.println("  Example: 1 90\n");
}

void loop() {
	if (Serial.available() > 0) {
		// Read the servo number (1 or 2)
		int servoNum = Serial.parseInt();
		
		// Read the position (0-180)
		int position = Serial.parseInt();
		
		// Clear remaining characters in buffer
		while (Serial.available() > 0) {
			Serial.read();
		}
		
		// Validate and execute command
		if (servoNum == 1 && position >= 0 && position <= 180) {

			servo1.write(position);
			
			Serial.print("Servo1 set to: ");
			Serial.print(position);
			Serial.println("°");
		} else if (servoNum == 2 && position >= 0 && position <= 180) {
			
			servo2.write(position);

			Serial.print("Servo2 set to: ");
			Serial.print(position);
			Serial.println("°");
		} else {
			Serial.println("Invalid command! Use: <servo> <angle>");
			Serial.println("Example: 1 90");
		}
	}
	
	delay(10); // Small delay to prevent overwhelming the serial buffer
}

