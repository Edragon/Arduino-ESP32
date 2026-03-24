#include "Esp32LedcRegistry.h"


//#include "debug_config.h"
#include "DebugMsg.h"
//#include <mdomisc.h>

#include "LedcTimer.h"
#include "LedcChannel.h"
#include "ServoController.h"


namespace MDO {
namespace ESP32ServoController {

Esp32LedcRegistry* Esp32LedcRegistry::m_pSingleton = nullptr;

//private
std::shared_ptr<LedcTimer> Esp32LedcRegistry::findTimerIn(const pwmcontroller_t* pPwmControllers, uint32_t uiFreqHz) const {
	std::shared_ptr<LedcTimer> pRet;
	if (pPwmControllers != nullptr) {
		for (auto cit = pPwmControllers->begin(); cit != pPwmControllers->end(); cit++) {
			std::shared_ptr<LedcTimer> pOption = (*cit)->getTimer();
			if ((pOption != nullptr) && (pOption->getFrequency() == uiFreqHz)) {
				pRet = pOption;
				break;
			}
		}		
	}
	
	return pRet;
}

/**
 * Searches through our registry to see if there already is a timer for the provided frequency and speed mode for a PWM Controller
 * Providing no speed mode means 'search all options' (from a speed mode point of view)
 * return either a timer pointer when a hit is found, or nullptr
 */
std::shared_ptr<LedcTimer> Esp32LedcRegistry::hasTimerForPwm(uint32_t uiFreqHz, ledc_mode_t eSpeedMode /*= LEDC_SPEED_MODE_MAX*/) const {

	std::shared_ptr<LedcTimer> pRet;
	
	const pwmcontroller_t* pPwmControllers = nullptr;	
#if SOC_LEDC_SUPPORT_HS_MODE
	if ((eSpeedMode == LEDC_SPEED_MODE_MAX) || (eSpeedMode == LEDC_HIGH_SPEED_MODE)) {
		pPwmControllers = &m_sPwmControllers.sHighSpeed.vPwmControllers;
		pRet = findTimerIn(pPwmControllers, uiFreqHz);
	}
#endif

	if (pRet != nullptr) {
		return pRet;
	}

	if ((eSpeedMode == LEDC_SPEED_MODE_MAX) || (eSpeedMode == LEDC_LOW_SPEED_MODE)) {
		pPwmControllers = &m_sPwmControllers.sLowSpeed.vPwmControllers;
		pRet = findTimerIn(pPwmControllers, uiFreqHz);
	}
	
	return pRet;
}

const ServoController* Esp32LedcRegistry::getServoUsing(uint32_t uiFreqHz, ledc_mode_t eSpeedMode) const {
	
	//MDO_SERVO_DEBUG_PRINTLN(String("Esp32LedcRegistry::getServoUsing - ") + uiFreqHz + ", " + eSpeedMode);
	
	const servos_t* pMap = nullptr;
	const ServoController* pServoFound = nullptr;
	
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		pMap = &m_sServoControllers.sLowSpeed;
	} else {
		pMap = &m_sServoControllers.sHighSpeed;
	}
	
	for (auto cit = pMap->begin(); cit != pMap->end(); cit++) {
		if ((cit->second != nullptr) && (cit->second->getTimerFreqHz() == uiFreqHz)) {
			pServoFound = cit->second;
			MDO_SERVO_DEBUG_PRINTLN("  found matching servo instance");
			return pServoFound;
		}
	}
	MDO_SERVO_DEBUG_PRINTLN("  no matching servo found");
	return pServoFound;
}

bool Esp32LedcRegistry::registerTimer(const LedcTimer* pTimer) {
	if ((pTimer == nullptr) || (isTimerInUse(pTimer))) {
		return false;
	}
	if (pTimer->getSpeedMode() == LEDC_LOW_SPEED_MODE) {
		//MDO_SERVO_DEBUG_PRINTLN("Registering low speed timer");
		m_sPwmControllers.sLowSpeed.mTimers[pTimer->getTimerNr()] = pTimer;
	} else {
		//MDO_SERVO_DEBUG_PRINTLN("Registering high speed timer");
		m_sPwmControllers.sHighSpeed.mTimers[pTimer->getTimerNr()] = pTimer;
	}
	return true;
}

/**
 * unregisters a timer.
 */
void Esp32LedcRegistry::unregisterTimer(const LedcTimer* pTimer) {
	if (pTimer == nullptr) {
		return;
	}
	
	ledc_mode_t eSpeedMode = pTimer->getSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		return;
	}
	
