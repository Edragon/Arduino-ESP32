#include "Esp32LedcFactory.h"


//#include "debug_config.h"
//#include <mdomisc.h>

#include "LedcTimer.h"

namespace MDO {
namespace ESP32ServoController {

//protected:
uint8_t Esp32LedcFactory::getTimerResolutionBits(uint8_t uiResolutionBits, LedcTimer* pTimer, uint32_t uiFreqHz) const {
	if ((uiResolutionBits != 0) || (pTimer == nullptr)){
		//a resolution in bits has been selected already
		return uiResolutionBits;
	}
	//in this case we will try to determine the maximum value, available to this timer, with the desired frequency
	return pTimer->findMaxResolution(pTimer->getSourceClockFrequency(), uiFreqHz);
}

/*virtual*/ ledc_mode_t Esp32LedcFactory::getAlternativeSpeedMode() const {
	return LEDC_SPEED_MODE_MAX;	//which is a safe default (since it's incorrect to be used usefully)
}

/*virtual*/ bool Esp32LedcFactory::supportAlternativeSpeedMode() const {
	return false;
}

Esp32LedcFactory::Esp32LedcFactory() {
}

Esp32LedcFactory::~Esp32LedcFactory() {
}

}	//namespace end
}	//namespace end