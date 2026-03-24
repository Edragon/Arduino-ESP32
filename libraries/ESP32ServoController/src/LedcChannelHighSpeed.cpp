#include "LedcChannelHighSpeed.h"

#include <driver/ledc.h>

//#include "debug_config.h"
//#include <mdomisc.h>



namespace MDO {
namespace ESP32ServoController {


//so this will result in a linker error when used while not supported
ledc_mode_t LedcChannelHighSpeed::getSpeedModePrivate() const {
#ifdef SOC_LEDC_SUPPORT_HS_MODE	
	return LEDC_HIGH_SPEED_MODE;
#else
	return LEDC_SPEED_MODE_MAX;	//to avoid a linker error, but ok: this is still a blocking issue
#endif		
}

/*virtual*/ bool LedcChannelHighSpeed::begin(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false*/) {
#ifdef SOC_LEDC_SUPPORT_HS_MODE	
	return configure(iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
#else
	return false;
#endif
}

LedcChannelHighSpeed::LedcChannelHighSpeed(uint8_t uiChannelNr)
:LedcChannel(uiChannelNr, getSpeedModePrivate()) {
}

LedcChannelHighSpeed::~LedcChannelHighSpeed() {
}

}	//namespace end
}	//namespace end