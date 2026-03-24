#include "ServoController.h"
#include "Esp32LedcRegistry.h"

//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "LedcTimer.h"
#include "LedcChannel.h"

namespace MDO {
namespace ESP32ServoController {


/*bool ServoController::privateSetSpeedMode(ledc_mode_t eSpeedMode) {
#if SOC_LEDC_SUPPORT_HS_MODE												//if supported
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {								//check high speed first												
		if (Esp32LedcRegistry::instance()->getNrOfHighSpeedTimers() > 0) {	//and available
			eSpeedMode = LEDC_HIGH_SPEED_MODE;
		}
	}
#endif
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {								//maybe not.. check low speed
		if (Esp32LedcRegistry::instance()->getNrOfLowSpeedTimers() > 0) {	//if available..
			eSpeedMode = LEDC_LOW_SPEED_MODE;
		}		
	}	
	
	if (eSpeedMode != LEDC_SPEED_MODE_MAX) {
		setSpeedMode(eSpeedMode);
	}
	
	return (eSpeedMode != LEDC_SPEED_MODE_MAX);
}*/

/**
 * Converts a 0-180 degree angle to a duty cycle for the channel.
 */
double ServoController::angleToDuty(double dAngle) const {
	
	const uint32_t uiServoFreq	= Esp32LedcRegistry::instance()->getServoFrequency();
	const uint32_t uiMinPosTime	= Esp32LedcRegistry::instance()->getServoMinPosTime();
	const uint32_t uiMaxPosTime	= Esp32LedcRegistry::instance()->getServoMaxPosTime();
	
	if (dAngle < 0) {
		dAngle = 0;
	} else if (dAngle > 180) {
		dAngle = 180;
	}
	
	double dHighTime_usec = (1000/uiServoFreq)*1000;	//assuming uiServoFreq never exceeds 1kHz
	double dDuty_usec = uiMinPosTime + (uiMaxPosTime - uiMinPosTime) * (dAngle / 180.0);
	double dDuty = dDuty_usec / dHighTime_usec;
	//MDO_SERVO_DEBUG_PRINTLN(String("angleToDuty - converted ") + dAngle + " degrees to duty: " + dDuty);
	return dDuty;
}

/*uint32_t ServoController::angleToDuty(uint8_t uiAngle) const {
	const uint32_t uiServoFreq	= Esp32LedcRegistry::instance()->getServoFrequency();
	const uint32_t uiMinPosTime	= Esp32LedcRegistry::instance()->getServoMinPosTime();
	const uint32_t uiMaxPosTime	= Esp32LedcRegistry::instance()->getServoMaxPosTime();
	
	if (uiAngle > 180) {
		uiAngle = 180;
	}
	
	uint32_t uiMaxResolutionValue = getTimer()->getMaxResolutionValue();
	uint32_t uiDuty = 0;
	return uiDuty;
}*/

uint32_t ServoController::getTimerFreqHz() const {
	uint32_t uiRet = 0;
	if (getTimer() != nullptr) {
		uiRet = getTimer()->getFrequency();
	}
	
	return uiRet;
}

ledc_mode_t ServoController::getSpeedMode() const {
	ledc_mode_t eSpeedMode = LEDC_SPEED_MODE_MAX;
	if (getTimer() != nullptr) {
		eSpeedMode = getTimer()->getSpeedMode();
	}
	return eSpeedMode;
}

uint8_t ServoController::getId() const {
	uint8_t uiRet = 0xFF;
	if (getChannel() != nullptr) {
		uiRet = getChannel()->getChannelNr();
	}
	
	return uiRet;
}

/**
 * Move to a certain angle (0-180 degrees)
 * iMaxTime_ms is the time the controller will take to 'fade' the PWM signal to the desired value (not the servo itself!)
 * bBlocking can make this a blocking call, when this is left non-blocking: no new command can be provided in the next iMaxTime_ms msec.
 */
bool ServoController::moveTo(double dAngle, int iMaxTime_ms /*= 100*/, bool bBlocking /*= false*/) {
	return fade(angleToDuty(dAngle), iMaxTime_ms, bBlocking);
}

/**
 * initializes a servo controller to a certain pin.
 * dInitialAngle is the initial angle which is in degrees from 0 to 180 
 */
bool ServoController::begin(const ServoFactoryDecorator& oFactory, int iPinNr, double dInitialAngle /*= 90.0*/) {

	if ((dInitialAngle < 0.0) || (dInitialAngle > 180.0)) {
		return false;
	}

	m_spTimer = oFactory.createTimer();
	if (m_spTimer != nullptr) {
		//bOk = createAndConfigureChannel(iPinNr, angleToDuty(dInitialAngle), false);
		m_spChannel = oFactory.createChannel(iPinNr, this, angleToDuty(dInitialAngle));
	}
	
	bool bOk = (m_spTimer != nullptr) && (m_spChannel != nullptr);
	if (bOk) {
		Esp32LedcRegistry::instance()->registerServo(this);
	} else {
		cleanUp();
	}

	return bOk;
}

//bool ServoController::begin(int iPinNr, uint8_t uiInitialAngle /*= 90*/, ledc_mode_t eSpeedMode /*= LEDC_SPEED_MODE_MAX*/) {
/*	bool bOk = privateSetSpeedMode(eSpeedMode);
	if (bOk) {
		const uint32_t uiServoFreq = Esp32LedcRegistry::instance()->getServoFrequency();
		bOk = createAndConfigureTimer(uiServoFreq);
	}
	
	if (bOk) {
		bOk = createAndConfigureChannel(iPinNr, angleToDuty(uiInitialAngle), false);
	}
	
	if (bOk) {
		Esp32LedcRegistry::instance()->registerServo(this);
	} else {
		cleanUp();
	}

	return bOk;
}*/

ServoController::ServoController() {
//: PWMController(LEDC_SPEED_MODE_MAX) {
}

ServoController::~ServoController() {
	Esp32LedcRegistry::instance()->unregisterServo(this);
}

}	//namespace end
}	//namespace end