/** $Id: dishwasher.cpp,v 1.12 2008/02/13 02:22:35 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.cpp
	@addtogroup dishwasher
	@ingroup residential

	The dishwasher simulation is based on a demand profile attached to the object.
	The internal heat gain is calculated using a specified fraction of installed power.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "dishwasher.h"

//////////////////////////////////////////////////////////////////////////
// dishwasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* dishwasher::oclass = NULL;
dishwasher *dishwasher::defaults = NULL;

dishwasher::dishwasher(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"dishwasher",sizeof(dishwasher),PC_BOTTOMUP);
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
			PT_double,"heat_fraction",PADDR(heat_fraction),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(dishwasher));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

dishwasher::~dishwasher()
{
}

int dishwasher::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(dishwasher));

	return 1;
}

int dishwasher::init(OBJECT *parent)
{
	// default properties
	if (installed_power==0) installed_power = gl_random_uniform(1000,3000);		// dishwasher size [W]
	if (heat_fraction==0) heat_fraction = 0.50;  //Assuming 50% of the power is transformed to internal heat
	if (power_factor==0) power_factor = 0.95;

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || !gl_object_isa(parent,"house"))
	{
		gl_error("dishwasher must have a parent house");
		/*	TROUBLESHOOT
			The dishwasher object, being an enduse for the house model, must have a parent house
			that it is connected to.  Create a house object and set it as the parent of the
			offending dishwasher object.
		*/
		return 0;
	}

	// attach object to house panel
	house *pHouse = OBJECTDATA(parent,house);
	pVoltage = (pHouse->attach(OBJECTHDR(this),20,false))->pV;

	// compute initial power draw
	load.power.SetPowerFactor(installed_power*demand/1000.0, power_factor, J);

	return 1;
}

TIMESTAMP dishwasher::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	if (t0>0 && t1>t0)
		load.energy += load.total.Mag() * gl_tohours(t1-t0);

	load.power.SetPowerFactor(installed_power*demand/1000.0, power_factor, J);

	// update heatgain
	load.total = load.power + ~(load.current + load.admittance**pVoltage)**pVoltage/1000;
	load.heatgain = load.total.Mag()*heat_fraction;

	return TS_NEVER; 
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: dishwasher
//////////////////////////////////////////////////////////////////////////

EXPORT int create_dishwasher(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(dishwasher::oclass);
	if (*obj!=NULL)
	{
		dishwasher *my = OBJECTDATA(*obj,dishwasher);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_dishwasher(OBJECT *obj)
{
	dishwasher *my = OBJECTDATA(obj,dishwasher);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_dishwasher(OBJECT *obj, TIMESTAMP t0)
{
	dishwasher *my = OBJECTDATA(obj, dishwasher);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
