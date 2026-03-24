#include <Arduino.h>
#include <ESP32ServoController.h>

using namespace MDO::ESP32ServoController;

void setup() {
	Serial.begin(460800);
	delay(500);
	Serial.println();
	Serial.println("Starting");
	
	//configure our main settings in the ESP32 LEDC registry
	Esp32LedcRegistry::instance()->begin(LEDC_CONFIG_ESP32_S3);		//change this for the relevant controller
	
																	//!!in my case!! the servo needs specific timing parameters. Please check the relevant data sheet
	//Esp32LedcRegistry::instance()->setServoParams(510, 2490);		//set the lower bound and upper bound respectivly to 500 usec and 2500 usec, with some 'safeguards'	
	
	BestAvailableFactory oTimerChannelFactory;						//used to select the best available timer & channel based on the hardware setup
	ServoFactoryDecorator oFactoryDecorator(oTimerChannelFactory);	//let this ServoFactoryDecorator define the servo frequency to use and such
	//the above two are needed (variable scope related, in 'begin' only)
	
	ServoController oServo;

	const uint8_t uiPinNr	= 16;
	if (!oServo.begin(oFactoryDecorator, uiPinNr)) {				//3rd parameter is the default angle to start from: 90 degrees in this case
		Serial.println("  failed to init the servo..");
		return;
	}
	
	delay(5000);
	Serial.println("Moving servo for demo:");
	while (true) {
		Serial.println("  Moving servo to 0 degrees");
		oServo.moveTo(  0.0,  5000, true);							//move to 0 degrees in 5 seconds, and make this a blocking call
		Serial.println("  Done");
		delay(2000);

		Serial.println("  Moving servo to 180 degrees");
		oServo.moveTo(180.0, 10000, true);							//move to 180 degrees in 10 seconds, and make this a blocking call
		Serial.println("  Done");
		delay(2000);
	}
}

void loop() {
}