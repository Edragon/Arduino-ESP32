#include "LedcTimerLowSpeed.h"

//#include "debug_config.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h" 

namespace MDO {
namespace ESP32ServoController {

/*virtual*/ ledc_mode_t LedcTimerLowSpeed::getSpeedModePrivate() const {
	return LEDC_LOW_SPEED_MODE;
}

/*virtual*/ uint8_t LedcTimerLowSpeed::getClockSource() const {
	return (uint8_t)m_eClockSource;
}

/*virtual*/ bool LedcTimerLowSpeed::begin(uint32_t uiFreqHz, uint8_t uiResolutionBits /* = 0*/) {
	return configure(uiFreqHz, uiResolutionBits, getClockSource());
}

LedcTimerLowSpeed::LedcTimerLowSpeed(uint8_t uiTimerNr, enum LedcTimerLowSpeed_Source_t eClockSource /*= SLOW_CLOCK_SOURCE_APB*/)
:LedcTimer(uiTimerNr, getSpeedModePrivate()) {
	m_eClockSource = eClockSource;
}

LedcTimerLowSpeed::~LedcTimerLowSpeed() {
}

}	//namespace end
}	//namespace end