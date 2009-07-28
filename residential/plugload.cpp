/** $Id: plugload.cpp,v 1.13 2008/02/13 02:22:35 d3j168 Exp $
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
plugload *plugload::defaults = NULL;

plugload::plugload(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"plugload",sizeof(plugload),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit]",PADDR(demand),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_complex,"energy_meter[kWh]",PADDR(load.energy),
			PT_double,"heat_fraction[unit]",PADDR(heat_fraction),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(plugload));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

plugload::~plugload()
{
}

int plugload::create() 
{
	memcpy(this,defaults,sizeof(plugload));

	return 1;
	
}

int plugload::init(OBJECT *parent)
{
	// other derived properties
	if (demand==0) demand = gl_random_uniform(0, 0.1);  // assuming a default maximum 10% of the sync time 
	if (heat_fraction==0) heat_fraction = 0.90;  // assuming default 90% of power is transferred as heat

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || (!gl_object_isa(parent,"house") && !gl_object_isa(parent,"house_e")))
	{
		gl_error("plugload must have a parent house");
		/*	TROUBLESHOOT
			The plugload object, being an enduse for the house model, must have a parent house
			that it is connected to.  Create a house object and set it as the parent of the
			offending plugload object.
		*/
		return 0;
	}
	house *pHouse = OBJECTDATA(parent,house);

	//	pull parent attach_enduse and attach the enduseload
	FUNCTIONADDR attach = 0;
	load.end_obj = hdr;
	attach = (gl_get_function(parent, "attach_enduse"));
	if(attach == NULL){
		gl_error("plugload parent must publish attach_enduse()");
		/*	TROUBLESHOOT
			The Plugload object attempt to attach itself to its parent, which
			must implement the attach_enduse function.
		*/
		return 0;
	}
	pVoltage = ((CIRCUIT *(*)(OBJECT *, ENDUSELOAD *, double, int))(*attach))(hdr->parent, &(this->load), 20, false)->pV;


	// compute the total load and heat gain
	load.total = load.power + ~(load.current + load.admittance**pVoltage)**pVoltage/1000;
	load.heatgain = load.total.Mag() * heat_fraction;

	return 1;
}

TIMESTAMP plugload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// compute the total load and heat gain
	if (t0>0 && t1>t0){
		load.energy += load.total * gl_tohours(t1-t0);
		load.heatgain += load.total.Mag() * heat_fraction * gl_tohours(t1-t0);
	}
	if(demand > 1.0){
		gl_warning("plugload demand cannot be greater than 1.0, capping");
	}
	if(demand < 0.0){
		gl_error("plugload demand cannot be negative, capping");
		demand = 0.0;
	}
	load.total = (load.power + ~(load.current + load.admittance**pVoltage)**pVoltage/1000) ;
	load.total *= demand;
	if(pVoltage->Mag() < 72.0){
		load.total = 0.0; /* assuming too low to power the devices */
	}


	return TS_NEVER; 

}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_plugload(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(plugload::oclass);
	if (*obj!=NULL)
	{
		plugload *my = OBJECTDATA(*obj,plugload);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_plugload(OBJECT *obj)
{
	plugload *my = OBJECTDATA(obj,plugload);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_plugload(OBJECT *obj, TIMESTAMP t0)
{
	plugload *my = OBJECTDATA(obj, plugload);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
