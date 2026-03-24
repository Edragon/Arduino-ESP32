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
	Serial.println("Test starting");
	
	//configure our main settings in the ESP32 LEDC registry
	Esp32LedcRegistry::instance()->begin(LEDC_CONFIG_ESP32_S3);	//change this for the relevant controller
	
	{	//ESP32-S3 does not support high speed timers
		Serial.println("High speed timer check: ");
		LedcTimerHighSpeed oTimer(3);
		if (oTimer.begin(20000, 11)) {
			return;	//it should have failed
		}
		Serial.println("  ESP32-S3 does not support high speed timer. Tested OK");
		delay(1000);
		
		if (Esp32LedcRegistry::instance()->isTimerInUse(&oTimer)) {
			return;	//since the above failed, if should also not have been registered
		}
		Serial.println("  Esp32LedcRegistry does not contain the [failed to initialize]-timer. Tested OK");
		delay(1000);
	}
	
	{	//timer clock frequency check
		Serial.println("Timer clock frequency check: ");
		LedcTimerLowSpeed oTimer(0);
		if (oTimer.getSourceClockFrequency() != 80000000) {
			return;	//it should have been 80M, since no clock source selected in the above constructor
		}
		Serial.println("  ESP32-S3 low speed default timer should be 80MHz. Tested OK");
		delay(1000);
	}	//this destroys the not-running oTimer, which is also a test
	
	{
		Serial.println("Timer start check: ");
		const uint8_t uiTimerNr = 0;
		LedcTimerLowSpeed oTimer(uiTimerNr);
		if (Esp32LedcRegistry::instance()->isTimerInUse(&oTimer)) {
			return;	//since the timer isn't initialized yet, if should not have been registered
		}
		Serial.println("  Esp32LedcRegistry does not contain the [not initialized]-timer. Tested OK");
		
		const uint32_t uiFreqHz = 20000;
		uint8_t uiFoundLargestDutyResolutionBits = oTimer.findMaxResolution(oTimer.getSourceClockFrequency(), uiFreqHz);
		if (uiFoundLargestDutyResolutionBits == 0) {
			return;		
		}
		Serial.println(String("  For the selected clock, proposing a duty resolution of: ") + uiFoundLargestDutyResolutionBits + " bit");	

		if (!oTimer.begin(uiFreqHz, uiFoundLargestDutyResolutionBits)) {	//set it to max resolution supported by the chip
			return;
		}
		Serial.println("  Timer started. Good");
		delay(1000);
	}	//this destroys the running oTimer, which is also a test
	
	{
		Serial.println("Channel start checks: ");
		const uint8_t uiTimerNr = 0;
		const uint32_t uiFreqHz = 20000;
		LedcTimerLowSpeed oTimer(uiTimerNr);
		if (!oTimer.begin(uiFreqHz, oTimer.findMaxResolution(oTimer.getSourceClockFrequency(), uiFreqHz))) {	//set it to max resolution supported by the chip
			return;
		}
		
		const uint8_t uiChannelNr = 0;
		const uint8_t uiPinNr = 16;
		
		//set a Duty cycle of ~10%
		const uint32_t uiDuty = 205;	//this is the 'on'-amount of timer-ticks. note, must fit in the above set timer resolution!
		const int iHighPoint = 2000;	//this is the 'total'-amount of timer-ticks. Must also fit in the above set timer resolution
		
		LedcChannelLowSpeed oChannel(uiChannelNr);
		bool bOk = oChannel.begin(uiPinNr, &oTimer, uiDuty, iHighPoint);
		if (!bOk) {
			Serial.println("  Channel failed to configure. Wrong LEDC config selected?");
			return;
		}
		Serial.println("  PWM started");
		delay(5000);
		const uint8_t uiPinNr2 = 44;	//handy for the T-Display-S3
		//const uint8_t uiPinNr2 = 46;	//handy for the T-Display S3 AMOLED
		if (!oChannel.addPin(uiPinNr2)) {
			Serial.println("Failed to add the second pin");
			return;
		}
		Serial.println("  2nd pin added");
		delay(5000);		
	}
	Serial.println("  PWM auto-stopped");
	delay(5000);
	
	{
		Serial.println("Factory checks: ");
		const uint8_t uiPinNr = 16;
		const uint32_t uiFreqHz = 1000;
		const double dDuty = 0.666;
		Serial.println("  setting to 1kHz, 66.6% duty");
		
		LowSpeedFactory oFactory;
		PWMController oPwm;
		if (!oPwm.begin(oFactory, uiPinNr, uiFreqHz, dDuty)) {
			Serial.println("  Failed to create a new PWMController instance");
			return;		
		}
		delay(5000);
	}
	Serial.println("  PWM auto-stopped");
	delay(1000);
	
	{
		Serial.println("Fade checks: ");
		//Serial.println("  **check scope now, if PWM is not being generated**");
		//delay(5000);	//so ensure we can see that the start below indeed starts without a running PWM
		const uint8_t uiPinNr = 16;
		const uint32_t uiFreqHz = 1000;
		const double dDuty = 0.0;
		Serial.println(" setting to 1kHz, but start with 0% duty");
		//so this WILL generate a PWM signal for a 'very short moment' before actually doing the 0% as requested..
		//the best way to have no PWM is to not have a PWMController alive..
		LowSpeedFactory oFactory;
		{
			PWMController oPwm;		
			if (!oPwm.begin(oFactory, uiPinNr, uiFreqHz, dDuty)) {
				Serial.println("  Failed to create a new PWMController instance");
				return;		
			}
			delay(1000);
			
			Serial.println(" fading from 0-100% duty");
			for (int i=0; i<1000; i++) {
				double dDuty = (i==0) ? 0:(i/1000.0);
				oPwm.fade(dDuty, 20, true);	//20 msec per 0.1%, blocking call, so no delay-call needed
			}
			Serial.println(" keeping at 100% duty for a few seconds");
			oPwm.fade(1.0);
			delay(5000);
			
			Serial.println(" and back to 0% again (twice as fast)");
			for (int i=0; i<1000; i++) {
				double dDuty = (i==0) ? 0:(i/1000.0);
				oPwm.fade(1.0-dDuty);			//now a non-blocking call
				delay(10);						//so we do need a delay
			}

			Serial.println(" now at min duty");
			oPwm.fade(0.0);
			delay(5000);
			
			Serial.println(" slow hardware managed fade to 50%");
			oPwm.fade(0.5, 10000, true);	//blocking call
			Serial.println(" slow hardware managed fade to 50% - done");
			delay(5000);
		}
		Serial.println("  PWM stopped");
		delay(1000);
	}
	
	{
		Serial.println("Checking two channels with just one timer: ");
		const uint8_t 	uiPinNr1 = 16;
		const uint8_t	uiPinNr2 = 44;	//handy for the T-Display-S3
		//const uint8_t	uiPinNr2 = 46;	//handy for the T-Display S3 AMOLED
		const uint32_t	uiFreqHz = 1500;
		const double	dDuty1 = 0.06;
		const double	dDuty2 = 0.24;
		

		PWMController* pPwm1 = new PWMController();	//to test/demonstrate that the deletion sequence and the creation sequence do not have to be in sync
		PWMController* pPwm2 = new PWMController();	//based on the usage of the shared pointer for the timer
		
		LowSpeedFactory oFactory;

		if ((!pPwm1->begin(oFactory, uiPinNr1, uiFreqHz, dDuty1)) ||
			(!pPwm2->begin(oFactory, uiPinNr2, pPwm1, dDuty2))) {
			Serial.println("  Failed to create two PWM Controllers with the same timer");
			return;
		}
		
		Serial.println("  creates two channels with different duty cycle (but same frequency) -> so share one timer");
		delay(20000);
		Serial.println("  stopping original channel");
		delete pPwm1;
		
		//now PWM1 is no longer alive / running, PWM2 however is
		delay(5000);
		
		Serial.println("  stopping second channel");
		delete pPwm2;
		delay(5000);
	}
	
	{
		Serial.println("Checking servo functionality: ");
		const uint8_t 	uiPinNr1 = 16;
		const uint8_t	uiPinNr2 = 44;	//handy for the T-Display-S3
		//const uint8_t	uiPinNr2 = 46;	//handy for the T-Display S3 AMOLED
		LowSpeedFactory oTimerChannelFactory;							//lets create a new low speed timer & channel
		ServoFactoryDecorator oFactoryDecorator(oTimerChannelFactory);	//let this ServoFactoryDecorator define the frequency to use and such
		ServoController oServo1;
		if (!oServo1.begin(oFactoryDecorator, uiPinNr1)) {
			Serial.println("  failed to init servo 1");
			return;
		}
		Serial.println("  servo1 initialized at 90 degrees (1.5 msec)");	//so by default 1.5 msec
		
		ServoController oServo2;
		if (!oServo2.begin(oFactoryDecorator, uiPinNr2, 0.0)) {
			Serial.println("  failed to init servo 2");
			return;
		}
		Serial.println("  servo2 initialized at 0 degrees");	//so by default 1 msec
		
		delay(10000);
		
		Serial.println("  Moving servo1 to 0 degrees (1 msec)");
		oServo1.moveTo(  0.0,  2000, true);	//move to 0 degrees in 2 seconds, and make this a blocking call
		Serial.println("  Moving servo1 to 180 degrees (2 msec)");
		oServo1.moveTo(180.0, 10000, true);
		delay(10000);
	}
	
	{
		Serial.println("Checking BestAvailableFactory functionality: ");
		const uint32_t uiFreqHz = 15000;
		const double dDuty1 = 0.1;
		
		BestAvailableFactory oFactory;
		PWMController oPwms[8];											//so 8 is the EPS32 S3 value, this might not be a good plan on other controllers..
		const uint8_t uiPins[8] = {16,15,14,13,12,11,10,43};	//handy for the T-Display-S3
		//const uint8_t uiPins[8] = {16,15,14,13,12,11,10,46};	//handy for the T-Display S3 AMOLED
		if (!oPwms[0].begin(oFactory, uiPins[0], uiFreqHz, dDuty1)) {
			Serial.println("  Failed to create initial PWM Controller");
			return;
		}
		for (int i=1; i<(sizeof(oPwms) / sizeof(PWMController)); i++) {
			if (!oPwms[i].begin(oFactory, uiPins[i], &oPwms[0], dDuty1 + i*dDuty1)) {
				Serial.println(String("  Failed to create PWM Controller ") + i);
				return;				
			}
		}
		
		Serial.println("  Testing channel-depletion detection:");
		PWMController oTooMuch;
		const uint8_t uiFakePin = 44;
		if (oTooMuch.begin(oFactory, uiFakePin, &oPwms[0], 0.66)) {
			Serial.println("    Failed. We should have depleted the maximum amount (8) of channels, which should be detected");
			return;			
		}
		Serial.println("  Depleted channel detected. Good.");
		
		delay(20000);
	}
	
	Serial.println("");
	printEsp32LedcRegistryUsage();	//nothing should be 'pending' at this point

	Serial.println("Tests execution: OK");
}

void loop() {
}