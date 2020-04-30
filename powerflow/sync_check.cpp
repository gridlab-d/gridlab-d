/** $Id: sync_check

    Implements sychronization check functionality for switches to close
    when two grids are within parameters

	Copyright (C) 2020 Battelle Memorial Institute
**/

#include "sync_check.h"

#include <iostream>

using namespace std;

//////////////////////////////////////////////////////////////////////////
// sync_check CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS *sync_check::oclass = NULL;
CLASS *sync_check::pclass = NULL;

sync_check::sync_check(MODULE *mod) : powerflow_object(mod)
{
	if (oclass == NULL)
	{
		pclass = powerflow_object::oclass;
		oclass = gl_register_class(mod, "sync_check", sizeof(sync_check), PC_PRETOPDOWN | PC_BOTTOMUP | PC_POSTTOPDOWN | PC_AUTOLOCK);
		if (oclass == NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);
		if (gl_publish_variable(oclass,
								PT_bool, "enabled", PADDR(arm_sync), PT_DESCRIPTION, "Flag to arm the synchronization close",
								PT_double, "frequency_tolerance[pu]", PADDR(frequency_tolerance_pu), PT_DESCRIPTION, "The user-specified tolerance for checking the frequency metric",
								PT_double, "voltage_tolerance[pu]", PADDR(voltage_tolerance_pu), PT_DESCRIPTION, "voltage_tolerance",
								PT_double, "metrics_period[s]", PADDR(metrics_period_sec), PT_DESCRIPTION, "The user-defined period when both metrics are satisfied",
								NULL) < 1)
			GL_THROW("unable to publish properties in %s", __FILE__);

		if (gl_publish_function(oclass, "interupdate_pwr_object", (FUNCTIONADDR)interupdate_sync_check) == NULL)
			GL_THROW("Unable to publish sync_check deltamode function");
	}
}

int sync_check::isa(char *classname)
{
	return strcmp(classname, "sync_check") == 0;
}

int sync_check::create(void)
{
	int result = powerflow_object::create();

	/* init member with default values */
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	reg_dm_flag = false;

	/* init some published properties that have the default value */
	arm_sync = false; //Start disabled

	return result;
}

int sync_check::init(OBJECT *parent)
{
	int retval = powerflow_object::init(parent);

	data_sanity_check(parent);
	reg_deltamode();
	init_sensors();

	return retval;
}

TIMESTAMP sync_check::presync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::presync(t0);

	// Check if this object needs to be registered for running in deltamode
	if (reg_dm_flag)
	{
		// Turn off this one-time flag
		reg_dm_flag = false;

		//Check limits first
		if (pwr_object_current >= pwr_object_count)
		{
			GL_THROW("Too many objects tried to populate deltamode objects array in the powerflow module!");
			/*  TROUBLESHOOT
			While attempting to populate a reference array of deltamode-enabled objects for the powerflow
			module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
			error persists, please submit a bug report and your code via the trac website.
			*/
		}

		// Add this object into the list of deltamode objects
		delta_objects[pwr_object_current] = obj;

		// Map up the function
		delta_functions[pwr_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_pwr_object"));

		// Dobule check the mapped function
		if (delta_functions[pwr_object_current] == NULL)
		{
			gl_warning("Failure to map deltamode function for this device: %s", obj->name);
			/*  TROUBLESHOOT
			Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
			the object supports deltamode.  This error may simply be an indication that the object of interest
			does not support deltamode.  If the error persists and the object does, please submit your code and
			a bug report via the trac website.
			*/
		}

		//Increment
		pwr_object_current++;
	}

	return tret;
}

TIMESTAMP sync_check::sync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::sync(t0);

	return tret;
}

TIMESTAMP sync_check::postsync(TIMESTAMP t0)
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP tret = powerflow_object::postsync(t0);

	return tret;
}

