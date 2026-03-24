#include "PWMController.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>
#include "Esp32LedcRegistry.h"
#include "LedcChannelHighSpeed.h"
#include "LedcChannelLowSpeed.h"
#include "LedcTimerHighSpeed.h"
#include "LedcTimerLowSpeed.h"


namespace MDO {
namespace ESP32ServoController {

/**
 * Protected: Convert a duty (double, percentage from 0 to 1) to the relevant integer values for a channel, based on our timer
 */
bool PWMController::dutyToInt(double dDuty, uint32_t& uiDuty, int& iHighPoint) const {
	if (m_spTimer == nullptr) {
		return false;
	}
	if ((dDuty < 0.0) || (dDuty > 1.0)) {
		MDO_SERVO_DEBUG_PRINTLN(String("Invalid duty cycle parameter: ") + dDuty);
		return false;
	}
	
	uint32_t uiMaxResolutionValue = m_spTimer->getMaxResolutionValue();
	uiDuty = (uint32_t)std::round(uiMaxResolutionValue * dDuty);
	iHighPoint = uiMaxResolutionValue-1;	//the '-1' is a hardware limit which is checked in LedcChannel as well
	
	//and now for some final annoying rounding cases
	if (dDuty == 0.0) {
		//MDO_SERVO_DEBUG_PRINTLN("PWMController::dutyToInt - fixing 0% rounding");
		uiDuty = 0;	
	} else if (dDuty == 1.0) {
		uiDuty = uiMaxResolutionValue;	
	}	
	//MDO_SERVO_DEBUG_PRINTLN(String("dutyToInt: ") + uiMaxResolutionValue + ", " + uiDuty + ", " + iHighPoint);
	return true;
}

//protected
void PWMController::setTimer(std::shared_ptr<LedcTimer> oTimer) {
	m_spTimer = oTimer;
}

//protected
void PWMController::cleanUp() {
	if (m_spChannel != nullptr) {	//it's advised to start cleaning with the channel
		m_spChannel.reset();
	}
	
	if (m_spTimer != nullptr) {
		m_spTimer.reset();
	}	
}

ledc_mode_t PWMController::getSpeedMode() const {
	const LedcTimer* pTimer(m_spTimer.get());
	if (pTimer != nullptr) {
		return pTimer->getSpeedMode();
	}
	MDO_SERVO_DEBUG_PRINTLN("PWMController::getSpeedMode - initialisation issue (?)");
	return LEDC_SPEED_MODE_MAX;
}

std::shared_ptr<LedcTimer> PWMController::getTimer() const {
	return m_spTimer;
} 

std::shared_ptr<LedcChannel> PWMController::getChannel() const {
	return m_spChannel;
}

/**
 * Fades to a duty cycle 'percentage' (from 0 to 1) in the required time
 * note that the time accuracy can be quite low, depending on the frequency and such. See: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/ledc.html#_CPPv423ledc_set_fade_with_time11ledc_mode_t14ledc_channel_t8uint32_ti
 * bBlocking can be used to make this a blocking method, or not. The controller will not allow a new fade command within the provided iMaxFadeTime_ms.
 */
bool PWMController::fade(double dDuty, int iMaxFadeTime_ms /*=1*/, bool bBlocking /* = false*/) {
	
	if (m_spChannel == nullptr) {
		return false;
	}
	uint32_t uiDuty = 0;
	int iHighPoint = 0;
	dutyToInt(dDuty, uiDuty, iHighPoint);
	return m_spChannel->fade(uiDuty, iMaxFadeTime_ms, bBlocking);
}

/**
 * Just adds a pin to an existing channel
 * Keeping this PWMController instance practically empty
 *
 * Please be aware that in my ESP32-S3 setup with esp32 lib 3.1.2 from Espressif, 
 *	=> if the :begin requested bInvertOutput to be true, the added pin in this method will not be inverted.. ??
 *  => after this channel is deleted, and a new is created afterwards with just one pin, the pin added here will still get the same output 
 *		(so no 'full means' to deconfigure a channel without reassiging the pin to something else)
 */
bool PWMController::begin(int iPinNr, const PWMController* pChannelProvider) {
	m_spTimer = pChannelProvider->getTimer();
	m_spChannel = pChannelProvider->getChannel();
	
	bool bOk = (m_spTimer != nullptr) && (m_spChannel != nullptr);
	if (bOk) {
		bOk = m_spChannel->addPin(iPinNr);
	}
	//I don't think that in this scenario we should register this PWMController to the Esp32LedcRegistry..
	
	return bOk;
}	

/**
 * Initializes a PWMController.
 * Reuses the timer from the pTimerProvider and creates a new channel
 */
bool PWMController::begin(const Esp32LedcFactory& oFactory, int iPinNr, const PWMController* pTimerProvider, double dDuty, bool bInvertOutput /*= false*/) {
	cleanUp();	//just in case

	if ((pTimerProvider == nullptr) || 
		(pTimerProvider->getTimer() == nullptr)) {
		MDO_SERVO_DEBUG_PRINTLN("Invalid timer provider");
		return false;
	}
	if ((dDuty < 0.0) || (dDuty > 1.0)) {
		MDO_SERVO_DEBUG_PRINTLN("Invalid duty cycle parameter");
		return false;
	}
	
	setTimer(pTimerProvider->getTimer());
	if (m_spTimer != nullptr) {
		uint32_t uiDuty = 0;
		int iHighPoint = 0;
		dutyToInt(dDuty, uiDuty, iHighPoint);
		
		m_spChannel = oFactory.createChannel(iPinNr, m_spTimer.get(), uiDuty, iHighPoint, bInvertOutput);		
	}

	bool bOk = (m_spTimer != nullptr) && (m_spChannel != nullptr);

	if (bOk) {
		Esp32LedcRegistry::instance()->registerPwmController(this);
	} else {
		cleanUp();
		MDO_SERVO_DEBUG_PRINTLN("PWMController::begin - failed to allocate/configure a timer and/or channel");
	}

	return bOk;
}

/**
 * Initializes a PWMController.
 * Create a new timer and creates a new channel
 */
bool PWMController::begin(const Esp32LedcFactory& oFactory, int iPinNr, uint32_t uiFreqHz, double dDuty, bool bInvertOutput /*= false*/) {
	//MDO_SERVO_DEBUG_PRINTLN("PWMController::begin");
	
	cleanUp();	//just in case

	if ((dDuty < 0.0) || (dDuty > 1.0)) {
		MDO_SERVO_DEBUG_PRINTLN("Invalid duty cycle parameter");
		return false;
	}

	setTimer(oFactory.createTimer(uiFreqHz, 0));
	if (m_spTimer != nullptr) {
		uint32_t uiDuty = 0;
		int iHighPoint = 0;
		dutyToInt(dDuty, uiDuty, iHighPoint);
		
		m_spChannel = oFactory.createChannel(iPinNr, m_spTimer.get(), uiDuty, iHighPoint, bInvertOutput);
	}
	
	bool bOk = (m_spTimer != nullptr) && (m_spChannel != nullptr);

	if (bOk) {
		Esp32LedcRegistry::instance()->registerPwmController(this);
	} else {
		cleanUp();
		MDO_SERVO_DEBUG_PRINTLN("PWMController::begin - failed to allocate/configure a timer and/or channel");
	}

	return bOk;
}

/**
 * Initializes a PWMController.
 * Tries to re-use a timer, if this is not possible will create a new timer. And creates a new channel
 */
bool PWMController::begin(const PwmFactoryDecorator& oFactory, int iPinNr, uint32_t uiFreqHz, double dDuty, bool bInvertOutput /*= false*/) {
	//MDO_SERVO_DEBUG_PRINTLN("PWMController::begin");
	
	cleanUp();	//just in case

	if ((dDuty < 0.0) || (dDuty > 1.0)) {
		MDO_SERVO_DEBUG_PRINTLN("Invalid duty cycle parameter");
		return false;
	}

	setTimer(oFactory.createTimer(uiFreqHz, 0));
	if (m_spTimer != nullptr) {
		uint32_t uiDuty = 0;
		int iHighPoint = 0;
		dutyToInt(dDuty, uiDuty, iHighPoint);
		
		m_spChannel = oFactory.createChannel(iPinNr, m_spTimer.get(), uiDuty, iHighPoint, bInvertOutput);
	}
	
	bool bOk = (m_spTimer != nullptr) && (m_spChannel != nullptr);

	if (bOk) {
		Esp32LedcRegistry::instance()->registerPwmController(this);
	} else {
		cleanUp();
		MDO_SERVO_DEBUG_PRINTLN("PWMController::begin - failed to allocate/configure a timer and/or channel");
	}

	return bOk;
}

PWMController::PWMController() {
}

PWMController::~PWMController() {
	//note that here we can also *be* a ServoController
	//in that case, the Registry will just ignore our unregister request
	Esp32LedcRegistry::instance()->unregisterPwmController(this);
	cleanUp();
}

}	//namespace end
}	//namespace end