	timers_t* pTimers = nullptr;	
	if (pTimer->getSpeedMode() == LEDC_LOW_SPEED_MODE) {
		pTimers = &m_sPwmControllers.sLowSpeed.mTimers;
	} else {
		pTimers = &m_sPwmControllers.sHighSpeed.mTimers;
	}
	
	auto it = pTimers->find(pTimer->getTimerNr());
	if (it != pTimers->end()) {
		it->second = nullptr;	//ensure the pointer does not get deleted
		pTimers->erase(it);
	}
}

bool Esp32LedcRegistry::registerChannel(const LedcChannel* pChannel) {
	if ((pChannel == nullptr) || (isChannelInUse(pChannel))) {
		return false;
	}
	if (pChannel->getSpeedMode() == LEDC_LOW_SPEED_MODE) {
		m_sPwmControllers.sLowSpeed.mChannels[pChannel->getChannelNr()] = pChannel;
	} else {
		m_sPwmControllers.sHighSpeed.mChannels[pChannel->getChannelNr()] = pChannel;
	}
	return true;
}

/**
 * unregisters a channel.
 */
void Esp32LedcRegistry::unregisterChannel(const LedcChannel* pChannel) {
	if (pChannel == nullptr) {
		return;
	}
	
	ledc_mode_t eSpeedMode = pChannel->getSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		return;
	}
	
	channels_t* pChannels = nullptr;
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		pChannels = &m_sPwmControllers.sLowSpeed.mChannels;
	} else {
		pChannels = &m_sPwmControllers.sHighSpeed.mChannels;
	}
	
	auto it = pChannels->find(pChannel->getChannelNr());
	if (it !=pChannels->end()) {
		it->second = nullptr;	//ensure the pointer does not get deleted
		pChannels->erase(it);
	}	
}

bool Esp32LedcRegistry::registerPwmController(const PWMController* pPwmController) {
	if (pPwmController == nullptr) {
		return false;
	}
	ledc_mode_t eSpeedMode = pPwmController->getSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		return false;
	}
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		m_sPwmControllers.sLowSpeed.vPwmControllers.push_back(pPwmController);
	} else {
		m_sPwmControllers.sHighSpeed.vPwmControllers.push_back(pPwmController);
	}
	return true;
}

void Esp32LedcRegistry::unregisterPwmController(const PWMController* pPwmController) {
	if (pPwmController == nullptr) {
		return;
	}
	ledc_mode_t eSpeedMode = pPwmController->getSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		return;
	}
	
	pwmcontroller_t* pPwmControllers = nullptr;	
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		pPwmControllers = &m_sPwmControllers.sLowSpeed.vPwmControllers;
	} else {
		pPwmControllers = &m_sPwmControllers.sHighSpeed.vPwmControllers;
	}
	
	for (auto it = pPwmControllers->begin(); it != pPwmControllers->end(); it++) {
		if (pPwmController == *it) {	//if the same pointer
			pPwmControllers->erase(it);
			return;						//we're done
		}
	}
}

bool Esp32LedcRegistry::registerServo(const ServoController* pServo) {
	if ((pServo == nullptr) || (pServo->getId() == 0xFF) || (isServoInUse(pServo))) {
		MDO_SERVO_DEBUG_PRINTLN("Ignore registerServo");
		return false;
	}
	ledc_mode_t eSpeedMode = pServo->getSpeedMode();
	if (eSpeedMode == LEDC_SPEED_MODE_MAX) {
		return false;
	}
	
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		m_sServoControllers.sLowSpeed[pServo->getId()] = pServo;
	} else {
		m_sServoControllers.sHighSpeed[pServo->getId()] = pServo;
	}
	MDO_SERVO_DEBUG_PRINTLN("Servo registered");
	return true;
}

void Esp32LedcRegistry::unregisterServo(const ServoController* pServo) {
	if ((pServo == nullptr) || (pServo->getId() == 0xFF)) {
		//this does not always indicate an error, so just ignore
		return;
	}
	
	servos_t* pServos = nullptr;	
	if (pServo->getSpeedMode() == LEDC_LOW_SPEED_MODE) {
		pServos = &m_sServoControllers.sLowSpeed;
	} else {
		pServos = &m_sServoControllers.sHighSpeed;
	}

	auto it = pServos->find(pServo->getId());
	if (it != pServos->end()) {
		it->second = nullptr;	//ensure the pointer does not get deleted
		pServos->erase(it);
	}
	
}

void Esp32LedcRegistry::setHardwareFadeEnabled() {
	m_bHardwareFadeEnabled = true;
}

