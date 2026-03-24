#ifndef _MDO_LedcChannel_H
#define _MDO_LedcChannel_H

#include <Arduino.h>

namespace MDO {
namespace ESP32ServoController {

class LedcTimer;

/**
 * An abstract base class for Ledc channel types
 */ 
class LedcChannel {
	
	private:
		uint8_t		m_uiChannelNr;	//0..7
		bool		m_bInitialized;
		LedcTimer*	m_pTimer;
	
	protected:
		ledc_mode_t	m_eSpeedMode;	
	
	private:
		bool				checkDutyAndHighPoint(uint32_t uiDuty, uint32_t uiHighPoint);
	
	protected:
		bool				configure(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false);
	
	public:
		bool				fade(uint32_t uiTargetDuty, int iMaxFadeTime_ms, bool bBlocking = false);
		bool				disableOutput(uint32_t uiIdleLevel = 0);
		uint32_t 			getHardwareDuty() const;
		bool				updateDuty(uint32_t uiDuty, uint32_t uiHighPoint);
		uint8_t 			getChannelNr() const;
		ledc_mode_t			getSpeedMode() const;
		bool				isInitialized() const;
		bool				addPin(int iPinNr);

		virtual bool		begin(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) = 0;	
	
		LedcChannel(uint8_t	uiChannelNr, ledc_mode_t eSpeedMode);
	private:
		LedcChannel(const LedcChannel& oSrc);	//not implemented
	public:			
		virtual ~LedcChannel();
};

}	//namespace end
}	//namespace end

#endif