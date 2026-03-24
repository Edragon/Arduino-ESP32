#ifndef _MDO_LedcTimerLowSpeed_H
#define _MDO_LedcTimerLowSpeed_H

#include "LedcTimer.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A low speed version of a Ledc timer.
 */ 
class LedcTimerLowSpeed: public LedcTimer {
	
	public:	//types
		enum LedcTimerLowSpeed_Source_t {	//see enum ledc_slow_clk_sel_t
			SLOW_CLOCK_SOURCE_RC_FAST	= LEDC_SLOW_CLK_RC_FAST		//LEDC low speed timer clock source is RC_FAST clock (~ 8 MHz for ESP32, ~ 20 MHz for ESP32-C3)
#if SOC_LEDC_SUPPORT_APB_CLOCK			
			,SLOW_CLOCK_SOURCE_APB		= LEDC_SLOW_CLK_APB			//LEDC low speed timer clock source is 80MHz APB clock 
#endif
#if SOC_LEDC_SUPPORT_XTAL_CLOCK			
			,SLOW_CLOCK_SOURCE_XTAL		= LEDC_SLOW_CLK_XTAL		//ESP32-C3 40 MHz
#endif
		};


	private:
		enum LedcTimerLowSpeed_Source_t m_eClockSource;
	
	private:
		ledc_mode_t			getSpeedModePrivate() const;
	
	public:
		virtual uint8_t		getClockSource() const;		
		virtual bool		begin(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0);
	
#if SOC_LEDC_SUPPORT_APB_CLOCK	
		LedcTimerLowSpeed(uint8_t uiTimerNr, enum LedcTimerLowSpeed_Source_t eClockSource = SLOW_CLOCK_SOURCE_APB);
#else
		LedcTimerLowSpeed(uint8_t uiTimerNr, enum LedcTimerLowSpeed_Source_t eClockSource = SLOW_CLOCK_SOURCE_RC_FAST);
#endif
	private:
		LedcTimerLowSpeed(const LedcTimerLowSpeed& oSrc);	//not implemented
	public:
		virtual ~LedcTimerLowSpeed();
};

}	//namespace end
}	//namespace end

#endif