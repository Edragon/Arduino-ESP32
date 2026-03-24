#include <Arduino.h>
#include <ESP32ServoController.h>

using namespace MDO::ESP32ServoController;

void printEsp32LedcRegistryUsage() {
	const Esp32LedcRegistry* pRegistry = Esp32LedcRegistry::instance();
	//report the amount of timers used, that should be 1 out of 4 on the tested ESP32 S3
	Serial.println("Esp32LedcRegistry reports:");
	Serial.println(String("  ") + pRegistry->timerUsageToString());
	Serial.println(String("  ") + pRegistry->channelUsageToString());
	Serial.println("");
}

void setup() {
	Serial.begin(460800);
	delay(500);
	Serial.println();
	Serial.println("Starting");
	
	//configure our main settings in the ESP32 LEDC registry
	//Esp32LedcRegistry::instance()->begin(LEDC_CONFIG_ESP32_S3);	//change this for the relevant controller
	Esp32LedcRegistry::instance()->begin(LEDC_CONFIG_ESP32);	//change this for the relevant controller
	

	BestAvailableFactory oFactory;								//used to select the best available timer & channel based on the hardware setup
	PwmFactoryDecorator oFactoryDecorator(oFactory);			//and please 'auto-re-use' timers as much as possible
	//the above two are only needed during the PWMController::begin call
	
	PWMController oPwm1;										//the actual PWM controller, number 1
	PWMController oPwm2;										//the actual PWM controller, number 2
	
	const uint32_t uiFreqHz	= 5000;								//5kHz
	const uint8_t uiPinNr1	= 2;			//ESP32 pin
	const uint8_t uiPinNr2	= 12;			//ESP32 pin
	//const uint8_t uiPinNr1	= 16;		//ESP32_S3 pin
	//const uint8_t uiPinNr2	= 46;		//ESP32_S3 pin
	const double dDuty		= 0.333;							//a duty cycle of 33.3%
	
	printEsp32LedcRegistryUsage();
	
	if (!oPwm1.begin(oFactoryDecorator, uiPinNr1, uiFreqHz, dDuty)) {
		Serial.println("Failed to start the PWM controller (1)");
		return;
	}
	
	if (!oPwm2.begin(oFactoryDecorator, uiPinNr2, uiFreqHz, dDuty*2)) {	//let's start this PWM with another duty for demo purposes
		Serial.println("Failed to start the PWM controller (2)");
		return;
	}

	Serial.println("PWM controllers started");
	printEsp32LedcRegistryUsage();	//should now report in use: 1 timer and 2 channels 
	delay(5000);
	
	Serial.println("start endless fading");
	bool bIncrease = true;
	while (true) {
		for (int i=0; i<1000; i++) {
			double dDuty = (i==0) ? 0:(i/1000.0);
			oPwm1.fade((bIncrease == true ) ? (dDuty) : (1.0-dDuty), 10);	//non-blocking call, but let the fade take 10 msec
			oPwm2.fade((bIncrease == false) ? (dDuty) : (1.0-dDuty), 10);	//same as the above, but inverted in duty percentage
			delay(11);														//take 1 msec extra, to ensure the hardware is done with the fade
		}
		bIncrease = !bIncrease;
	}

}	//note that since oPwm's are local variables, the PWM would stop here without the above endless loop

void loop() {
}