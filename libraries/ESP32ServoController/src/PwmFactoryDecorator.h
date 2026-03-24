#ifndef _MDO_PwmFactoryDecorator_H
#define _MDO_PwmFactoryDecorator_H

#include "Esp32LedcFactory.h"

namespace MDO {
namespace ESP32ServoController {

class LedcTimer;

/**
 * A timer and channel factory-decorator specifically for a PWMController.
 * The purpose of this class is *maximally re-use timers where possible*, 
 * meaning that even if 'createTimer' is called, when a timer with the same frequency already is in use, that will be returned.
 * Note that this will ignore the provided uiResolutionBits in the 'is able to share'-decision.
 * The (mandatory) provided factory will determine which timer/channel type to create
 */ 
class PwmFactoryDecorator {
	
	public:		//types
	private:
		const Esp32LedcFactory&	m_oFactory;
	
	private:
	
	public:
		virtual std::shared_ptr<LedcTimer>		createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) const;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) const;		

		PwmFactoryDecorator(const Esp32LedcFactory& oFactory);
		virtual ~PwmFactoryDecorator();
};

}	//namespace end
}	//namespace end

#endif