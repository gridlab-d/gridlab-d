/** $Id: range.h,v 1.13 2008/02/09 23:48:51 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file range.h
	@addtogroup range
	@ingroup residential

 @{
 **/

#ifndef _RANGE_H
#define _RANGE_H
#include "residential.h"

class range  
{
private:
	complex *pVoltage;

public:
	double circuit_split;		///< -1=100% negative, 0=balanced, +1=100% positive
	double installed_power;		///< installed wattage [W]
	double demand;				///< fraction of installed power currently used
	double power_factor;		///< power factor (default = 1.0)
	double heat_fraction;		///< fraction of the plugload that is transferred as heat (default = 0.90)
	ENDUSELOAD load;			///< enduse load structure


public:
	static CLASS *oclass;
	static range *defaults;

	range(MODULE *module);
	~range();
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _RANGE_H

/**@}**/
