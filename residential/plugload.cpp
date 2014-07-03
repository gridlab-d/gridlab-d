/** $Id: plugload.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file plugload.cpp
	@addtogroup plugload
	@ingroup residential
	
	The plugload simulation is based on demand profile of the connected plug loads.
	Heat fraction ratio is used to calculate the internal gain from plug loads.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "plugload.h"

//////////////////////////////////////////////////////////////////////////
// plugload CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* plugload::oclass = NULL;
CLASS* plugload::pclass = NULL;

plugload::plugload(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"plugload",sizeof(plugload),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class plugload";
		else
			oclass->trl = TRL_QUALIFIED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit]",PADDR(shape.load),
			PT_double,"installed_power[kW]",PADDR(shape.params.analog.power), PT_DESCRIPTION, "installed plugs capacity",
			PT_complex,"actual_power[kVA]",PADDR(plugs_actual_power),PT_DESCRIPTION,"actual power demand",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

plugload::~plugload()
{
}

int plugload::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.power_fraction = load.current_fraction = load.impedance_fraction = 0;
	load.heatgain_fraction = 0.90;
	load.power_factor = 0.90;
	//load.power_fraction = 1.0;
	load.voltage_factor = 1.0; // assume 'even' voltage, initially
	shape.load = gl_random_uniform(RNGSTATE,0, 0.1);
	return res;
}

int plugload::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	load.breaker_amps = 40;

	if ( (load.power_fraction + load.current_fraction + load.impedance_fraction) == 0.0)
	{
		load.power_fraction = 1.0;
		load.current_fraction = 0.0;
		load.impedance_fraction = 0.0;
	}

	return residential_enduse::init(parent);
}

int plugload::isa(char *classname)
{
	return (strcmp(classname,"plugload")==0 || residential_enduse::isa(classname));
}

TIMESTAMP plugload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP t2 = TS_NEVER;
	double val = 0.0;

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor

	t2 = residential_enduse::sync(t0,t1);

	if (pCircuit->status==BRK_CLOSED) 
	{
		if (shape.type == MT_UNKNOWN)
		{
			if(shape.load < 0.0){
				gl_error("plugload demand cannot be negative, capping");
				shape.load = 0.0;
			}
			load.power = load.power_fraction * shape.load;
			load.current = load.current_fraction * shape.load;
			load.admittance = load.impedance_fraction * shape.load;
			if(fabs(load.power_factor) < 1 && load.power_factor != 0.0){
				val = (load.power_factor < 0 ? -1.0 : 1.0) * load.power.Re() * sqrt(1/(load.power_factor * load.power_factor) - 1);
			} else {
				val = 0;
			}
			load.power.SetRect(load.power.Re(), val);
		}
	}
	else
		load.power = load.current = load.admittance = complex(0,0,J);

	gl_enduse_sync(&(residential_enduse::load),t1);

	plugs_actual_power = load.power + (load.current + load.admittance * load.voltage_factor) * load.voltage_factor;
	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_plugload(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(plugload::oclass);
		if (*obj!=NULL)
		{
			plugload *my = OBJECTDATA(*obj,plugload);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(plugload);
}

EXPORT int init_plugload(OBJECT *obj)
{
	try
	{
		plugload *my = OBJECTDATA(obj,plugload);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(plugload);
}

EXPORT int isa_plugload(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,plugload)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_plugload(OBJECT *obj, TIMESTAMP t0)
{
	try
	{
		plugload *my = OBJECTDATA(obj, plugload);
		TIMESTAMP t1 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(plugload);
}

/**@}**/
