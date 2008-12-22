/** $Id: freezer.h,v 1.10 2008/02/13 01:26:12 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file freezer.h
	@addtogroup freezer
	@ingroup residential

 @{
 **/

#ifndef _FREEZER_H
#define _FREEZER_H

#include "residential.h"

class freezer  
{
private:
	complex *pVoltage;		// reference to the assigned panel circuit voltage
	house *pHouse;			// reference to the parent house

public:
	double size;  ///< freezer volume (cf) 
	double rated_capacity;  ///< rated capacity (Btu/h)
	double thermostat_deadband;  ///< freezer thermostat hysterisys (degF)
	double UAr;		///< UA of Refrigerator compartment
	double UAf;		///< UA of the food-air
	double Tair;	///< Refirgerator air temperature (degF)
	double Tout;	///< House air temperature
	double Tset;	///< Refrigerator control set point temperature (degF)
	double Cf;		///< heat capapcity of the food
	double Qr;		///< heat rate from the cooling system
	double COPcoef;	///< compressor COP

	complex power_kw;				// total power demand [kW]
	double power_factor;
	double kwh_meter;				// energy used since start of simulation [kWh] 

public:

	static CLASS *oclass;
	static freezer *defaults;
	freezer(MODULE *module);
	~freezer();
	
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
};

#endif // _FREEZER_H

/**@}**/
