#include "ServoFactoryDecorator.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h"
#include "ServoController.h"


namespace MDO {
namespace ESP32ServoController {

/*virtual*/ /*ledc_mode_t ServoFactoryDecorator::getAlternativeSpeedMode() const {
	return m_oFactory.getAlternativeSpeedMode();
}

/*virtual*/ /*bool ServoFactoryDecorator::supportAlternativeSpeedMode() const {
	return m_oFactory.supportAlternativeSpeedMode();
}

/*virtual*/ /*ledc_mode_t ServoFactoryDecorator::getDefaultSpeedMode() const {
	return m_oFactory.getDefaultSpeedMode();
}
*/

/*virtual*/ std::shared_ptr<LedcTimer> ServoFactoryDecorator::createTimer(uint8_t uiResolutionBits /*= 0*/) const {
	
	const Esp32LedcRegistry* pRegistry(Esp32LedcRegistry::instance());
	const uint32_t uiServoFreq = pRegistry->getServoFrequency();
	
	{	//hide some local variables for later
		const ledc_mode_t eDefaultSpeedMode = m_oFactory.getDefaultSpeedMode();
		
		const ServoController* pOtherServo = pRegistry->getServoUsing(uiServoFreq, eDefaultSpeedMode);
		if (pOtherServo != nullptr) {
			//ok, good. We'll re-use the timer of the found other ServoController.
			MDO_SERVO_DEBUG_PRINTLN("Servo - share a timer");
			return pOtherServo->getTimer();
		}
		
		//ok, so no simular timer-requirement-servo exists currently
		//see if it's possible to allocate a timer as-per the default SpeedMode wishes
		if (pRegistry->getFirstAvailableTimer(eDefaultSpeedMode) != LEDC_TIMER_MAX) {
			//at least one timer for the requested default SpeedMode exists
			//so use that
			MDO_SERVO_DEBUG_PRINTLN("Servo - create a timer");
			return m_oFactory.createTimer(uiServoFreq);
		}
	}
	
	//at this point, the default speed mode seems to have been depleted
	//see if we can switch to an alternative speed mode
	if (m_oFactory.supportAlternativeSpeedMode()) {
		const ledc_mode_t eAlternativeSpeedMode = m_oFactory.getAlternativeSpeedMode();
		
		const ServoController* pOtherServo = pRegistry->getServoUsing(uiServoFreq, eAlternativeSpeedMode);
		if (pOtherServo != nullptr) {
			//ok, good. We'll re-use the timer of the found other ServoController.
			MDO_SERVO_DEBUG_PRINTLN("Servo - share a timer");
			return pOtherServo->getTimer();
		}
		
		//ok, so no simular timer-requirement-servo exists currently
		//see if it's possible to allocate a timer as-per the alternative SpeedMode
		if (pRegistry->getFirstAvailableTimer(eAlternativeSpeedMode) != LEDC_TIMER_MAX) {
			//at least one timer for the requested SpeedMode exists
			//so use that
			MDO_SERVO_DEBUG_PRINTLN("Servo - create a alternative speed mode timer");
			return m_oFactory.createTimer(uiServoFreq);	//since there are no alternatives, this will revert to the second-best option
		}			
	}
	
	//well.. no alternative left.
	return nullptr;
}

/*virtual*/ std::shared_ptr<LedcChannel> ServoFactoryDecorator::createChannel(int iPinNr, ServoController* pServoController, double dDuty, bool bInvertOutput /*= false*/) const {
	if (pServoController == nullptr) {
		return nullptr;
	}
	uint32_t uiDuty = 0;
	int iHighPoint = 0;
	if (!pServoController->dutyToInt(dDuty, uiDuty, iHighPoint)) {
		return nullptr;
	}
	
	return m_oFactory.createChannel(iPinNr, pServoController->getTimer().get(), uiDuty, iHighPoint, bInvertOutput);
}

ServoFactoryDecorator::ServoFactoryDecorator(const Esp32LedcFactory& oFactory)
:m_oFactory(oFactory) {
}

ServoFactoryDecorator::~ServoFactoryDecorator() {
}

}	//namespace end
}	//namespace end