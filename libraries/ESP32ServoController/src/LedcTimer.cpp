#include "LedcTimer.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h" 
#include <esp_clk_tree.h>

namespace MDO {
namespace ESP32ServoController {

/**
 * protected: Configure the timer.
 * That that the frequency and the duty resolution are interdependent
 * setting uiResolutionBits to 0 means 'use max'
 */
bool LedcTimer::configure(uint32_t uiFreqHz, uint8_t uiResolutionBits, uint8_t uiClockSource) {
	//MDO_SERVO_DEBUG_PRINTLN(String("Configuring timer: ") + uiFreqHz + ", " + uiResolutionBits + ", " + uiClockSource);
	if (isInitialized()) {
		//if we're already initialized. I guess that an explicit deconfigure should be required first
		MDO_SERVO_DEBUG_PRINTLN("Configure called when already configued");
		return false;
	}
	
	if (uiResolutionBits == 0) {
		uiResolutionBits = Esp32LedcRegistry::instance()->getMaxTimerResolutionBits();
	}
	
	if ((uiResolutionBits == 0) || (uiResolutionBits > 20)) {
		MDO_SERVO_DEBUG_PRINTLN("Resolution out of bounds");
		return false;
	}
	if (uiFreqHz == 0) {
		MDO_SERVO_DEBUG_PRINTLN("Invalid frequency for a new timer");
		return false;
	}
	
	if (Esp32LedcRegistry::instance()->isTimerInUse(this)) {
		MDO_SERVO_DEBUG_PRINTLN("Timer already in use. Wrong timer number?");
		return false;		
	}
	
	//MDO_SERVO_DEBUG_PRINTLN(String("Actual timer config: ") + uiFreqHz + ", " + uiResolutionBits + ", " + uiClockSource);
	//delay(100);	
	
	ledc_timer_config_t sConfig;
	sConfig.speed_mode 		= getSpeedMode();
	sConfig.duty_resolution = (ledc_timer_bit_t)uiResolutionBits;
	sConfig.timer_num 		= (ledc_timer_t)getTimerNr();
	sConfig.freq_hz 		= uiFreqHz;
	sConfig.clk_cfg			= (ledc_clk_cfg_t)uiClockSource;	//not any clock souce can be selected, based on the hardware platform. ESp32-ledc will report when this was wrong.
	sConfig.deconfigure 	= false;
	
	
	bool bOk = ledc_timer_config(&sConfig) == ESP_OK;
	if (bOk) {
		m_uiFreqHz = uiFreqHz;
		m_uiMaxResolutionValue = 1<<uiResolutionBits;
		Esp32LedcRegistry::instance()->registerTimer(this);
	} else {
		MDO_SERVO_DEBUG_PRINTLN(String("LEDC timer config failed: ") + getTimerNr() + ", " + uiFreqHz + ", " + getSpeedMode() + ", " + uiResolutionBits + ", " + uiClockSource);
	}
	m_bInitialized = bOk;
	m_bRunning = bOk;
	return bOk;
}

uint32_t LedcTimer::getMaxResolutionValue() const {
	return m_uiMaxResolutionValue;
}

/**
 * Deconfigures this times
 * Tries to pause the timer when it's running
 */
bool LedcTimer::end() {
	if (!isInitialized()) {
		return true;	//I guess we're done when not initialized at all.. ?
	}
	ledc_timer_config_t sConfig;
	sConfig.speed_mode	= m_eSpeedMode;
	sConfig.timer_num	= (ledc_timer_t)getTimerNr();
	sConfig.deconfigure	= true;
	
	bool bOk = true;
	if (m_bRunning) {
		bOk = ledc_timer_pause(m_eSpeedMode, (ledc_timer_t)getTimerNr()) == ESP_OK;
		m_bRunning = !bOk;
	}
	
	bOk = (bOk) && (ledc_timer_config(&sConfig) == ESP_OK);
	if (bOk) {
		m_uiFreqHz = 0;
		m_uiMaxResolutionValue = 0;
		m_bInitialized = false;
	}
	return bOk;
}

bool LedcTimer::isInitialized() const {
	return m_bInitialized;
}

/**
 * Updates the alreayd configured timer to a new frequency.
 * Note, will only work when a proper pre-divider number base on the given frequency and the current duty_resolution (from configure) can be determined
 */
bool LedcTimer::updateFrequency(uint32_t uiFreqHz) {
	if (!isInitialized()) {
		return false;
	}
	bool bOk = ledc_set_freq(getSpeedMode(), (ledc_timer_t)getTimerNr(), uiFreqHz) == ESP_OK;
	if (bOk) {
		m_uiFreqHz = uiFreqHz;
	}
	return bOk;
}

/**
 * Gets the frequency from the hardware.
 */
uint32_t LedcTimer::getHardwareFrequency() const {
	if (!isInitialized()) {
		return 0;
	}
	return ledc_get_freq(getSpeedMode(), (ledc_timer_t)getTimerNr());
}

uint32_t LedcTimer::getFrequency() const {
	return m_uiFreqHz;
}

uint8_t LedcTimer::getTimerNr() const {
	return m_uiTimerNr;	//should be 0..3 (typically)
}

ledc_mode_t	LedcTimer::getSpeedMode() const {
	return m_eSpeedMode;
}

/**
 * Gets the source frequency of a clock source (which must be from soc_module_clk_t type).
 * When not provided, uses the one from this instance
 */
uint32_t LedcTimer::getSourceClockFrequency(uint8_t uiClockSource /*= 0*/) const {
	//see: C:\temp\esp32\esp-idf-master\components\soc\esp32\include\soc\clk_tree_defs.h => soc_module_clk_t
	
	if (uiClockSource == 0) {
		uint8_t uiSlowClockSource = getClockSource();
#if SOC_LEDC_SUPPORT_APB_CLOCK		
		if (uiSlowClockSource == LEDC_APB_CLK) {
			uiClockSource = SOC_MOD_CLK_APB;
		}
#endif		
#ifdef SOC_LEDC_SUPPORT_REF_TICK		
		if (uiSlowClockSource == LEDC_REF_TICK) {
			uiClockSource = SOC_MOD_CLK_REF_TICK;
		}
#endif
		if (uiSlowClockSource == LEDC_SLOW_CLK_RC_FAST) {
			uiClockSource = SOC_MOD_CLK_RC_FAST;
		}
#if SOC_LEDC_SUPPORT_XTAL_CLOCK		
		if (uiSlowClockSource == LEDC_SLOW_CLK_XTAL) {
			uiClockSource = SOC_MOD_CLK_XTAL;
		}
#endif
	}
	
	uint32_t uiFreqHz = 0;
	if (esp_clk_tree_src_get_freq_hz(	(soc_module_clk_t)uiClockSource,
										(esp_clk_tree_src_freq_precision_t)0, 
										&uiFreqHz) 									== ESP_OK) {	
		return uiFreqHz;
	}
	MDO_SERVO_DEBUG_PRINTLN("Source clock frequency not determined..");
	return 0;
}

/**
 * Find the maximum resolution (in bits), for the provided clock frequency and desired frequency
 * Returns 0 on error
 */
uint8_t LedcTimer::findMaxResolution(uint32_t uiSourceClockFreqHz, uint32_t uiDesiredFreqHz) const {
	if ((uiSourceClockFreqHz == 0) || (uiDesiredFreqHz == 0)) {
		MDO_SERVO_DEBUG_PRINTLN("findMaxResolution - invalid parameter");
		return 0;
	}
	//MDO_SERVO_DEBUG_PRINTLN(String("findMaxResolution: ") + uiSourceClockFreqHz + ", " + uiDesiredFreqHz);
	return (uint8_t)ledc_find_suitable_duty_resolution(uiSourceClockFreqHz, uiDesiredFreqHz);
}

LedcTimer::LedcTimer(uint8_t uiTimerNr, ledc_mode_t eSpeedMode) {
	m_uiTimerNr = uiTimerNr;
	m_bInitialized = false;
	m_uiFreqHz = 0;
	m_bRunning = false;
	m_uiMaxResolutionValue = 0;
	m_eSpeedMode = eSpeedMode;
}

LedcTimer::~LedcTimer() {
	Esp32LedcRegistry::instance()->unregisterTimer(this);
	end();
}

}	//namespace end
}	//namespace end