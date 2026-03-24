#ifndef _MDO_LedcTimer_H
#define _MDO_LedcTimer_H

#include <driver/ledc.h>
#include <Arduino.h>

namespace MDO {
namespace ESP32ServoController {

/**
 * An abstract base class for Ledc timer types
 */ 
class LedcTimer {
	
	private:
		uint8_t		m_uiTimerNr;								//0..3 (typically)
		bool		m_bInitialized;
		uint32_t	m_uiFreqHz;
		bool		m_bRunning;
		uint32_t	m_uiMaxResolutionValue;
		ledc_mode_t	m_eSpeedMode;
	
	private:
	
	protected:
		bool 				configure(uint32_t uiFreqHz, uint8_t uiResolutionBits, uint8_t uiClockSource);
	
	public:
		virtual uint8_t		getClockSource() const = 0;
		uint32_t			getMaxResolutionValue() const;
		bool				end();
		bool				isInitialized() const;
		
		bool				updateFrequency(uint32_t uiFreqHz);
		uint32_t			getHardwareFrequency() const;
		uint32_t			getFrequency() const;
		uint8_t 			getTimerNr() const;
		ledc_mode_t			getSpeedMode() const;
	
		uint32_t			getSourceClockFrequency(uint8_t uiClockSource = 0) const;
		uint8_t				findMaxResolution(uint32_t uiSourceClockFreqHz, uint32_t uiDesiredFreqHz) const;
		virtual bool 		begin(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) = 0;
	
		LedcTimer(uint8_t uiTimerNr, ledc_mode_t eSpeedMode);
	private:
		LedcTimer(const LedcTimer& oSrc);	//not implemented
	public:		
		virtual ~LedcTimer();
};

}	//namespace end
}	//namespace end

#endif