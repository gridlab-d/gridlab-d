/** $Id: dishwasher.h,v 1.8 2008/02/09 00:05:09 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.h
	@addtogroup dishwasher
	@ingroup residential

 @{
 **/

#ifndef _DISHWASHER_H
#define _DISHWASHER_H

#include "residential.h"
#include "residential_enduse.h"

class dishwasher : public residential_enduse
{

public:
	double circuit_split;		///< -1=100% negative, 0=balanced, +1=100% positive
	double installed_power;		///< installed washer power [W] (default = random uniform between 1000 - 3000 W)
	double demand	;			///< fraction of dishwasher load
	double heat_fraction;		///< internal gain fraction of installed power (default = 0.5)
	double power_factor;		///< power factor (default = 0.95)

public:
static CLASS *oclass, *pclass;
static dishwasher *defaults;

	dishwasher(MODULE *module);
	~dishwasher();
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _DISHWASHER_H

/**@}**/
