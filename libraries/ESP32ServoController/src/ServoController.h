#ifndef _MDO_ServoController_H
#define _MDO_ServoController_H

#include "PWMController.h"
#include "ServoFactoryDecorator.h"

namespace MDO {
namespace ESP32ServoController {

/**
 * A servo controlling class, meant for 180 degree servo's
 * Has a (sometimes shared) timer and a channel for this purpose, which are created used the provided factory (decorator)
 */ 
class ServoController: protected PWMController {
	friend class ServoFactoryDecorator;
	
	public:		//types
	private:	
	
	private:
		//bool		privateSetSpeedMode(ledc_mode_t eSpeedMode);
		double		angleToDuty(double dAngle) const;	
	
	public:
		uint32_t	getTimerFreqHz() const;
		ledc_mode_t	getSpeedMode() const;
		uint8_t		getId() const;

		bool	moveTo(double dAngle, int iMaxTime_ms = 100, bool bBlocking = false);
		
		bool	begin(const ServoFactoryDecorator& oFactory, int iPinNr, double dInitialAngle = 90.0);
	
		ServoController();
		virtual ~ServoController();
};

}	//namespace end
}	//namespace end

#endif