/**
 * When a timer is already in use, and the hardware only supports one clock source: the new clock source is already known ;-)
 * in this scenario, will set uiClockSourceOutput to the relevant value
 */
/*	not implemented yet.. Maybe later. Since ledc error messages are quite clear, this might not be needed.
bool Esp32LedcRegistry::isClockSourceFixed(uint8_t& uiClockSourceOutput) const {
	bool bIsClockSourceFixed =	(getNrOfSimultaneousClockSources() == 1) && 
								(getCurrentNrOfTimersInUse() > 0);
	if (bIsClockSourceFixed) {
		//set uiClockSourceOutput
	}
	
	return bIsClockSourceFixed;
}*/


/**
 * Fore debug purposes, provides an overview of the channel usage
 */
String Esp32LedcRegistry::channelUsageToString() const {
	String strRet;
	bool bHasHighSpeedCapabilities = false;
#if SOC_LEDC_SUPPORT_HS_MODE
	if (getNrOfHighSpeedTimers() != 0) {
		bHasHighSpeedCapabilities = true;
		strRet += String("High speed channel usage: ") + getCurrentNrOfChannelsInUse(LEDC_HIGH_SPEED_MODE) + " / " + getNrOfHighSpeedChannels() + ", ";			
	}
#endif

	if (bHasHighSpeedCapabilities) {
		strRet += String("Low speed channel usage: ") + getCurrentNrOfChannelsInUse(LEDC_LOW_SPEED_MODE) + " / " + getNrOfLowSpeedChannels();
	} else {
		strRet += String("Channel usage: ") + getCurrentNrOfChannelsInUse(LEDC_LOW_SPEED_MODE) + " / " + getNrOfLowSpeedChannels();
	}

	return strRet;
}

/**
 * Fore debug purposes, provides an overview of the timer usage
 */
String Esp32LedcRegistry::timerUsageToString() const {
	String strRet;
	bool bHasHighSpeedCapabilities = false;
#if SOC_LEDC_SUPPORT_HS_MODE
	if (getNrOfHighSpeedTimers() != 0) {
		bHasHighSpeedCapabilities = true;
		strRet += String("High speed timer usage: ") + getCurrentNrOfTimersInUse(LEDC_HIGH_SPEED_MODE) + " / " + getNrOfHighSpeedTimers() + ", ";			
	}
#endif
	if (bHasHighSpeedCapabilities) {
		strRet += String("Low speed timer usage: ") + getCurrentNrOfTimersInUse(LEDC_LOW_SPEED_MODE) + " / " + getNrOfLowSpeedTimers();
	} else {
		strRet += String("Timer usage: ") + getCurrentNrOfTimersInUse(LEDC_LOW_SPEED_MODE) + " / " + getNrOfLowSpeedTimers();
	}
	
	return strRet;
}
		
uint8_t Esp32LedcRegistry::getCurrentNrOfChannelsInUse(ledc_mode_t eSpeedMode /*= LEDC_SPEED_MODE_MAX*/) const {
	uint8_t uiCount = 0;
	if (eSpeedMode != LEDC_LOW_SPEED_MODE) {
		uiCount += m_sPwmControllers.sHighSpeed.mChannels.size();
	}
	if ((eSpeedMode == LEDC_LOW_SPEED_MODE) || (eSpeedMode == LEDC_SPEED_MODE_MAX)) {
		uiCount += m_sPwmControllers.sLowSpeed.mChannels.size();
	}
	return uiCount;
}

//gives an answer globally (when eSpeedMode is set to LEDC_SPEED_MODE_MAX), or locally LEDC_LOW_SPEED_MODE / LEDC_HIGH_SPEED_MODE when requested
uint8_t Esp32LedcRegistry::getCurrentNrOfTimersInUse(ledc_mode_t eSpeedMode /*= LEDC_SPEED_MODE_MAX*/) const {
	uint8_t uiCount = 0;
	if (eSpeedMode != LEDC_LOW_SPEED_MODE) {
		uiCount += m_sPwmControllers.sHighSpeed.mTimers.size();
	}
	if ((eSpeedMode == LEDC_LOW_SPEED_MODE) || (eSpeedMode == LEDC_SPEED_MODE_MAX)) {
		uiCount += m_sPwmControllers.sLowSpeed.mTimers.size();
	}
	return uiCount;
}

/**
 * Get first available timer for the provided speed mode.
 * Speed mode cannot be LEDC_SPEED_MODE_MAX
 * Returns either the timer number available or LEDC_TIMER_MAX when nothing is available anymore
 */
