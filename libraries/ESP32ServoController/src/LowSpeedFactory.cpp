#include "LowSpeedFactory.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h"
#include "LedcChannelLowSpeed.h"

namespace MDO {
namespace ESP32ServoController {

/*virtual*/ ledc_mode_t LowSpeedFactory::getDefaultSpeedMode() const {
	return m_eSpeedMode;
}

/**
 * Creates a Ledc High Speed timer if available.
 * When uiResolutionBits is not set (or 0), this method will try to determine the maximum value available
 */
/*virtual*/ std::shared_ptr<LedcTimer> LowSpeedFactory::createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits /*= 0*/) const {
	
	if ((m_eSpeedMode == LEDC_SPEED_MODE_MAX) || (uiFreqHz == 0)) {
		return nullptr;
	}
	
	std::shared_ptr<LedcTimer> psRet;	
	uint8_t uiTimerNr = Esp32LedcRegistry::instance()->getFirstAvailableTimer(m_eSpeedMode);
	bool bOk = uiTimerNr != LEDC_TIMER_MAX;
	if (bOk) {
		//MDO_SERVO_DEBUG_PRINTLN("  Creating timer");
		psRet.reset(new LedcTimerLowSpeed(uiTimerNr));
	}
	
	bOk = bOk && (psRet != nullptr);
	if (bOk) {
		//MDO_SERVO_DEBUG_PRINTLN("  Configure timer");
		bOk = psRet->begin(	uiFreqHz, 					//freq as requested
							getTimerResolutionBits(uiResolutionBits, psRet.get(), uiFreqHz));
	}

	if (bOk) {
		return psRet;
	}
	return nullptr;
}

/*virtual*/ std::shared_ptr<LedcChannel> LowSpeedFactory::createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false*/) const {
	
	if ((m_eSpeedMode == LEDC_SPEED_MODE_MAX) || (pTimer == nullptr)) {
		return nullptr;
	}
	
	std::shared_ptr<LedcChannel> psRet;
	uint8_t uiChannelNr = Esp32LedcRegistry::instance()->getFirstAvailableChannel(m_eSpeedMode);
	bool bOk = uiChannelNr != LEDC_CHANNEL_MAX;
	if (bOk) {	
		//MDO_SERVO_DEBUG_PRINTLN("Creating channel");
		psRet.reset(new LedcChannelLowSpeed(uiChannelNr));
	}
	
	bOk = bOk && (psRet != nullptr);
	if (bOk) {
		//uint32_t uiDuty = 0;
		//int iHighPoint = 0;
		//dutyToInt(dDuty, uiDuty, iHighPoint);			
		bOk = psRet->begin(iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
	}
	
	if (bOk) {
		return psRet;
	}
	return nullptr;
}


void LowSpeedFactory::setClockSource(enum LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t eClockSource) {
	m_eClockSource = eClockSource;
}

LowSpeedFactory::LowSpeedFactory() {
#if SOC_LEDC_SUPPORT_APB_CLOCK	
	m_eClockSource = LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t::SLOW_CLOCK_SOURCE_APB;
#else
	m_eClockSource = LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t::SLOW_CLOCK_SOURCE_XTAL;
#endif	
	m_eSpeedMode = LEDC_LOW_SPEED_MODE;
}

LowSpeedFactory::~LowSpeedFactory() {
}

}	//namespace end
}	//namespace end