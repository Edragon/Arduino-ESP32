#ifndef _MDO_Esp32LedcRegistry_H
#define _MDO_Esp32LedcRegistry_H

#include <Arduino.h>
#include <memory>
#include <vector>
#include <map>

#include <driver/ledc.h>

namespace MDO {
namespace ESP32ServoController {
	
class LedcChannel;
class LedcTimer;
class PWMController;
class ServoController;

//config source: https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32/api-reference/peripherals/ledc.html
//uiNrOfHighSpeedTimers: search for 'only supports configuring channels in "low speed" mode.'
//uiNrOfLowSpeedTimers: search for 'enum ledc_timer_t'
//uiNrOfClockSources = number of clock sources which might be active at the *same time*. Search for 'all timers share one clock source' or 'All chips except esp32 and esp32s2 do not have timer-specific clock sources, which means clock source for all timers must be the same one'
//uiNrOfChannels: search for 'enum ledc_channel_t', note that when we support high speed timers, this number is used for both low speed channels and high speed channels
//uiMaxTimerResolutionBits: search for 'enum ledc_timer_bit_t'
//
//						uiNrOfHighSpeedTimers, uiNrOfLowSpeedTimers, uiNrOfSimultaneousClockSources, uiNrOfChannels, uiMaxTimerResolutionBits
#define LEDC_CONFIG_ESP32			4, 						4, 					2, 								8, 				20
#define LEDC_CONFIG_ESP32_C2		0,						4,					1,								6,				14
#define LEDC_CONFIG_ESP32_C3		0, 						4, 					1, 								6, 				14
#define LEDC_CONFIG_ESP32_C5		0, 						4, 					1, 								6, 				20
#define LEDC_CONFIG_ESP32_C6		0, 						4, 					1, 								6, 				20
#define LEDC_CONFIG_ESP32_H2		0,						4,					1,								6,				20
#define LEDC_CONFIG_ESP32_P4		0,						4,					1,								8,				20
#define LEDC_CONFIG_ESP32_S2		0, 						4, 					2, 								8, 				14
#define LEDC_CONFIG_ESP32_S3		0, 						4, 					1, 								8, 				14

/**
 * This is the main registry for hardware capabilities and registration on usage.
 * In addition, this class also regsiters some settings (see setServoParams()).
 * This is a singleton, see instance()
 */ 
class Esp32LedcRegistry {
	friend class LedcTimer;
	friend class LedcChannel;
	friend class PWMController;
	friend class PwmFactoryDecorator;
	friend class ServoController;
	friend class ServoFactoryDecorator;
	
	public:		//types
		typedef std::map<uint8_t, const LedcTimer*>			timers_t;
		typedef std::map<uint8_t, const LedcChannel*>		channels_t;
		
		typedef std::vector<const PWMController*>			pwmcontroller_t;
		typedef std::map<uint8_t, const ServoController*>	servos_t;			//servo's are registers based on the channel number
		
		typedef struct PwmControllerContainer_t {
			timers_t		mTimers;
			channels_t		mChannels;
			pwmcontroller_t	vPwmControllers;
		} PwmControllerContainer_t;

		typedef struct PwmControllers_t {
			PwmControllerContainer_t	sHighSpeed;
			PwmControllerContainer_t	sLowSpeed;
		} PwmControllers_t;

		typedef struct Servos_t {
			servos_t	sHighSpeed;
			servos_t	sLowSpeed;
		} Servos_t;
	
	private:
		static Esp32LedcRegistry*	m_pSingleton;
		bool						m_bHardwareFadeEnabled;
		uint8_t						m_uiNrOfHighSpeedTimers;
		uint8_t						m_uiNrOfLowSpeedTimers;
		uint8_t						m_uiNrOfSimultaneousClockSources;
		uint8_t						m_uiNrOfChannels;
		uint8_t						m_uiMaxTimerResolutionBits;
		
		PwmControllers_t			m_sPwmControllers;
		Servos_t					m_sServoControllers;
		
