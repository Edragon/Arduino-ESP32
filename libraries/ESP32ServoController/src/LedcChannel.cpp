#include "LedcChannel.h"

#include <driver/ledc.h>

//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h" 
#include "LedcTimer.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * Private: checks the Duty and HighPoint
 * returns true for OK
 * m_pTimer must be set for this to work
 */
bool LedcChannel::checkDutyAndHighPoint(uint32_t uiDuty, uint32_t uiHighPoint) {
	if (m_pTimer == 0) {
		return false;
	}
	const uint32_t uiMaxTimerValue = m_pTimer->getMaxResolutionValue();
	if (uiDuty > uiMaxTimerValue) {
		MDO_SERVO_DEBUG_PRINTLN(String("Channel: uiDuty (") + uiDuty + ") must be <= the max timer resolution value (" + uiMaxTimerValue + ")");
		return false;
	}
	const uint32_t uiMaxHighPoint = uiMaxTimerValue-1;
	if (uiHighPoint > uiMaxHighPoint) {
		MDO_SERVO_DEBUG_PRINTLN(String("Channel: uiHighPoint (") + uiHighPoint + ") must be <= the [max timer resolution value -1] (" + uiMaxHighPoint + ")");
		return false;
	}
	return true;
}

bool LedcChannel::configure(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false */) {
	
	if ((iHighPoint < 0) || (pTimer == 0)) { 
		MDO_SERVO_DEBUG_PRINTLN("Parameter error");
		return false;
	}
	if (!pTimer->isInitialized()) {
		MDO_SERVO_DEBUG_PRINTLN("Channel: timer not initialized");
	}
	
	if (pTimer->getSpeedMode() != getSpeedMode()) {	//cannot mix [high speed & low speed] for [timer & channel]
		MDO_SERVO_DEBUG_PRINTLN("Speed mode mismatch (timer/channel)");
		return false;
	}
	if (getChannelNr() >= Esp32LedcRegistry::instance()->getNrOfChannels()) {
		//channel number not supported for the configured chip hardware
		MDO_SERVO_DEBUG_PRINTLN("Unsupported channel number");
		return false;
	}
	
	if (Esp32LedcRegistry::instance()->isChannelInUse(this)) {
		MDO_SERVO_DEBUG_PRINTLN(String("Channel already in use: ") + getChannelNr());
		return false;
	}
	
//	if (isInitialized()) {
//		//if we're already initialized.. maybe this should require some special action first?
//		return false;
//	}	
	
	m_pTimer = pTimer;
	if (!checkDutyAndHighPoint(uiDuty, iHighPoint)) {
		MDO_SERVO_DEBUG_PRINTLN("Duty / highpoint parameter error");
		return false;
	}	
	
	ledc_channel_config_t sConfig;
	sConfig.gpio_num			= iPinNr;
	sConfig.speed_mode			= getSpeedMode();
	sConfig.channel				= (ledc_channel_t)getChannelNr();
	sConfig.intr_type			= LEDC_INTR_DISABLE;					//for now, from ledc_intr_type_t, other option: LEDC_INTR_FADE_END
	sConfig.timer_sel			= (ledc_timer_t)pTimer->getTimerNr();
	sConfig.duty				= uiDuty;
	sConfig.hpoint				= iHighPoint;
	sConfig.sleep_mode			= Esp32LedcRegistry::instance()->getSleepPowerMode();
	sConfig.flags.output_invert	= bInvertOutput ? 1:0;
	
	bool bOk = ledc_channel_config(&sConfig) == ESP_OK;
	m_bInitialized = bOk;
	if (bOk) {
		Esp32LedcRegistry::instance()->registerChannel(this);
	} else {
		MDO_SERVO_DEBUG_PRINTLN("LEDC channel config error");
	}
	return bOk;
}

/**
 * Fades the PWM signal to a target duty cycle
 * When selecting bBlocking to be false:	For ESP32, hardware does not support any duty change while a fade operation is running in progress on that channel. 
 * 											Other duty operations will have to wait until the fade operation has finished.
 */