//==Funcs
bool sync_check::data_sanity_check(OBJECT *par)
{
	bool flag_prop_modified = false;
	OBJECT *obj = OBJECTHDR(this);

	// Check if the parent is a switch_object object
	if (par == NULL)
	{
		GL_THROW("sync_check (%d - %s): the partent property must be specified!", obj->id, (obj->name ? obj->name : "Unnamed"));
	}
	else
	{
		if (gl_object_isa(par, "switch", "powerflow") == false)
		{
			GL_THROW("sync_check (%d - %s): the partent property must be specified as the name of a switch_object object!", obj->id, (obj->name ? obj->name : "Unnamed"));
		}
	}

	// Check the params
	if (frequency_tolerance_pu <= 0)
	{
		frequency_tolerance_pu = 1e-2; // i.e., 1%
		gl_warning("sync_check:%d %s - frequency_tolerance_pu was not set as a positive value, it is reset to %f [pu].", obj->id, (obj->name ? obj->name : "Unnamed"), frequency_tolerance_pu);
		flag_prop_modified = true;
	}

	if (voltage_tolerance_pu <= 0)
	{
		voltage_tolerance_pu = 1e-2; // i.e., 1%
		gl_warning("sync_check:%d %s - voltage_tolerance_pu was not set as a positive value, it is reset to %f [pu].", obj->id, (obj->name ? obj->name : "Unnamed"), voltage_tolerance_pu);
		flag_prop_modified = true;
	}

	if (metrics_period_sec <= 0)
	{
		metrics_period_sec = 5; // i.e., 5 secs
		gl_warning("sync_check:%d %s - metrics_period_sec was not set as a positive value, it is reset to %f [secs].", obj->id, (obj->name ? obj->name : "Unnamed"), metrics_period_sec);
		flag_prop_modified = true;
	}
}

void sync_check::reg_deltamode()
{
	OBJECT *obj = OBJECTHDR(this);

	// Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	// Check the module deltamode flag & the object deltamode flag for consistency
	if (enable_subsecond_models)
	{
		if (deltamode_inclusive != true)
		{
			gl_warning("sync_check:%d %s - Deltamode is enabled for the powerflow module, but not this sync_check object!",
					   obj->id, (obj->name ? obj->name : "Unnamed"));
		}
		else // Both the powerflow module and object are enabled for the deltamode
		{
			pwr_object_count++;
			reg_dm_flag = true;
		}
	}
	else
	{
		if (deltamode_inclusive == true)
		{
			gl_warning("sync_check:%d %s - Deltamode is enabled for the sync_check object, but not this powerflow module!",
					   obj->id, (obj->name ? obj->name : "Unnamed"));
		}
	}
}

void sync_check::init_sensors()
{
}

void sync_check::get_measurements()
{
	cout << "Luan~~";
}

//Deltamode call
//Module-level call
SIMULATIONMODE sync_check::inter_deltaupdate_sync_check(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	get_measurements();

	return SM_EVENT;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: sync_check
//////////////////////////////////////////////////////////////////////////

/**
* REQUIRED: allocate and initialize an object.
*
* @param obj a pointer to a pointer of the last object in the list
* @param parent a pointer to the parent of this object
* @return 1 for a successfully created object, 0 for error
*/
EXPORT int create_sync_check(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(sync_check::oclass);
		if (*obj != NULL)
		{
			sync_check *my = OBJECTDATA(*obj, sync_check);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(sync_check);
}

EXPORT int init_sync_check(OBJECT *obj)
{
	try
	{
		sync_check *my = OBJECTDATA(obj, sync_check);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(sync_check);
}

/**
* Sync is called when the clock needs to advance on the bottom-up pass (PC_BOTTOMUP)
*
* @param obj the object we are sync'ing
* @param t0 this objects current timestamp
* @param pass the current pass for this sync call
* @return t1, where t1>t0 on success, t1=t0 for retry, t1<t0 on failure
*/
EXPORT TIMESTAMP sync_sync_check(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try
	{
		sync_check *pObj = OBJECTDATA(obj, sync_check);
		TIMESTAMP t1 = TS_NEVER;
		switch (pass)
		{
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
	SYNC_CATCHALL(sync_check);
}

EXPORT int isa_sync_check(OBJECT *obj, char *classname)
{
	return OBJECTDATA(obj, sync_check)->isa(classname);
}

//Deltamode export
EXPORT SIMULATIONMODE interupdate_sync_check(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	sync_check *my = OBJECTDATA(obj, sync_check);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate_sync_check(delta_time, dt, iteration_count_val, interupdate_pos);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_sync_check(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

/**@}**/
