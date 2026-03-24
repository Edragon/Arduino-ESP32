#ifndef _MDO_LedcChannelHighSpeed_H
#define _MDO_LedcChannelHighSpeed_H

#include "LedcChannel.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A high speed version of a Ledc channel.
 * Only available when the hardware supports this.
 */ 
class LedcChannelHighSpeed: public LedcChannel {
	
	private:
	
	private:
		ledc_mode_t			getSpeedModePrivate() const;
	
	public:
		virtual bool		begin(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false);
	
		LedcChannelHighSpeed(uint8_t uiChannelNr);
		virtual ~LedcChannelHighSpeed();
};

}	//namespace end
}	//namespace end

#endif