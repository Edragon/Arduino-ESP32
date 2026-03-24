#include "BestAvailableFactory.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h"

namespace MDO {
namespace ESP32ServoController {

/*virtual*/ ledc_mode_t BestAvailableFactory::getAlternativeSpeedMode() const {
	return m_oLowSpeedFactory.getDefaultSpeedMode();
}

/*virtual*/ bool BestAvailableFactory::supportAlternativeSpeedMode() const {
	return m_oHighSpeedFactory.getDefaultSpeedMode() != LEDC_SPEED_MODE_MAX;
}
	
/*virtual*/ ledc_mode_t BestAvailableFactory::getDefaultSpeedMode() const {
	ledc_mode_t eSpeedMode = m_oHighSpeedFactory.getDefaultSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		eSpeedMode = m_oLowSpeedFactory.getDefaultSpeedMode();
	}
	return eSpeedMode;
}

/*virtual*/ std::shared_ptr<LedcTimer> BestAvailableFactory::createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits /*= 0*/) const {
	
	Esp32LedcRegistry* pSettings = Esp32LedcRegistry::instance();
	
	if ((pSettings->getNrOfHighSpeedTimers()) > 0 &&
		(pSettings->getFirstAvailableTimer(m_oHighSpeedFactory.getDefaultSpeedMode()) != LEDC_TIMER_MAX) &&
		(pSettings->getFirstAvailableChannel(m_oHighSpeedFactory.getDefaultSpeedMode()) != LEDC_CHANNEL_MAX)) {	//also check if a channel could be allocated as well, if not the timer itself will not help 
		return m_oHighSpeedFactory.createTimer(uiFreqHz, uiResolutionBits);
	}
	
	//high speed not available, revert to low speed
	return m_oLowSpeedFactory.createTimer (uiFreqHz, uiResolutionBits);
}

/*virtual*/ std::shared_ptr<LedcChannel> BestAvailableFactory::createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false*/) const {

	if (pTimer == nullptr) {
		return nullptr;
	}
	
	ledc_mode_t eSpeedMode = pTimer->getSpeedMode();	//it *must* match the timer
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		//invalid speed mode..
		return nullptr;
	}
	
	Esp32LedcRegistry* pSettings = Esp32LedcRegistry::instance();
	if (pSettings->getFirstAvailableTimer(eSpeedMode) != LEDC_TIMER_MAX) {
		if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
			return m_oLowSpeedFactory.createChannel (iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
		} else {
			return m_oHighSpeedFactory.createChannel(iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
		}
	}
	
	//no channels available (anymore) matching the required speed mode
	return nullptr;
}

void BestAvailableFactory::setClockSources(enum LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t eClockSourceLowSpeed, enum LedcTimerHighSpeed::LedcTimerHighSpeed_Source_t eClockSourceHighSpeed){
	m_oLowSpeedFactory.setClockSource(eClockSourceLowSpeed);
	m_oHighSpeedFactory.setClockSource(eClockSourceHighSpeed);
}

BestAvailableFactory::BestAvailableFactory() {
}

BestAvailableFactory::~BestAvailableFactory() {
}

}	//namespace end
}	//namespace end