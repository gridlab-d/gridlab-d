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
CLASS* dishwasher::pclass = NULL;
dishwasher *dishwasher::defaults = NULL;

dishwasher::dishwasher(MODULE *module) : residential_enduse(module)
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
			PT_INHERIT, "residential_enduse",
			PT_double,"installed_power[kW]",PADDR(shape.params.analog.power), PT_DESCRIPTION, "installed power draw",
			PT_double,"demand[unit]",PADDR(shape.load),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

dishwasher::~dishwasher()
{
}

int dishwasher::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.power_factor = 0.95;
	load.power_fraction = 1.0;
	load.shape->type = MT_PULSED;

	return res;
}

int dishwasher::init(OBJECT *parent)
{
	// default properties
	if(shape.params.analog.power == 0){
		shape.params.analog.power = gl_random_uniform(1000, 3000);		// dishwasher size [W]
	}
	if(load.heatgain_fraction == 0){
		load.heatgain_fraction = 0.50;  //Assuming 50% of the power is transformed to internal heat
	}
	if(load.power_factor == 0){
		load.power_factor = 0.95;
	}

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	/* handle schedules */
	switch(shape.type){
		case MT_UNKNOWN:
			break;
		case MT_ANALOG:
			break;
		case MT_PULSED:
			break;
		case MT_MODULATED:
			break;
		case MT_QUEUED:
			break;
		default:
			char name[64];
			if(hdr->name == 0){
				sprintf(name, "%s:%s", hdr->oclass->name, hdr->id);
			}
			gl_error("dishwasher %s schedule of an unrecognized type", hdr->name ? hdr->name : name);
	}

	return residential_enduse::init(parent);
}

int dishwasher::isa(char *classname)
{
	return (strcmp(classname,"dishwasher")==0 || residential_enduse::isa(classname));
}


TIMESTAMP dishwasher::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = residential_enduse::sync(t0, t1);

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor

	if(shape.type == MT_UNKNOWN){ /* requires manual enduse control */
		double real = 0.0, imag = 0.0;
		real = shape.params.analog.power * shape.load;

		if(fabs(load.power_factor) < 1){
			imag = (load.power_factor<0?-1.0:1.0) * real * sqrt(1/(load.power_factor * load.power_factor) - 1);
		} else {
			imag = 0;
		}

		load.power.SetRect(real, imag); // convert from W to kW
	}
	
	gl_enduse_sync(&(residential_enduse::load), t1);

	return t2; 
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

EXPORT int isa_dishwasher(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,dishwasher)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_dishwasher(OBJECT *obj, TIMESTAMP t0)
{
	dishwasher *my = OBJECTDATA(obj, dishwasher);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
