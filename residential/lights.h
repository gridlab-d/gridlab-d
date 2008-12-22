/** $Id: lights.h,v 1.17 2008/01/17 21:03:22 d3p988 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.h
	@addtogroup residential_lights

 @{
 **/

#ifndef _LIGHTS_H
#define _LIGHTS_H

#include "residential.h"

class lights  
{
private:
	complex *pVoltage;
	ENDUSELOAD load;

public:
	enum { // make certain this matchers the power_factor table
		INCANDESCENT,	///< incandescent lights
		FLUORESCENT,	///< fluorescent lights
		CFL,			///< compact fluorescent lights
		SSL,			///< solid state lights
		HID,			///< high-intensity discharge lights
		_MAXTYPES
	} type;				///< lighting type
	static double power_factor[_MAXTYPES]; ///< Lighting power factors (the ordinals must match the \p type enumeration)
	enum {
		INDOOR=0,		///< indoor lighting (100% indoor heat gain)
		OUTDOOR=1,		///< outdoor lighting (0% indoor heat gain)
	} placement;		///< lighting location 
	double circuit_split;			///< -1=100% negative, 0=balanced, +1=100% positive
	double power_density;			///< Installed lighting power density [W/sf]
	double installed_power;			///< installed wattage [W]
	double demand;					///< fraction of light that is on [pu]

public:
	static CLASS *oclass;
	static lights *defaults;
	lights(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _LIGHTS_H

/**@}**/
