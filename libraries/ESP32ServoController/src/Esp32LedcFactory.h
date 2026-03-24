#ifndef _MDO_Esp32LedcFactory_H
#define _MDO_Esp32LedcFactory_H

#include <driver/ledc.h>
#include <memory>

namespace MDO {
namespace ESP32ServoController {

class LedcTimer;
class LedcChannel;

/**
 * Abstract factory base class. Creates an interface for creating Ledc Timers and Ledc Channels
 * See: https://refactoring.guru/design-patterns/abstract-factory
 */ 
class Esp32LedcFactory {
	
	public:		//types
	private:	
	
	private:
	
	protected:
		uint8_t 								getTimerResolutionBits(uint8_t uiResolutionBits, LedcTimer* pTimer, uint32_t uiFreqHz) const;	
	
	public:
		virtual ledc_mode_t						getAlternativeSpeedMode() const;
		virtual bool							supportAlternativeSpeedMode() const;
		virtual ledc_mode_t						getDefaultSpeedMode() const = 0;
		virtual std::shared_ptr<LedcTimer>		createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits = 0) const = 0;
		virtual std::shared_ptr<LedcChannel>	createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput = false) const = 0;

		Esp32LedcFactory();
		virtual ~Esp32LedcFactory();
};

}	//namespace end
}	//namespace end

#endif