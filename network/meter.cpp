/** $Id: meter.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file meter.cpp
	@addtogroup meter Meter
	@ingroup network
	
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"


//////////////////////////////////////////////////////////////////////////
// meter CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////

CLASS* meter::oclass = NULL;
meter *meter::defaults = NULL;
CLASS* meter::pclass = NULL;
//CLASS *meter_class = NULL;

CLASS *meter_class = (NULL);

meter::meter(MODULE *mod) : node(mod)
{
	// first time init
	if (oclass==NULL)
	{
		// link to parent class (used by isa)
		pclass = node::oclass;

		// register the class definition
		meter_class = oclass = gl_register_class(mod,"meter",sizeof(meter),PC_BOTTOMUP);
		if (oclass==NULL)
			throw "unable to register class meter";
		else
			oclass->trl = TRL_STANDALONE;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration,"type",PADDR(type),
				PT_KEYWORD,"MT_ONEPHASE",MT_ONEPHASE,
				PT_KEYWORD,"MT_THREEPHASE",MT_THREEPHASE,
			PT_complex, "demand[kW]", PADDR(demand),
			PT_complex, "meter[kWh]", PADDR(meterkWh),
			PT_complex, "line1_current[A]", PADDR(line1_current),
			PT_complex, "line2_current[A]", PADDR(line2_current),
			PT_complex, "line3_current[A]", PADDR(line3_current),
			PT_complex, "line1_admittance[1/Ohm]", PADDR(line1_admittance),
			PT_complex, "line2_admittance[1/Ohm]", PADDR(line2_admittance),
			PT_complex, "line3_admittance[1/Ohm]", PADDR(line3_admittance),
			PT_complex, "line1_power[S]", PADDR(line1_power),
			PT_complex, "line2_power[S]", PADDR(line2_power),
			PT_complex, "line3_power[S]", PADDR(line3_power),
			PT_complex, "line1_volts[V]", PADDR(V),
			PT_complex, "line2_volts[V]", PADDR(V),
			PT_complex, "line3_volts[V]", PADDR(V),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		type = MT_ONEPHASE;
		demand = complex(0,0);
		meterkWh = complex(0,0);
		line1_current = complex(0,0);
		line2_current = complex(0,0);
		line3_current = complex(0,0);
		line1_admittance = complex(0,0);
		line2_admittance = complex(0,0);
		line3_admittance = complex(0,0);
		line1_power = complex(0,0);
		line2_power = complex(0,0);
		line3_power = complex(0,0);
	}
}

int meter::create() 
{
	//node::create();
	//node::type = PQ;

	int result = node::create();
	memcpy(this,defaults,sizeof(*this));
	return result;

/*
	type = MT_ONEPHASE;
	demand_kW = complex(0,0);
	meter_kWh = complex(0,0);
	line1_A = complex(0,0);
	line2_A = complex(0,0);
	line3_A = complex(0,0);
	line1_Y = complex(0,0);
	line2_Y = complex(0,0);
	line3_Y = complex(0,0);
	line1_S = complex(0,0);
	line2_S = complex(0,0);
	line3_S = complex(0,0);
*/
	return 1;
}
// Initialize a distribution meter, return 1 on success
int meter::init(OBJECT *parent)
{
	return node::init(parent);
}

TIMESTAMP meter::postsync(TIMESTAMP t0, TIMESTAMP t1) 
{
	switch (type) {
	case MT_ONEPHASE:
		S = line1_power + line3_power;
		//S = V*(line1_A + line3_A);
		break;
	case MT_THREEPHASE:
		Ys += line1_admittance + line2_admittance + line3_admittance;
		YVs += line1_current + line2_current + line3_current;
		S = line1_power + line2_power + line3_power;
		//S = V*(line1_A + line2_A + line3_A);
		break;
	default:
		return TS_NEVER;
	}
	meterkWh = S * (double)((t1-t0)/TS_SECOND/3600);
	return node::postsync(t1);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: meter
//////////////////////////////////////////////////////////////////////////



EXPORT int create_meter(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(meter_class);
	if (*obj!=NULL)
	{
		meter *my = OBJECTDATA(*obj,meter);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_meter(OBJECT *obj)
{
	meter *my = OBJECTDATA(obj,meter);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_meter(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP t1 = OBJECTDATA(obj,meter)->postsync(obj->clock,t0);
	obj->clock = t0;
	return t1;
}

EXPORT TIMESTAMP meter_isa(OBJECT *obj, char *type)
{
	return strcmp(type,"meter")==0 || strcmp(type,"node")==0;
}
/**@}*/
