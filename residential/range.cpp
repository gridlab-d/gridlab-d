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
CLASS* range::pclass = NULL;

range::range(MODULE *module) : residential_enduse(module)
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
			PT_INHERIT, "residential_enduse",
			PT_double,"installed_power[kW]",PADDR(shape.params.analog.power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit]",PADDR(shape.load),
			PT_complex,"energy_meter[kWh]",PADDR(load.energy),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);


	}
}

range::~range()
{
}

int range::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);

	return res;
}

int range::init(OBJECT *parent)
{
	if(shape.params.analog.power < 0){
		gl_warning("range installed power is negative, using random default");
		installed_power = 0;
	}
	if(load.heatgain_fraction < 0.0 || load.heatgain_fraction > 1.0){
		gl_warning("range heat_fraction out of bounds, restoring default");
		heat_fraction = 0;
	}
	
	if (shape.params.analog.power==0) shape.params.analog.power = gl_random_uniform(8,15);	// range size [W]; based on a GE drop-in range 11600kW;
	if (load.power_factor==0) load.power_factor = 1.0;
	if (load.heatgain_fraction==0) load.heatgain_fraction = 0.9;
	if (load.voltage_factor==0) load.voltage_factor = 1.0;
	
	load.config = EUC_IS220;
	load.breaker_amps = 30;

	load.total = complex(shape.params.analog.power*shape.load,0,J);
	load.admittance = load.total*(240/240);
	load.heatgain = load.total.Mag()*load.heatgain_fraction;

	return residential_enduse::init(parent);
}

int range::isa(char *classname)
{
	return (strcmp(classname,"range")==0 || residential_enduse::isa(classname));
}

TIMESTAMP range::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double val = 0.0;
	TIMESTAMP t2 = TS_NEVER;

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 240; // update voltage factor

	if(shape.load < 0.0){
		GL_THROW("range demand is negative");
	}
	if(shape.load > 1.0){
		GL_THROW("range demand is greater than 1.0 and out of bounds");
	}

	t2 = residential_enduse::sync(t0,t1);

	if(shape.type == MT_UNKNOWN){ /* manual power calculation*/
		double frac = shape.load;
		if(shape.load < 0){
			gl_warning("range shape demand is negative, capping to 0");
			shape.load = 0.0;
		} else if (shape.load > 1.0){
			gl_warning("range shape demand exceeds installed lighting power, capping to 100%%");
			shape.load = 1.0;
		}
		load.power = shape.params.analog.power * shape.load;
		if(fabs(load.power_factor) < 1){
			val = (load.power_factor<0?-1.0:1.0) * load.power.Re() * sqrt(1/(load.power_factor * load.power_factor) - 1);
		} else {
			val = 0;
		}
		load.power.SetRect(load.power.Re(), val);
	}

	gl_enduse_sync(&(residential_enduse::load),t1);

	return t2;
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

EXPORT int isa_range(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,range)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_range(OBJECT *obj, TIMESTAMP t0)
{
	range *my = OBJECTDATA(obj, range);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
