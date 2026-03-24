#include "LedcTimerHighSpeed.h"


//#include "debug_config.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h" 

namespace MDO {
namespace ESP32ServoController {

ledc_mode_t LedcTimerHighSpeed::getSpeedModePrivate() const {
#ifdef SOC_LEDC_SUPPORT_HS_MODE
	return LEDC_HIGH_SPEED_MODE;
#else
	return LEDC_SPEED_MODE_MAX;	//to avoid a linker error, but ok: this is still a blocking issue
#endif	
}

/*virtual*/ uint8_t LedcTimerHighSpeed::getClockSource() const {
	return (uint8_t)m_eClockSource;
}

/*virtual*/ bool LedcTimerHighSpeed::begin(uint32_t uiFreqHz, uint8_t uiResolutionBits /* = 0 */) {
#ifdef SOC_LEDC_SUPPORT_HS_MODE
	return configure(uiFreqHz, uiResolutionBits, getClockSource());
#else
	return false;
#endif		
}

LedcTimerHighSpeed::LedcTimerHighSpeed(uint8_t uiTimerNr, enum LedcTimerHighSpeed_Source_t eClockSource /*= FAST_CLOCK_SOURCE_APB_CLK*/)
:LedcTimer(uiTimerNr, getSpeedModePrivate()) {
	m_eClockSource = eClockSource;
}

LedcTimerHighSpeed::~LedcTimerHighSpeed() {
}

}	//namespace end
}	//namespace end