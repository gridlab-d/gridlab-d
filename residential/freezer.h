/** $Id: freezer.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file freezer.h
	@addtogroup freezer
	@ingroup residential

 @{
 **/

#ifndef _FREEZER_H
#define _FREEZER_H

#include "residential.h"
#include "residential_enduse.h"
#include "house_e.h"

class freezer : public residential_enduse
{
public:
	typedef enum {
		S_OFF=0,
		S_ON=1
	} MOTORSTATE;

private:
	house_e *pHouse;			// reference to the parent house
	PROPERTY *pTempProp;

public:
	double size;  ///< refrigerator volume (cf) 
	double rated_capacity;  ///< rated capacity (Btu/h)
	double thermostat_deadband;  ///< refrigerator thermostat hysterisys (degF)
	double UA;
	double UAr;		///< UA of Refrigerator compartment
	double UAf;		///< UA of the food-air
	double Tair;	///< Refirgerator air temperature (degF)
	double Tout;	///< House air temperature
	double Tset;	///< Refrigerator control set point temperature (degF)
	double Cf;		///< heat capapcity of the food
	double Qr;		///< heat rate from the cooling system
	double COPcoef;	///< compressor COP
	
	double Tevent;	///< Temperature we will switch the motor on or off.  Available for SmartGrid PLC code to nudge.

	double power_factor;

	enumeration motor_state;
	TIMESTAMP last_time, next_time;

public:

	static CLASS *oclass, *pclass;
	static freezer *defaults;
	freezer(MODULE *module);
	~freezer();
	
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
};

#endif // _FREEZER_H

/**@}**/