		uint32_t					m_uiServoFreqHz;
		uint32_t					m_uiServoMinPos_usec; 
		uint32_t					m_uiServoMaxPos_usec;
		
		ledc_sleep_mode_t			m_eSleepPowerMode;
	
	private:
		std::shared_ptr<LedcTimer>	findTimerIn(const pwmcontroller_t* pPwmControllers, uint32_t uiFreqHz) const;
	
	protected:
		std::shared_ptr<LedcTimer>	hasTimerForPwm(uint32_t uiFreqHz, ledc_mode_t eSpeedMode = LEDC_SPEED_MODE_MAX) const;	//do we already have a timer registered for this frequency?
		const ServoController*		getServoUsing(uint32_t uiFreqHz, ledc_mode_t eSpeedMode) const;

		bool						registerTimer(const LedcTimer* pTimer);
		void						unregisterTimer(const LedcTimer* pTimer);

		bool						registerChannel(const LedcChannel* pChannel);
		void						unregisterChannel(const LedcChannel* pChannel);
		
		bool						registerPwmController(const PWMController* pPwmController);
		void						unregisterPwmController(const PWMController* pPwmController);	
		
		bool						registerServo(const ServoController* pServo);
		void						unregisterServo(const ServoController* pServo);
	
		void						setHardwareFadeEnabled();

	public:	
		//bool						isClockSourceFixed(uint8_t& uiClockSourceOutput) const;
		String						channelUsageToString() const;
		String						timerUsageToString() const;
		uint8_t						getCurrentNrOfChannelsInUse(ledc_mode_t eSpeedMode = LEDC_SPEED_MODE_MAX) const;
		uint8_t						getCurrentNrOfTimersInUse(ledc_mode_t eSpeedMode = LEDC_SPEED_MODE_MAX) const;
		uint8_t 					getFirstAvailableTimer(ledc_mode_t eSpeedMode) const;
		uint8_t						getFirstAvailableChannel(ledc_mode_t eSpeedMode) const;		
		bool						isTimerInUse(uint8_t uiTimerNr, ledc_mode_t eSpeedMode) const;
		bool						isTimerInUse(const LedcTimer* pTimer) const;
		bool						isChannelInUse(uint8_t uiChannelNr, ledc_mode_t eSpeedMode) const;
		bool						isChannelInUse(const LedcChannel* pChannel) const;	
		bool						isServoInUse(uint8_t uiId, ledc_mode_t eSpeedMode) const;
		bool						isServoInUse(const ServoController* pServo) const;		
		
		bool						isHardwareFadeEnabled() const;
		
		uint32_t					getServoFrequency() const;
		uint32_t					getServoMinPosTime() const;	//in usec
		uint32_t					getServoMaxPosTime() const;	//in usec
		
		uint8_t 					getNrOfHighSpeedTimers() const;
		uint8_t						getNrOfLowSpeedTimers() const;
		uint8_t						getNrOfSimultaneousClockSources() const;
		uint8_t 					getNrOfHighSpeedChannels() const;
		uint8_t						getNrOfLowSpeedChannels() const;
		uint8_t						getNrOfChannels() const;
		uint8_t						getMaxTimerResolutionBits() const;	
	
		ledc_sleep_mode_t			getSleepPowerMode() const;
		void						setSleepPowerMode(ledc_sleep_mode_t eSleepPowerMode);
		
		bool						setServoParams(uint32_t uiMinPos_usec = 1000, uint32_t uiMaxPos_usec = 2000, uint32_t uiFreqHz = 50, bool bLimitCheck = true);
		void						begin(uint8_t uiNrOfHighSpeedTimers, uint8_t uiNrOfLowSpeedTimers, uint8_t uiNrOfSimultaneousClockSources, uint8_t uiNrOfChannels, uint8_t uiMaxTimerResolutionBits);
		static Esp32LedcRegistry*	instance();

	private:
		Esp32LedcRegistry();
		Esp32LedcRegistry(const Esp32LedcRegistry& oSource);	//not implemented
	public:
		virtual ~Esp32LedcRegistry();
};

}	//namespace end
}	//namespace end

#endif