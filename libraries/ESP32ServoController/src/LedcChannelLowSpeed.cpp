#include "LedcChannelLowSpeed.h"


//#include "debug_config.h"
//#include <mdomisc.h>



namespace MDO {
namespace ESP32ServoController {

ledc_mode_t LedcChannelLowSpeed::getSpeedModePrivate() const {
	return LEDC_LOW_SPEED_MODE;
}

/*virtual*/ bool LedcChannelLowSpeed::begin(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false*/) {
	return configure(iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
}

LedcChannelLowSpeed::LedcChannelLowSpeed(uint8_t uiChannelNr)
:LedcChannel(uiChannelNr, getSpeedModePrivate()) {
}

LedcChannelLowSpeed::~LedcChannelLowSpeed() {
}

}	//namespace end
}	//namespace end