/** $Id: occupantload.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
CLASS* occupantload::pclass = NULL;

occupantload::occupantload(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"occupantload",sizeof(occupantload),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class occupantload";
		else
			oclass->trl = TRL_QUALIFIED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_int32,"number_of_occupants",PADDR(number_of_occupants),
			PT_double,"occupancy_fraction[unit]",PADDR(occupancy_fraction),
			PT_double,"heatgain_per_person[Btu/h]",PADDR(heatgain_per_person),
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

occupantload::~occupantload()
{
}

int occupantload::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.config = EUC_HEATLOAD;
	load.config |= EUC_IS220;
	return res;
}

int occupantload::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("occupantload::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
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

	load.heatgain = number_of_occupants * occupancy_fraction * heatgain_per_person;

	if(shape.type != MT_UNKNOWN && shape.type != MT_ANALOG){
		char outname[64];
		if(hdr->name){
			//sprintf(outname, "%s", hdr->name);
		} else {
			sprintf(outname, "occupancy_load:%i", hdr->id);
		}
		gl_warning("occupancy_load \'%s\' may not work properly with a non-analog load shape.", hdr->name ? hdr->name : outname);
	}
	return 1;
}

int occupantload::isa(char *classname)
{
	return (strcmp(classname,"occupantload")==0 || residential_enduse::isa(classname));
}

TIMESTAMP occupantload::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	/* sanity checks */
	if(heatgain_per_person < 0){
		gl_error("negative heatgain per person, reseting to 400 BTU/hr");
		heatgain_per_person = 400.0;
	}
	if(heatgain_per_person > 1600){
		//	Bob's party is on fire.  Literally.
		gl_error("heatgain per person above 1600 Btu/hr (470W), reseting to 400 Btu/hr");
		heatgain_per_person = 400.0;
	}


	if(shape.type == MT_UNKNOWN){
		if(number_of_occupants < 0){
			gl_error("negative number of occupants, reseting to zero");
			number_of_occupants = 0;
		}
		if(occupancy_fraction < 0.0){
			gl_error("negative occupancy_fraction, reseting to zero");
			occupancy_fraction = 0.0;
		}
		if(occupancy_fraction > 1.0){
			; /* party at Bob's house! */
		}
		if(occupancy_fraction * number_of_occupants > 300.0){
			gl_error("attempting to fit 300 warm bodies into a house, reseting to zero");
			// let's assume that the police cleared the party
			// or the fire department said 'this is a bad sign, people!'
			// how about that the house just plain collapsed?
			occupancy_fraction = 0;
		}

		load.heatgain = number_of_occupants * occupancy_fraction * heatgain_per_person;
	}

	return TS_NEVER; 
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_occupantload(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(occupantload::oclass);
		if (*obj!=NULL)
		{
			occupantload *my = OBJECTDATA(*obj,occupantload);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(occupantload);
}

EXPORT int init_occupantload(OBJECT *obj)
{
	try
	{
		occupantload *my = OBJECTDATA(obj,occupantload);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(occupantload);
}

EXPORT int isa_occupantload(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,occupantload)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_occupantload(OBJECT *obj, TIMESTAMP t0)
{
	try
	{
		occupantload *my = OBJECTDATA(obj, occupantload);
		TIMESTAMP t1 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(occupantload);
}

/**@}**/
