/** $Id: occupantload.cpp,v 1.8 2008/02/10 00:11:17 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file occupantload.cpp
	@addtogroup occupantload
	@ingroup residential
	
	The occupantload model is based on occupancy fraction/schedule.
	DOE-2 assumptions are used for calculating the internal gain from occupant load.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "occupantload.h"


//////////////////////////////////////////////////////////////////////////
// occupantload CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* occupantload::oclass = NULL;
occupantload *occupantload::defaults = NULL;

occupantload::occupantload(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"occupantload",sizeof(occupantload),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_int32,"number_of_occupants",PADDR(number_of_occupants),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_double,"occupancy_fraction[unit]",PADDR(occupancy_fraction),
			PT_double,"heatgain_per_person[Btu/h]",PADDR(heatgain_per_person),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(occupantload));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

occupantload::~occupantload()
{
}

int occupantload::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(occupantload));
	return 1;
}

int occupantload::init(OBJECT *parent)
{
	if (number_of_occupants==0)	number_of_occupants = 4;		// defaulted to 4, but perhaps define it based on house size??
	if (heatgain_per_person==0) heatgain_per_person = 400.0;	// Based on DOE-2, includes latent and sensible heatgain

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || (!gl_object_isa(parent,"house") && !gl_object_isa(parent,"house_e")))
	{
		gl_error("occupantload must have a parent house");
		/*	TROUBLESHOOT
			The occupantload object, being an enduse for the house model, must have a parent house
			that it is connected to.  Create a house object and set it as the parent of the
			offending occupantload object.
		*/
		return 0;
	}

	//	pull parent attach_enduse and attach the enduseload
	FUNCTIONADDR attach = 0;
	load.end_obj = hdr;
	attach = (gl_get_function(parent, "attach_enduse"));
	if(attach == NULL){
		gl_error("occupantload parent must publish attach_enduse()");
		/*	TROUBLESHOOT
			The occupantload object attempt to attach itself to its parent, which
			must implement the attach_enduse function.
		*/
		return 0;
	}
	// Needed to pass heat gain up to the house
	// "true" on 220 keeps the circuits "balanced"
	((CIRCUIT *(*)(OBJECT *, ENDUSELOAD *, double, int))(*attach))(hdr->parent, &(this->load), 20, true);

	load.heatgain = number_of_occupants * occupancy_fraction * heatgain_per_person * KWPBTUPH;

	return 1;
}

TIMESTAMP occupantload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	load.heatgain = number_of_occupants * occupancy_fraction * heatgain_per_person * KWPBTUPH;

	return TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_occupantload(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(occupantload::oclass);
	if (*obj!=NULL)
	{
		occupantload *my = OBJECTDATA(*obj,occupantload);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_occupantload(OBJECT *obj)
{
	occupantload *my = OBJECTDATA(obj,occupantload);
	return my->init(obj->parent);
}

EXPORT TIMESTAMP sync_occupantload(OBJECT *obj, TIMESTAMP t0)
{
	occupantload *my = OBJECTDATA(obj, occupantload);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
