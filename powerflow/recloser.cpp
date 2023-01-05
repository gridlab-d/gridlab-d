/** $Id: recloser.cpp,v 2.2 2011/01/05 11:15:00 fish334 Exp $
	Copyright (C) 2011 Battelle Memorial Institute
	@file recloser.cpp
	@addtogroup powerflow recloser
	@ingroup powerflow

	Implements a recloser object.

 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "recloser.h"

CLASS* recloser::oclass = nullptr;
CLASS* recloser::pclass = nullptr;

recloser::recloser(MODULE *mod) : switch_object(mod)
{
	if(oclass == nullptr)
	{
		pclass = link_object::oclass;

		oclass = gl_register_class(mod,"recloser",sizeof(recloser),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
        if(oclass == nullptr)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

        if(gl_publish_variable(oclass,
			PT_INHERIT, "switch",
			PT_double, "retry_time[s]", PADDR(retry_time), PT_DESCRIPTION, "the amount of time in seconds to wait before the recloser attempts to close",
			PT_double, "max_number_of_tries", PADDR(ntries), PT_DESCRIPTION, "the number of times the recloser will try to close before permanently opening",
			PT_double, "number_of_tries", PADDR(curr_tries), PT_DESCRIPTION, "Current number of tries recloser has attempted",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,"change_recloser_state",(FUNCTIONADDR)change_recloser_state)==nullptr)
			GL_THROW("Unable to publish recloser state change function");
		if (gl_publish_function(oclass,"recloser_reliability_operation",(FUNCTIONADDR)recloser_reliability_operation)==nullptr)
			GL_THROW("Unable to publish recloser reliability operation function");
		if (gl_publish_function(oclass,	"change_recloser_faults", (FUNCTIONADDR)recloser_fault_updates)==nullptr)
			GL_THROW("Unable to publish recloser fault correction function");
        if (gl_publish_function(oclass,	"fix_fault", (FUNCTIONADDR)fix_fault_switch)==nullptr)
            GL_THROW("Unable to publish recloser fault restoration function");

        //Publish deltamode functions -- replicate switch
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_switch)==nullptr)
			GL_THROW("Unable to publish recloser deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish recloser external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish recloser external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish recloser external current calculation function");
    }
}

int recloser::isa(char *classname)
{
	return strcmp(classname,"recloser")==0 || switch_object::isa(classname);
}

int recloser::create()
{
	int result = switch_object::create();

	prev_full_status = 0x00;		//Flag as all open initially
	switch_banked_mode = BANKED_SW;	//Assume operates in banked mode normally
	phase_A_state = SW_CLOSED;			//All switches closed by default
	phase_B_state = SW_CLOSED;
	phase_C_state = SW_CLOSED;

	prev_SW_time = 0;
	retry_time = 0;
	ntries = 0;
	curr_tries = 0.0;
	prev_rec_time = 0;
	return result;
}

int recloser::init(OBJECT *parent)
{
	int result = switch_object::init(parent);

	//Check for deferred
	if (result == 2)
		return 2;	//Return the deferment - no sense doing everything else!

	TIMESTAMP next_ret;
	next_ret = gl_globalclock;

	if(ntries<0)
	{
		gl_warning("The number of recloser tries is less than 0 resetting to zero");
		ntries = 0;
	}

	if(retry_time < 0)
	{
		gl_warning("retry time is < 0 resetting to zero");
		retry_time = 0;
	}

	if(ntries == 0)
		ntries = 3; // seting default to 3 tries
	if(retry_time == 0)
	{
		retry_time = 1; // setting default to 1 second
		return_time = 1;
	} else {
		return_time = (TIMESTAMP)floor(retry_time + 0.5);
	}
	return result;
}

//sync function - serves only to reset number of tries
TIMESTAMP recloser::sync(TIMESTAMP t0)
{
	//Check and update time
	if (prev_rec_time != t0)
	{
		prev_rec_time = t0;
		curr_tries = 0.0;
	}

	return switch_object::sync(t0);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: recloser
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_recloser(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(recloser::oclass);
		if (*obj!=nullptr)
		{
			recloser *my = OBJECTDATA(*obj,recloser);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(recloser);
}

/**
* Object initialization is called once after all object have been created
*
* @param obj a pointer to this object
* @return 1 on success, 0 on error
*/
EXPORT int init_recloser(OBJECT *obj)
{
	try {
		recloser *my = OBJECTDATA(obj,recloser);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(recloser);
}

//Syncronizing function
EXPORT TIMESTAMP sync_recloser(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		recloser *pObj = OBJECTDATA(obj,recloser);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return pObj->presync(t0);
		case PC_BOTTOMUP:
			return pObj->sync(t0);
		case PC_POSTTOPDOWN:
			t1 = pObj->postsync(t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	SYNC_CATCHALL(recloser);
}

EXPORT int isa_recloser(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,recloser)->isa(classname);
}

//Function to change recloser states - just call underlying switch routine
EXPORT double change_recloser_state(OBJECT *thisobj, unsigned char phase_change, bool state)
{
	double count_values;
	char desA, desB, desC;
	recloser *reclobj;
	switch_object *swtchobj;
	FUNCTIONADDR funadd = nullptr;

	//Init
	count_values = 0.0;

	//Map us as a recloser - just so we can get our count
	reclobj = OBJECTDATA(thisobj,recloser);

	//Set count
	if (!state)
		count_values = reclobj->ntries;	//Opening, so must have "tried" all times
	else
		count_values = 1.0;	//Just a non-zero value

	//Map the switch
	swtchobj = OBJECTDATA(thisobj,switch_object);

	if ((swtchobj->switch_banked_mode == switch_object::BANKED_SW) || meshed_fault_checking_enabled)
	{
		swtchobj->set_switch(state);
	}
	else	//Must be individual
	{
		//Figure out what we need to call
		if ((phase_change & 0x04) == 0x04)
		{
			if (state)
				desA=1;	//Close it
			else
				desA=0;	//Open it
		}
		else	//Nope, no A
			desA=2;		//I don't care

		//Phase B
		if ((phase_change & 0x02) == 0x02)
		{
			if (state)
				desB=1;	//Close it
			else
				desB=0;	//Open it
		}
		else	//Nope, no B
			desB=2;		//I don't care

		//Phase C
		if ((phase_change & 0x01) == 0x01)
		{
			if (state)
				desC=1;	//Close it
			else
				desC=0;	//Open it
		}
		else	//Nope, no A
			desC=2;		//I don't care

		//Perform the switching!
		swtchobj->set_switch_full(desA,desB,desC);
	}//End individual adjustments

	return count_values;
}

//Reliability interface - calls underlying switch routine
EXPORT int recloser_reliability_operation(OBJECT *thisobj, unsigned char desired_phases)
{
	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);

	swtchobj->set_switch_full_reliability(desired_phases);

	return 1;	//This will always succeed...because I say so!
}

EXPORT int recloser_fault_updates(OBJECT *thisobj, unsigned char restoration_phases)
{
	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Call the update
	thisswitch->set_switch_faulted_phases(restoration_phases);

	return 1;	//We magically always succeed
}
