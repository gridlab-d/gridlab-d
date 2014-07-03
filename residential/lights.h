/** $Id: lights.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.h
	@addtogroup residential_lights

 @{
 **/

#ifndef _LIGHTS_H
#define _LIGHTS_H

#include "residential.h"
#include "residential_enduse.h"

class lights : public residential_enduse {
public:
	typedef enum { // make certain this matchers the power_factor table
		INCANDESCENT=0,	///< incandescent lights
		FLUORESCENT,	///< fluorescent lights
		CFL,			///< compact fluorescent lights
		SSL,			///< solid state lights
		HID,			///< high-intensity discharge lights
		_MAXTYPES
	};
	enumeration type;				///< lighting type
	static double power_factor[_MAXTYPES]; ///< Lighting power factors (the ordinals must match the \p type enumeration)
	static double power_fraction[_MAXTYPES][3];
	typedef enum {
		INDOOR=0,		///< indoor lighting (100% indoor heat gain)
		OUTDOOR=1,		///< outdoor lighting (0% indoor heat gain)
	};
	enumeration placement;		///< lighting location 
	double circuit_split;			///< -1=100% negative, 0=balanced, +1=100% positive (DEPRECATED)
	double power_density;			///< Installed lighting power density [W/sf]
	double curtailment;				///< fractional curtailment of lighting [pu]
	complex lights_actual_power;	///< actual light power demand as function of voltage

public:
	static CLASS *oclass, *pclass;
	lights(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _LIGHTS_H

/**@}**/