uint8_t Esp32LedcRegistry::getFirstAvailableTimer(ledc_mode_t eSpeedMode) const {
	uint8_t iFirstAvailableTimerNr = LEDC_TIMER_MAX;
	
#if SOC_LEDC_SUPPORT_HS_MODE
	//so now we can support high speed and low speed
	//'max' in not that, so that's wrong
	if (eSpeedMode != LEDC_SPEED_MODE_MAX) {	
#else
	//now we only support low speed
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
#endif

		const uint8_t uiMaxNrOfTimers = (eSpeedMode == LEDC_LOW_SPEED_MODE) ? getNrOfLowSpeedTimers() : getNrOfHighSpeedTimers();
		for (uint8_t i=0; i< uiMaxNrOfTimers; i++) {
			if (!isTimerInUse(i, eSpeedMode)) {
				iFirstAvailableTimerNr = i;
				return iFirstAvailableTimerNr;
			}			
		}
	}

	return iFirstAvailableTimerNr;
}

uint8_t Esp32LedcRegistry::getFirstAvailableChannel(ledc_mode_t eSpeedMode) const {
	uint8_t iFirstAvailableChannelNr = LEDC_CHANNEL_MAX;
	
#if SOC_LEDC_SUPPORT_HS_MODE
	//so now we can support high speed and low speed
	//'max' in not that, so that's wrong
	if (eSpeedMode != LEDC_SPEED_MODE_MAX) {	
#else
	//now we only support low speed
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
#endif

		const uint8_t uiMaxNrOfChannels = getNrOfChannels();	//no distinction between low speed and high speed here
		for (uint8_t i=0; i< uiMaxNrOfChannels; i++) {
			if (!isChannelInUse(i, eSpeedMode)) {
				iFirstAvailableChannelNr = i;
				return iFirstAvailableChannelNr;
			}			
		}
	}

	return iFirstAvailableChannelNr;
}

bool Esp32LedcRegistry::isTimerInUse(uint8_t uiTimerNr, ledc_mode_t eSpeedMode) const {
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		return m_sPwmControllers.sLowSpeed.mTimers.find(uiTimerNr)  != m_sPwmControllers.sLowSpeed.mTimers.end();
	} else {
		return m_sPwmControllers.sHighSpeed.mTimers.find(uiTimerNr) != m_sPwmControllers.sHighSpeed.mTimers.end();
	}
}

bool Esp32LedcRegistry::isTimerInUse(const LedcTimer* pTimer) const {
	if (pTimer == 0) {
		MDO_SERVO_DEBUG_PRINTLN("Esp32LedcRegistry::isTimerInUse - parameter error");
		return true;
	}
	return isTimerInUse(pTimer->getTimerNr(), pTimer->getSpeedMode());
}

bool Esp32LedcRegistry::isChannelInUse(uint8_t uiChannelNr, ledc_mode_t eSpeedMode) const {
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		return m_sPwmControllers.sLowSpeed.mChannels.find(uiChannelNr)  != m_sPwmControllers.sLowSpeed.mChannels.end();
	} else {
		return m_sPwmControllers.sHighSpeed.mChannels.find(uiChannelNr) != m_sPwmControllers.sHighSpeed.mChannels.end();
	}
}

bool Esp32LedcRegistry::isChannelInUse(const LedcChannel* pChannel) const {
	if (pChannel == nullptr) {
		MDO_SERVO_DEBUG_PRINTLN("Esp32LedcRegistry::isChannelInUse - parameter error");
		return true;
	}
	return isChannelInUse(pChannel->getChannelNr(), pChannel->getSpeedMode());
}

bool Esp32LedcRegistry::isServoInUse(uint8_t uiId, ledc_mode_t eSpeedMode) const {
	if (eSpeedMode == LEDC_LOW_SPEED_MODE) {
		return m_sServoControllers.sLowSpeed.find(uiId)  != m_sServoControllers.sLowSpeed.end();
	} else {
		return m_sServoControllers.sHighSpeed.find(uiId) != m_sServoControllers.sHighSpeed.end();
	}	
}

bool Esp32LedcRegistry::isServoInUse(const ServoController* pServo) const {
	if (pServo == nullptr) {
		MDO_SERVO_DEBUG_PRINTLN("Esp32LedcRegistry::isServoInUse - parameter error");
		return true;
	}
	return isServoInUse(pServo->getId(), pServo->getSpeedMode());
}

bool Esp32LedcRegistry::isHardwareFadeEnabled() const {
	return m_bHardwareFadeEnabled;
}

