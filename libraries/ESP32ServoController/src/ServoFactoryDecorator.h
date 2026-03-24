#ifndef _MDO_ServoFactoryDecorator_H
#define _MDO_ServoFactoryDecorator_H

#include "Esp32LedcFactory.h"

namespace MDO {
namespace ESP32ServoController {

class ServoController;

/**
 * A timer and channel factory specifically for a ServoController.
 * (easier to use compared to the normal Factory types based on defaults retrieved from Esp32LedcRegistry)
 */ 
class ServoFactoryDecorator {
	
	public:		//types
	private:
		const Esp32LedcFactory&	m_oFactory;
	
	private:
	
	public:
		virtual std::shared_ptr<LedcTimer>		createTimer(uint8_t uiResolutionBits = 0) const;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, ServoController* pServoController, double dDuty, bool bInvertOutput = false) const;	

		ServoFactoryDecorator(const Esp32LedcFactory& oFactory);
		virtual ~ServoFactoryDecorator();
};

}	//namespace end
}	//namespace end

#endif