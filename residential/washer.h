// washer.h

#ifndef _WASHER_H
#define _WASHER_H

#include "residential.h"

class washer  
{

public:

	double circuit_split;			// -1=100% negative, 0=balanced, +1=100% positive
	double installed_power;		// installed washer power [W]
	double demand	;			// fraction of washer load [W]
	complex power_demand;		// average MW usage [MWh/h]
	double kwh_meter;			// MWhmeter [MWh]
	double heat_loss;				// heat losses [BTU/h]

public:

	washer();
	~washer();
	void create();
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	void pre_update(void);
	void post_update(void);

};

#endif // _Washer_H
