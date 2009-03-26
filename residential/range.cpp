/** $Id: range.cpp,v 1.17 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file range.cpp
	@addtogroup range
	@ingroup residential

	The range simulation is based on a demand profile attached to the object.
	The internal heat gain is calculated as the demand fraction of installed power.

	@{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "range.h"

//////////////////////////////////////////////////////////////////////////
// range CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* range::oclass = NULL;
range *range::defaults = NULL;

range::range(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"range",sizeof(range),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double,"installed_power[W]",PADDR(installed_power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit]",PADDR(demand),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_double,"energy_meter[kWh]",PADDR(load.energy),
			PT_double,"heat_fraction[unit]",PADDR(heat_fraction),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(range));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

range::~range()
{
}

int range::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(range));

	return 1;
}

int range::init(OBJECT *parent)
{
	if (installed_power==0) installed_power = gl_random_uniform(8000,15000);	// range size [W]; based on a GE drop-in range 11600kW;
	if (power_factor==0) power_factor = 1.0;
	if (heat_fraction==0) heat_fraction = 0.9;
	
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL)
	{
		gl_error("range must have a parent house");
		return 0;
	}

	// attach object to house panel
	house *pHouse = OBJECTDATA(parent,house);
	pVoltage = (pHouse->attach(OBJECTHDR(this),30,true))->pV;

	load.total = complex(installed_power*demand/1000,0,J);
	load.admittance = load.total*(1000/240/240);
	load.heatgain = load.total.Mag()*heat_fraction;

	return 1;
}

TIMESTAMP range::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	if (t0>0 && t1>t0)
		load.energy += load.total.Mag() * gl_tohours(t1-t0);

	load.total = complex(installed_power*demand,0,J);
	load.admittance = load.total*(1.0/240/240);
	load.heatgain = load.total.Mag()*heat_fraction;

	return TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////


EXPORT int create_range(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(range::oclass);
	if (*obj!=NULL)
	{
		range *my = OBJECTDATA(*obj,range);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_range(OBJECT *obj)
{
	range *my = OBJECTDATA(obj,range);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_range(OBJECT *obj, TIMESTAMP t0)
{
	range *my = OBJECTDATA(obj, range);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
