/** $Id: sectionalizer.cpp,v 2.2 2011/01/11 23:00:00 d3x593 Exp $
	Copyright (C) 2011 Battelle Memorial Institute
	@file setionalizer.cpp
	@addtogroup powerflow sectionalizer
	@ingroup powerflow
 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "sectionalizer.h"

//Class management
CLASS* sectionalizer::oclass = nullptr;
CLASS* sectionalizer::pclass = nullptr;

sectionalizer::sectionalizer(MODULE *mod) : switch_object(mod)
{
	// first time init
	if (oclass==nullptr)
	{
		// link to parent class (used by isa)
		pclass = link_object::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"sectionalizer",sizeof(sectionalizer),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_UNSAFE_OVERRIDE_OMIT|PC_AUTOLOCK);
        if(oclass == nullptr)
            GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
        if(gl_publish_variable(oclass,
			PT_INHERIT, "switch",
			NULL) < 1) GL_THROW("unable to publish properties in %s",__FILE__);

		if (gl_publish_function(oclass,"change_sectionalizer_state",(FUNCTIONADDR)change_sectionalizer_state)==nullptr)
			GL_THROW("Unable to publish sectionalizer state change function");
		if (gl_publish_function(oclass,"sectionalizer_reliability_operation",(FUNCTIONADDR)sectionalizer_reliability_operation)==nullptr)
			GL_THROW("Unable to publish sectionalizer reliability operation function");
		if (gl_publish_function(oclass,	"change_sectionalizer_faults", (FUNCTIONADDR)sectionalizer_fault_updates)==nullptr)
			GL_THROW("Unable to publish sectionalizer fault correction function");

		//Publish deltamode functions
		if (gl_publish_function(oclass,	"interupdate_pwr_object", (FUNCTIONADDR)interupdate_switch)==nullptr)
			GL_THROW("Unable to publish sectionalizer deltamode function");

		//Publish restoration-related function (current update)
		if (gl_publish_function(oclass,	"update_power_pwr_object", (FUNCTIONADDR)updatepowercalc_link)==nullptr)
			GL_THROW("Unable to publish sectionalizer external power calculation function");
		if (gl_publish_function(oclass,	"check_limits_pwr_object", (FUNCTIONADDR)calculate_overlimit_link)==nullptr)
			GL_THROW("Unable to publish sectionalizer external power limit calculation function");
		if (gl_publish_function(oclass,	"perform_current_calculation_pwr_link", (FUNCTIONADDR)currentcalculation_link)==nullptr)
			GL_THROW("Unable to publish sectionalizer external current calculation function");
    }
}

int sectionalizer::isa(char *classname)
{
	return strcmp(classname,"sectionalizer")==0 || switch_object::isa(classname);
}

//Creation run
int sectionalizer::create()
{
	int result = switch_object::create();

	prev_full_status = 0x00;		//Flag as all open initially
	switch_banked_mode = BANKED_SW;	//Sectionalizer starts with all three phases open
	phase_A_state = SW_CLOSED;			//All switches closed by default
	phase_B_state = SW_CLOSED;
	phase_C_state = SW_CLOSED;

	return result;
}

//Initialization routine
int sectionalizer::init(OBJECT *parent)
{
	//Nothing to see here, just init it
	return switch_object::init(parent);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int isa_sectionalizer(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj,sectionalizer)->isa(classname);
}

EXPORT int create_sectionalizer(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(sectionalizer::oclass);
		if (*obj!=nullptr)
		{
			sectionalizer *my = OBJECTDATA(*obj,sectionalizer);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(sectionalizer);
}

EXPORT int init_sectionalizer(OBJECT *obj)
{
	try {
		sectionalizer *my = OBJECTDATA(obj,sectionalizer);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(sectionalizer);
}

EXPORT TIMESTAMP sync_sectionalizer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		sectionalizer *pObj = OBJECTDATA(obj,sectionalizer);
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
	SYNC_CATCHALL(sectionalizer);
}

//Function to change sectionalizer states - just call underlying switch routine
EXPORT double change_sectionalizer_state(OBJECT *thisobj, unsigned char phase_change, bool state)
{
	double count_values, recloser_count;
	char desA, desB, desC;
	switch_object *swtchobj;
	sectionalizer *sectionobj;
	FUNCTIONADDR funadd = nullptr;
	bool perform_operation;

	//Init
	count_values = 0.0;

	if (!state && !restoration_checks_active)	//Check routine to find a recloser (or we're in reconfiguration mode)
	{
		//Map us up as a proper object
		sectionobj = OBJECTDATA(thisobj,sectionalizer);

		//Call to see if a recloser is present
		if (fault_check_object == nullptr)
		{
			GL_THROW("Reliability call made without fault_check object present!");
			/*  TROUBLESHOOT
			A sectionalizer attempted to call a reliability-related function.  However, this function
			requires a fault_check object to be present in the system.  Please add the appropriate object.
			If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//map the function
		funadd = (FUNCTIONADDR)(gl_get_function(fault_check_object,"handle_sectionalizer"));

		//make sure it worked
		if (funadd==nullptr)
		{
			GL_THROW("Failed to find sectionalizer checking method on object %s",fault_check_object->name);
			/*  TROUBLESHOOT
			While attempting to find the fault check method, or its subfunction to handle sectionalizers,
			an error was encountered.  Please ensure a proper fault_check object is present in the system.
			If the error persists, please submit your code and a bug report via the trac website.
			*/
		}

		//Function call
		recloser_count = ((double (*)(OBJECT *, int))(*funadd))(fault_check_object,sectionobj->NR_branch_reference);

		if (recloser_count == 0.0)	//Failed :(
		{
			GL_THROW("Failed to handle sectionalizer check on %s",thisobj->name);
			/*  TROUBLESHOOT
			While attempting to handle sectionalizer actions for the specified device, an error occurred.  Please
			try again and ensure all parameters are correct.  If the error persists, please submit your code and a
			bug report via the trac website.
			*/
		}

		//Error check
		if (recloser_count < 0)	//No recloser found, get us out of here
		{
			count_values = recloser_count;
			perform_operation = false;	//Flag as no change allowed
		}
		else	//Recloser found - pass the count out and flag a change allowed
		{
			perform_operation = true;
			count_values = recloser_count;
		}
	}
	else	//No check required for reconneciton
	{
		perform_operation = true;	//Flag operation as ok
		count_values = 1.0;			//Arbitrary non-zero value so fail check doesn't go off
	}

	if (perform_operation)	//Either is a "replace" or a recloser was found - operation is a go
	{
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
	}

	return count_values;
}

//Reliability interface - calls underlying switch routine
EXPORT int sectionalizer_reliability_operation(OBJECT *thisobj, unsigned char desired_phases)
{
	//Map the switch
	switch_object *swtchobj = OBJECTDATA(thisobj,switch_object);

	swtchobj->set_switch_full_reliability(desired_phases);

	return 1;	//This will always succeed...because I say so!
}

EXPORT int sectionalizer_fault_updates(OBJECT *thisobj, unsigned char restoration_phases)
{
	//Link to ourselves
	switch_object *thisswitch = OBJECTDATA(thisobj,switch_object);

	//Call the update
	thisswitch->set_switch_faulted_phases(restoration_phases);

	return 1;	//We magically always succeed
}
