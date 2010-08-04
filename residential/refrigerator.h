/** $Id: refrigerator.h,v 1.10 2008/02/13 01:26:12 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file refrigerator.h
	@addtogroup refrigerator
	@ingroup residential

 @{
 **/

#ifndef _REFRIGERATOR_H
#define _REFRIGERATOR_H

#include "residential.h"
#include "residential_enduse.h"

class refrigerator : public residential_enduse {
private:
	double *pTout;
public:
	typedef enum {
		S_OFF=0,
		S_ON=1
	} MOTORSTATE;
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

	MOTORSTATE motor_state;
	TIMESTAMP last_time, next_time;

public:

	static CLASS *oclass, *pclass;
	refrigerator(MODULE *module);
	~refrigerator();
	
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	void thermostat(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
};

#endif // _REFRIGERATOR_H

/**@}**/
