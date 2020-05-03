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
								PT_bool, "armed", PADDR(sc_enabled_flag), PT_DESCRIPTION, "Flag to arm the synchronization close",
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
	init_vars();
	return result;
}

int sync_check::init(OBJECT *parent)
{
	int retval = powerflow_object::init(parent);

	data_sanity_check(parent);
	init_norm_values(parent);
	reg_deltamode_check();

	return retval;
}

TIMESTAMP sync_check::presync(TIMESTAMP t0)
{
	TIMESTAMP tret = powerflow_object::presync(t0);

	reg_deltamode();

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
void sync_check::init_vars()
{
	/* init member with default values */
	deltamode_inclusive = false; //By default, don't be included in deltamode simulations
	reg_dm_flag = false;
	metrics_flag = false;
	temp_property_pointer = NULL;

	/* init some published properties that have the default value */
	sc_enabled_flag = false; //Unarmed

	/* init measurements, gld objs, & Nominal Values*/
	freq_norm = 0;
	volt_norm = 0;

	swt_fm_node = NULL;
	swt_to_node = NULL;

	swt_fm_node_freq = 0;
	swt_to_node_freq = 0;

	swt_prop_status = NULL;
}

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

	// Check the status of the 'switch_object' object (when it is armed, the partent switch should be in 'OPEN' status)
	swt_prop_status = new gld_property(par, "status");

	// Double check the validity of the nominal frequency property
	if ((swt_prop_status->is_valid() != true) || (swt_prop_status->is_enumeration() != true))
	{
		GL_THROW("sync_check:%d %s failed to map the swtich status property",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the swtich status property, an error occurred.  Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
	}

	if (sc_enabled_flag)
	{
		enumeration swt_init_status = swt_prop_status->get_enumeration();
		if (swt_init_status != LS_OPEN) //@TODO: get the Enum definitation from switch_object may be better, while not convenient? The LS_OPEN seems to be decoupled from the enum defined in the switch, while they are defined in the same way separately
		{
			GL_THROW("sync_check (%d - %s): the partent switch_object object must be OPEN when the sync_check is initiated as armed!",
					 obj->id, (obj->name ? obj->name : "Unnamed"));
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
		metrics_period_sec = 1.2; // i.e., 1.2 secs
		gl_warning("sync_check:%d %s - metrics_period_sec was not set as a positive value, it is reset to %f [secs].", obj->id, (obj->name ? obj->name : "Unnamed"), metrics_period_sec);
		flag_prop_modified = true;
	}

	return flag_prop_modified;
}

void sync_check::reg_deltamode_check()
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

void sync_check::reg_deltamode()
{
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

		// Get the self-pointer
		OBJECT *obj = OBJECTHDR(this);

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

		// Set the post delta function to NULL, thus it does not need to be checked
		post_delta_functions[pwr_object_current] = NULL;

		//Increment
		pwr_object_current++;
	}
}

void sync_check::init_norm_values(OBJECT *par)
{
	// Get the self-pointer
	OBJECT *obj = OBJECTHDR(this);

	/* Get the nominal frequency property */
	temp_property_pointer = new gld_property("powerflow::nominal_frequency");

	// Double check the validity of the nominal frequency property
	if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_double() != true))
	{
		GL_THROW("sync_check:%d %s failed to map the nominal_frequency property", obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the nominal_frequency property, an error occurred.  Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
	}

	// Get the value of nominal frequency from this property
	freq_norm = temp_property_pointer->get_double();

	// Clean the temporary property pointer
	delete temp_property_pointer;

	/* Get the from node */
	temp_property_pointer = new gld_property(par, "from");
	// Double check the validity
	if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_objectref() != true))
	{
		GL_THROW("sync_check:%d %s Failed to map the switch property 'from'!",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
        While attempting to map the a property from the switch object, an error occurred.  Please try again.
        If the error persists, please submit your GLM and a bug report to the ticketing system.
        */
	}

	swt_fm_node = temp_property_pointer->get_objectref();
	delete temp_property_pointer;

	/* Get the to node */
	temp_property_pointer = new gld_property(par, "to");
	// Double check the validity
	if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_objectref() != true))
	{
		GL_THROW("sync_check:%d %s Failed to map the switch property 'to'!",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
        While attempting to map the a property from the switch object, an error occurred.  Please try again.
        If the error persists, please submit your GLM and a bug report to the ticketing system.
        */
	}

	swt_to_node = temp_property_pointer->get_objectref();
	delete temp_property_pointer;

	/* Get the norminal voltage & check the consistency */
	// 'From' node voltage
	temp_property_pointer = new gld_property(swt_fm_node, "nominal_voltage");
	// Double check the validity of the nominal frequency property
	if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_double() != true))
	{
		GL_THROW("sync_check:%d %s failed to map the nominal_voltage property",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the nominal_voltage property, an error occurred.  Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
	}
	double volt_norm_fm = temp_property_pointer->get_double();
	delete temp_property_pointer;

	// 'To' node voltage
	temp_property_pointer = new gld_property(swt_to_node, "nominal_voltage");
	// Double check the validity of the nominal frequency property
	if ((temp_property_pointer->is_valid() != true) || (temp_property_pointer->is_double() != true))
	{
		GL_THROW("sync_check:%d %s failed to map the nominal_voltage property",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map the nominal_voltage property, an error occurred.  Please try again.
		If the error persists, please submit your GLM and a bug report to the ticketing system.
		*/
	}
	double volt_norm_to = temp_property_pointer->get_double();
	delete temp_property_pointer;

	// Check consistency
	if (abs(volt_norm_fm - volt_norm_to) > voltage_tolerance_pu)
	{
		GL_THROW("sync_check:%d %s the normal voltage check fails!",
				 obj->id, (obj->name ? obj->name : "Unnamed"));
	}
	else
	{
		volt_norm = (volt_norm_fm + volt_norm_to) / 2;
	}
}

void sync_check::update_measurements()
{
	//@Todo: 1) validity check of properties; 2) frequency of each phase?

	/* Get the 'from' node freq */
	temp_property_pointer = new gld_property(swt_fm_node, "measured_frequency");
	swt_fm_node_freq = temp_property_pointer->get_double();
	delete temp_property_pointer;

	/* Get the 'to' node freq */
	temp_property_pointer = new gld_property(swt_to_node, "measured_frequency");
	swt_to_node_freq = temp_property_pointer->get_double();
	delete temp_property_pointer;

	/* Get the 'from' node volt phasors */
	temp_property_pointer = new gld_property(swt_fm_node, "voltage_A");
	swt_fm_volt_A = temp_property_pointer->get_complex();
	delete temp_property_pointer;

	/* Get the 'to' node volt phasors */
	temp_property_pointer = new gld_property(swt_to_node, "voltage_A");
	swt_to_volt_A = temp_property_pointer->get_complex();
	delete temp_property_pointer;
}

void sync_check::check_metrics()
{
	double freq_diff = abs(swt_fm_node_freq - swt_to_node_freq);
	double freq_diff_pu = freq_diff / freq_norm;

	double volt_A_diff = (swt_fm_volt_A - swt_to_volt_A).Mag();
	double volt_A_diff_pu = volt_A_diff / volt_norm;

	if ((freq_diff_pu <= frequency_tolerance_pu) && (volt_A_diff_pu <= voltage_tolerance_pu))
	{
		metrics_flag = true;
	}
	else
	{
		metrics_flag = false;
	}
}

void sync_check::check_excitation(unsigned long dt)
{
	double dt_dm_sec = (double)dt / (double)DT_SECOND;

	if (metrics_flag)
	{
		t_sat += dt_dm_sec;
	}
	else
	{
		t_sat = 0;
	}

	if (t_sat >= metrics_period_sec)
	{
		gld_wlock *test_rlock;
		enumeration swt_cmd = LS_CLOSED;
		swt_prop_status->setp<enumeration>(swt_cmd, *test_rlock); // Close the switch for parallelling
		reset_after_excitation();
	}
}

void sync_check::reset_after_excitation()
{
	// After closing the swtich, disable itself
	sc_enabled_flag = false; //Unarmed

	// Reset other buffers & variables
	metrics_flag = false;
	t_sat = 0;
}

//Deltamode call
//Module-level call
SIMULATIONMODE sync_check::inter_deltaupdate_sync_check(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos)
{
	if (sc_enabled_flag)
	{
		if ((iteration_count_val == 0) && (!interupdate_pos)) //@TODO: Need further testings
		{
			update_measurements();
			check_metrics();
			check_excitation(dt);
		}
	}

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
