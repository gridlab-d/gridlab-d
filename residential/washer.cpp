/* washer.cpp
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "washer.h"

washer::washer() 
{
	memset(this, 0, sizeof(washer));
}

washer::~washer()
{
}

void washer::create() 
{
	// default properties
	installed_power = gl_random_uniform(500,750);		// washer size [W]

	// derived properties and other initial conditions
	demand = 0.0;
	heat_loss = 0.0;
	power_demand = 0.0;
	kwh_meter = 0.0;
}

int washer::isa(char *classname)
{
	return (strcmp(classname,"washer")==0 || residential_enduse::isa(classname));
}


TIMESTAMP washer::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// get demand fraction as an average per hour since the last synch time
	// a function  prototype will be 'get_average_demand(t0, t1)' which will be used to set 'demand' 
	// by default 'demand' is 1.0, now provided by the tape
//	demand = 1.0;

	if (t0 <= 0)
		return TS_NEVER;

	power_demand.SetPolar(installed_power * demand / 1000.0, acos(1.0), J);
	double energy = power_demand.Mag() * (t1-t0);
	heat_loss = energy;
	kwh_meter += energy;

	return TS_NEVER; 
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
CDECL CLASS *washer_class = NULL;
CDECL OBJECT *last_washer = NULL;

EXPORT int create_washer(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(washer_class,sizeof(washer));
	if (*obj!=NULL)
	{
		last_washer = *obj;
		washer *my = OBJECTDATA(*obj,washer);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int isa_washer(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,washer)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_washer(OBJECT *obj, TIMESTAMP t0)
{
	washer *my = OBJECTDATA(obj, washer);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}
