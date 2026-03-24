#ifndef _MDO_HighSpeedFactory_H
#define _MDO_HighSpeedFactory_H

#include "LedcTimerHighSpeed.h"
#include "Esp32LedcFactory.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A factory, able to make a timer and a channel, with the strategy of:
 * Only creates Ledc Timers and Channels from the HighSpeed type.
 * The clock source defaults to FAST_CLOCK_SOURCE_APB_CLK
 * This will obviously only work for supported hardware.
 */ 
class HighSpeedFactory: public Esp32LedcFactory {
	
	public:		//types
		
	protected:
		ledc_mode_t												m_eSpeedMode;
		enum LedcTimerHighSpeed::LedcTimerHighSpeed_Source_t	m_eClockSource;
	
	public:
		virtual ledc_mode_t						getDefaultSpeedMode() const override;
		virtual std::shared_ptr<LedcTimer>		createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) const override;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) const override;
		
		void									setClockSource(enum LedcTimerHighSpeed::LedcTimerHighSpeed_Source_t eClockSource);
		
		HighSpeedFactory();
		virtual ~HighSpeedFactory();
};

}	//namespace end
}	//namespace end

#endif