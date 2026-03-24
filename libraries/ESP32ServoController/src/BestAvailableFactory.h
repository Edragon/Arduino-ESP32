#ifndef _MDO_BestAvailableFactory_H
#define _MDO_BestAvailableFactory_H

#include "LowSpeedFactory.h"
#include "HighSpeedFactory.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A factory, able to make a timer and a channel, with the strategy of 'provides the best available' (based on hardware capabilities and current usage)
 */ 
class BestAvailableFactory: public Esp32LedcFactory {
	
	public:		//types
	private:
		LowSpeedFactory		m_oLowSpeedFactory;		//do a has-a, over a is-a
		HighSpeedFactory	m_oHighSpeedFactory;	//  related to the diamond-shaded inheritance issues
	
	private:
	
	public:
		virtual ledc_mode_t						getAlternativeSpeedMode() const override;
		virtual bool							supportAlternativeSpeedMode() const override;	
		virtual ledc_mode_t						getDefaultSpeedMode() const override;
		
		virtual std::shared_ptr<LedcTimer>		createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) const override;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) const override;

		void									setClockSources(enum LedcTimerLowSpeed::LedcTimerLowSpeed_Source_t eClockSourceLowSpeed, enum LedcTimerHighSpeed::LedcTimerHighSpeed_Source_t eClockSourceHighSpeed);
		BestAvailableFactory();
		virtual ~BestAvailableFactory();
};

}	//namespace end
}	//namespace end

#endif