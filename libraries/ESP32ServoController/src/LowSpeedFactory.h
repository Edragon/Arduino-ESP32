#ifndef _MDO_LowSpeedFactory_H
#define _MDO_LowSpeedFactory_H

#include "LedcTimerLowSpeed.h"
#include "Esp32LedcFactory.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A factory, able to make a timer and a channel, with the strategy of:
 * Only creates Ledc Timers and Channels from the LowSpeed type.
 * The clock source defaults to SLOW_CLOCK_SOURCE_APB
 * This will obviously only work when these resources are not all used yet
 */ 
class LowSpeedFactory: public Esp32LedcFactory {
	
	public:		//types
	protected:
		ledc_mode_t											m_eSpeedMode;
		enum LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t	m_eClockSource;
	
	public:
		virtual ledc_mode_t						getDefaultSpeedMode() const override;
		virtual std::shared_ptr<LedcTimer>		createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) const override;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) const override;
		
		void									setClockSource(enum LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t eClockSource);
		
		LowSpeedFactory();
		virtual ~LowSpeedFactory();
};

}	//namespace end
}	//namespace end

#endif