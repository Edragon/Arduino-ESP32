#include "PwmFactoryDecorator.h"


//#include "debug_config.h"
//#include <mdomisc.h>

#include "Esp32LedcRegistry.h"
#include "LedcTimer.h"

namespace MDO {
namespace ESP32ServoController {

/*virtual*/ std::shared_ptr<LedcTimer> PwmFactoryDecorator::createTimer(uint32_t uiFreqHz, uint8_t uiResolutionBits /*= 0*/) const {
	const Esp32LedcRegistry* pRegistry(Esp32LedcRegistry::instance());
	
	//check if we can share something from someone first
	std::shared_ptr<LedcTimer> pRet = pRegistry->hasTimerForPwm(uiFreqHz, m_oFactory.getDefaultSpeedMode());	//MDO: incompatible types.. Zucht. moet shared pointer zijn
	if (pRet == nullptr) {
		if (m_oFactory.supportAlternativeSpeedMode()) {
			pRet = pRegistry->hasTimerForPwm(uiFreqHz, m_oFactory.getAlternativeSpeedMode());
		}
	}
	
	//if not, we'll try to allocate a new timer
	if (pRet == nullptr) {
		//the factory will decide which timer type to create
		//at this level we do not care
		pRet = m_oFactory.createTimer(uiFreqHz, uiResolutionBits);
	}
	

	return pRet;
}

/*virtual*/ std::shared_ptr<LedcChannel> PwmFactoryDecorator::createChannel(int iPinNr, LedcTimer* pTimer, uint32_t uiDuty, int iHighPoint, bool bInvertOutput /*= false*/) const {
	return m_oFactory.createChannel(iPinNr, pTimer, uiDuty, iHighPoint, bInvertOutput);
}

PwmFactoryDecorator::PwmFactoryDecorator(const Esp32LedcFactory& oFactory)
:m_oFactory(oFactory) {
}

PwmFactoryDecorator::~PwmFactoryDecorator() {
}

}	//namespace end
}	//namespace end