bool LedcChannel::fade(uint32_t uiTargetDuty, int iMaxFadeTime_ms, bool bBlocking /* = false*/) {
	bool bOk = true;
	
	if (!isInitialized()) {
		return false;
	}
	
	if (!Esp32LedcRegistry::instance()->isHardwareFadeEnabled()) {
		//MDO, see: C:\temp\esp32\esp-idf-master\components\esp_hw_support\include\esp_intr_alloc.h
		bOk = ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM) == ESP_OK;	//not 100% sure about these. Does seem to work
		if (bOk) {
			Esp32LedcRegistry::instance()->setHardwareFadeEnabled();
		}
	}
	
	if (bOk) {
		bOk = ledc_set_fade_time_and_start(getSpeedMode(), (ledc_channel_t)getChannelNr(), uiTargetDuty, iMaxFadeTime_ms, bBlocking ? LEDC_FADE_WAIT_DONE:LEDC_FADE_NO_WAIT) == ESP_OK;
	}
	
	return bOk;
}


/**
 * LEDC stop. Disable LEDC output, and set idle level.
 * Note that this does nothing with the timer (as far as I know)
 */
bool LedcChannel::disableOutput(uint32_t uiIdleLevel /* = 0 */) {
	if (!isInitialized()) {
		return false;
	}
	//not sure what uiIdleLevel is / can be..
	return ledc_stop(getSpeedMode(), (ledc_channel_t)getChannelNr(), uiIdleLevel) == ESP_OK;
}

/**
 * Gets the current Duty from the hardware.
 * The hardware updates this value per PWM cycle (after a call to updateDuty), it might therefore be the 'old value' for some time.
 * Might return 0xFFFFFFFF on error
 */
uint32_t LedcChannel::getHardwareDuty() const {
	if (!isInitialized()) {
		return 0xFFFFFFFF;
	}
	
	return ledc_get_duty(getSpeedMode(), (ledc_channel_t)getChannelNr());
}

/**
 * A thread-safe API to set duty for LEDC channel and return when duty updated.
 * uiDuty: Set the LEDC duty, the range of duty setting is [0, (2**duty_resolution)] 
 * uiHighPoint: Set the LEDC hpoint value, the range is [0, (2**duty_resolution)-1]
 */
bool LedcChannel::updateDuty(uint32_t uiDuty, uint32_t uiHighPoint) {
	if ((!isInitialized()) || (!checkDutyAndHighPoint(uiDuty, uiHighPoint))) {
		MDO_SERVO_DEBUG_PRINTLN("LedcChannel::updateDuty - state issue");
		return false;
	}
	bool bOk = false;
	if (!Esp32LedcRegistry::instance()->isHardwareFadeEnabled()) {
		//MDO, see: C:\temp\esp32\esp-idf-master\components\esp_hw_support\include\esp_intr_alloc.h
		bOk = ledc_fade_func_install(ESP_INTR_FLAG_LEVEL1 | ESP_INTR_FLAG_SHARED | ESP_INTR_FLAG_IRAM) == ESP_OK;	//not 100% sure about these. Does seem to work
		if (bOk) {
			Esp32LedcRegistry::instance()->setHardwareFadeEnabled();
		}
	}
	if (bOk) {
		bOk = ledc_set_duty_and_update(getSpeedMode(), (ledc_channel_t)getChannelNr(), uiDuty, uiHighPoint) == ESP_OK;
	}
	
	return bOk;
}

uint8_t LedcChannel::getChannelNr() const {
	return m_uiChannelNr;
}

ledc_mode_t LedcChannel::getSpeedMode() const {
	return m_eSpeedMode;
}

bool LedcChannel::isInitialized() const {
	return m_bInitialized;
}

/**
 * Adds a pin to this channel, if this channel already is configured.
 * Please be aware that in my ESP32-S3 setup with esp32 lib 3.1.2 from Espressif, 
 *	=> if the :begin requested bInvertOutput to be true, the added pin in this method will not be inverted.. ??
 *  => after this channel is deleted, and a new is created afterwards with just one pin, the pin added here will still get the same output 
 *		(so no 'full means' to deconfigure a channel without reassiging the pin to something else)
 */
bool LedcChannel::addPin(int iPinNr) {
	if (!isInitialized()) {
		return false;
	}
	bool bOk = ledc_set_pin(iPinNr, getSpeedMode(), (ledc_channel_t)getChannelNr()) == ESP_OK;
	return bOk;
}

LedcChannel::LedcChannel(uint8_t uiChannelNr, ledc_mode_t eSpeedMode) {
	m_uiChannelNr = uiChannelNr;
	m_bInitialized = false;
	m_pTimer = 0;
	m_eSpeedMode = eSpeedMode;
}

LedcChannel::~LedcChannel() {
	Esp32LedcRegistry::instance()->unregisterChannel(this);
	disableOutput();	//don't check result
}

}	//namespace end
}	//namespace end