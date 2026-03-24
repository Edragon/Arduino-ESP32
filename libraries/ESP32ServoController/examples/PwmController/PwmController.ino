#include <Arduino.h>
#include <ESP32ServoController.h>

using namespace MDO::ESP32ServoController;

void setup() {
	Serial.begin(460800);
	delay(500);
	Serial.println();
	Serial.println("Starting");
	
	//configure our main settings in the ESP32 LEDC registry
	Esp32LedcRegistry::instance()->begin(LEDC_CONFIG_ESP32_S3);	//change this for the relevant controller
	

	BestAvailableFactory oFactory;								//used to select the best available timer & channel based on the hardware setup
	PWMController oPwm;											//the actual PWM controller
	
	const uint32_t uiFreqHz	= 20000;
	const uint8_t uiPinNr	= 16;
	const double dDuty		= 0.333;							//a duty cycle of 33.3%
	
	if (oPwm.begin(oFactory, uiPinNr, uiFreqHz, dDuty)) {
		Serial.println("PWM controller started");
		delay(5000);
		
		Serial.println("start endless fading");
		bool bIncrease = true;
		while (true) {
			for (int i=0; i<1000; i++) {
				double dDuty = (i==0) ? 0:(i/1000.0);
				oPwm.fade(bIncrease ? (dDuty) : (1.0-dDuty), 10, true);	//now a 10msec blocking call, so no delay required		
			}
			bIncrease = !bIncrease;
		}
		
	} else {
		Serial.println("Failed to start the PWM controller");		
	}

}	//note that since oPwm is a local variable, the PWM would stop here without the above endless loop

void loop() {
}