#ifndef _MDO_LedcTimerHighSpeed_H
#define _MDO_LedcTimerHighSpeed_H

#include "LedcTimer.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A high speed version of a Ledc timer.
 * Only available when the hardware supports this.
 */ 
class LedcTimerHighSpeed: public LedcTimer {
	
	public:	//types
		enum LedcTimerHighSpeed_Source_t {	//see enum ledc_clk_src_t
#ifdef SOC_LEDC_SUPPORT_REF_TICK
			FAST_CLOCK_SOURCE_REF_TICK = LEDC_REF_TICK,	//LEDC timer clock derived from reference tick (1Mhz)  
#endif
#if SOC_LEDC_SUPPORT_APB_CLOCK
			FAST_CLOCK_SOURCE_APB_CLK  = LEDC_APB_CLK	//LEDC timer clock derived from APB clock (80Mhz) 
#elif SOC_LEDC_SUPPORT_PLL_DIV_CLOCK
			FAST_CLOCK_SOURCE_PLL = LEDC_SCLK			//LEDC timer clock derived from PLL clock (96MHz) (for ESP32-H2)
#endif
		};
		
	private:
		enum LedcTimerHighSpeed_Source_t	m_eClockSource;
	
	private:
		ledc_mode_t			getSpeedModePrivate() const;
	
	public:
		virtual uint8_t		getClockSource() const;		
		virtual bool		begin(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0);
		
#if SOC_LEDC_SUPPORT_APB_CLOCK	
		LedcTimerHighSpeed(uint8_t uiTimerNr, enum LedcTimerHighSpeed_Source_t eClockSource = FAST_CLOCK_SOURCE_APB_CLK);
#elif SOC_LEDC_SUPPORT_PLL_DIV_CLOCK
		LedcTimerHighSpeed(uint8_t uiTimerNr, enum LedcTimerHighSpeed_Source_t eClockSource = FAST_CLOCK_SOURCE_PLL);
#endif		
	private:
		LedcTimerHighSpeed(const LedcTimerHighSpeed& oSrc);	//not implemented
	public:
		virtual ~LedcTimerHighSpeed();
};

}	//namespace end
}	//namespace end

#endif