uint32_t Esp32LedcRegistry::getServoFrequency() const {
	return m_uiServoFreqHz;
}

//in usec
uint32_t Esp32LedcRegistry::getServoMinPosTime() const {
	return m_uiServoMinPos_usec;
}

//in usec
uint32_t Esp32LedcRegistry::getServoMaxPosTime() const {
	return m_uiServoMaxPos_usec;
}

uint8_t Esp32LedcRegistry::getNrOfHighSpeedTimers() const {
	return m_uiNrOfHighSpeedTimers;
}

uint8_t Esp32LedcRegistry::getNrOfLowSpeedTimers() const {
	return m_uiNrOfLowSpeedTimers;
}

uint8_t Esp32LedcRegistry::getNrOfSimultaneousClockSources() const {
	return m_uiNrOfSimultaneousClockSources;
}

uint8_t Esp32LedcRegistry::getNrOfHighSpeedChannels() const {
	return (getNrOfHighSpeedTimers() == 0) ? (0) : (getNrOfChannels());
}

uint8_t Esp32LedcRegistry::getNrOfLowSpeedChannels() const {
	return getNrOfChannels();
}

uint8_t Esp32LedcRegistry::getNrOfChannels() const {
	return m_uiNrOfChannels;
}

uint8_t Esp32LedcRegistry::getMaxTimerResolutionBits() const {
	return m_uiMaxTimerResolutionBits;
}

ledc_sleep_mode_t Esp32LedcRegistry::getSleepPowerMode() const {
	return m_eSleepPowerMode;
}

void Esp32LedcRegistry::setSleepPowerMode(ledc_sleep_mode_t eSleepPowerMode) {
	m_eSleepPowerMode = eSleepPowerMode;
}

bool Esp32LedcRegistry::setServoParams(uint32_t uiMinPos_usec /*= 1000*/, uint32_t uiMaxPos_usec /*= 2000*/, uint32_t uiFreqHz /*= 50*/, bool bLimitCheck /*= true*/) {
	bool bOk = true;
	
	if ((bLimitCheck) && (uiFreqHz >= 40) && (uiFreqHz <= 200)) {	//values from wikipedia
		m_uiServoFreqHz = uiFreqHz;
	} else {
		bOk = false;
	}
	if ((bLimitCheck) && (m_uiServoMinPos_usec < m_uiServoMaxPos_usec)) {
		//could also limit these based on m_uiServoFreqHz. maybe later
		m_uiServoMinPos_usec	= uiMinPos_usec;
		m_uiServoMaxPos_usec	= uiMaxPos_usec;
	} else {
		bOk = false;
	}
	
	if (!bOk) {
		MDO_SERVO_DEBUG_PRINTLN("WARNING - Esp32LedcRegistry::setServoParams - ignoring parameters based on failed checks");
	}
	return bOk;
}

void Esp32LedcRegistry::begin(uint8_t uiNrOfHighSpeedTimers, uint8_t uiNrOfLowSpeedTimers, uint8_t uiNrOfSimultaneousClockSources, uint8_t uiNrOfChannels, uint8_t uiMaxTimerResolutionBits) {
	m_uiNrOfHighSpeedTimers			= uiNrOfHighSpeedTimers;
	m_uiNrOfLowSpeedTimers			= uiNrOfLowSpeedTimers;
	m_uiNrOfSimultaneousClockSources= uiNrOfSimultaneousClockSources;
	m_uiNrOfChannels				= uiNrOfChannels;
	m_uiMaxTimerResolutionBits		= uiMaxTimerResolutionBits;
}

/*static*/ Esp32LedcRegistry* Esp32LedcRegistry::instance() {
	if (m_pSingleton == nullptr) {
		m_pSingleton = new Esp32LedcRegistry();
	}
	
	return m_pSingleton;
}

Esp32LedcRegistry::Esp32LedcRegistry() {
	m_bHardwareFadeEnabled = false;
	
	m_uiNrOfHighSpeedTimers			= 0;
	m_uiNrOfLowSpeedTimers			= 0;
	m_uiNrOfSimultaneousClockSources= 0;
	m_uiNrOfChannels				= 0;
	m_uiMaxTimerResolutionBits		= 0;
	
	m_uiServoFreqHz = 50;
	m_uiServoMinPos_usec = 1000;
	m_uiServoMaxPos_usec = 2000;
	
	m_eSleepPowerMode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD;	//The default mode: no LEDC output, and no power off the LEDC power domain.
}

Esp32LedcRegistry::~Esp32LedcRegistry() {
}

}	//namespace end
}	//namespace end