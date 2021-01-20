// $Id$
// Copyright (C) 2021 Battelle Memorial Institute

#include "energy_storage.h"

using namespace std;

/* Framework */
CLASS *energy_storage::oclass = NULL;
energy_storage *energy_storage::defaults = NULL;

static PASSCONFIG passconfig = PC_BOTTOMUP;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
energy_storage::energy_storage(MODULE *module)
{
	if (oclass == NULL)
	{
		oclass = gl_register_class(module, "energy_storage", sizeof(energy_storage), passconfig | PC_AUTOLOCK);
		if (oclass == NULL)
			throw "unable to register class energy_storage";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//These are basically just placeholders - feel free to change them
			PT_double, "output_voltage[V]", PADDR(ES_DC_Voltage), PT_DESCRIPTION, "Energy storage DC output voltage",
			PT_double, "output_current[A]", PADDR(ES_DC_Current), PT_DESCRIPTION, "Energy storage DC output current",
			PT_double, "output_power", PADDR(ES_DC_Power_Val), PT_DESCRIPTION, "Energy storage DC output power",

			NULL) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		//Deltamode linkage
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_energy_storage) == NULL)
			GL_THROW("Unable to publish energy_storage deltamode function");
		if (gl_publish_function(oclass, "DC_gen_object_update", (FUNCTIONADDR)dc_object_update_energy_storage) == NULL)
			GL_THROW("Unable to publish energy_storage DC object update function");
	}
}

/* Object creation is called once for each object that is created by the core */
int energy_storage::create(void)
{
	//Initialize variables
	ES_DC_Voltage = 120.0;
	ES_DC_Current = 0.0;
	ES_DC_Power_Val = 0.0;

	//Deltamode flags
	deltamode_inclusive = false;
	first_sync_delta_enabled = false;

	//Inverter pointers
	inverter_voltage_property = NULL;
	inverter_current_property = NULL;
	inverter_power_property = NULL;

	//Default versions
	default_voltage_array = 0.0;
	default_current_array = 0.0;
	default_power_array = 0.0;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int energy_storage::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	FUNCTIONADDR temp_fxn;
	STATUS fxn_return_status;

	if (parent != NULL)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("energy_storage::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	// find parent inverter, if not defined, use a default voltage
	if (parent != NULL)
	{
		if (gl_object_isa(parent, "inverter", "generators") == true) // energy_storage has a PARENT and PARENT is an INVERTER - old-school inverter
		{
			//Throw an error for now - we'll have to think if we want to support old school inverters or not
			GL_THROW("energy_storage:%d - %s - Only inverter_dyn objects are supported at this time", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The energy_storage object only supports connections to inverter_dyn right now.  Older inverter functionality may be added in the future.
			*/
		}
		else if (gl_object_isa(parent, "inverter_dyn", "generators") == true) // energy_storage has a PARENT and PARENT is an inverter_dyn object
		{
			//Map the inverter voltage
			inverter_voltage_property = new gld_property(parent, "V_In");

			//Check it
			if ((inverter_voltage_property->is_valid() != true) || (inverter_voltage_property->is_double() != true))
			{
				GL_THROW("energy_storage:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				While attempting to map to one of the inverter interface variables, an error occurred.  Please try again.
				If the error persists, please submit a bug report and your model file via the issue tracking system.
				*/
			}

			//Map the inverter current
			inverter_current_property = new gld_property(parent, "I_In");

			//Check it
			if ((inverter_current_property->is_valid() != true) || (inverter_current_property->is_double() != true))
			{
				GL_THROW("energy_storage:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter power value
			inverter_power_property = new gld_property(parent, "P_In");

			//Check it
			if ((inverter_power_property->is_valid() != true) || (inverter_power_property->is_double() != true))
			{
				GL_THROW("energy_storage:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//"Register" us with the inverter

			//Map the function
			temp_fxn = (FUNCTIONADDR)(gl_get_function(parent, "register_gen_DC_object"));

			//See if it was located
			if (temp_fxn == NULL)
			{
				GL_THROW("energy_storage:%d - %s - failed to map additional current injection mapping for inverter_dyn:%d - %s", obj->id, (obj->name ? obj->name : "unnamed"), parent->id, (parent->name ? parent->name : "unnamed"));
				/*  TROUBLESHOOT
				While attempting to map the DC interfacing function for the energy_storage object, an error was encountered.
				Please try again.  If the error persists, please submit your code and a bug report via the issues tracker.
				*/
			}

			//Call the mapping function
			fxn_return_status = ((STATUS(*)(OBJECT *, OBJECT *))(*temp_fxn))(obj->parent, obj);

			//Make sure it worked
			if (fxn_return_status != SUCCESS)
			{
				GL_THROW("energy_storage:%d - %s - failed to map additional current injection mapping for inverter_dyn:%d - %s", obj->id, (obj->name ? obj->name : "unnamed"), parent->id, (parent->name ? parent->name : "unnamed"));
				//Defined above
			}
		}	 //End inverter_dyn
		else //It's not an inverter - fail it.
		{
			GL_THROW("energy_storage panel can only have an inverter as its parent.");
			/* TROUBLESHOOT
			The energy_storage panel can only have an INVERTER as parent, and no other object. Or it can be all by itself, without a parent.
			*/
		}
	}
	else //No parent
	{	 // default values of voltage
		gl_warning("energy_storage panel:%d has no parent defined. Using static voltages.", obj->id);

		//Map the values to ourself

		//Map the inverter voltage
		inverter_voltage_property = new gld_property(obj, "default_voltage_variable");

		//Check it
		if ((inverter_voltage_property->is_valid() != true) || (inverter_voltage_property->is_double() != true))
		{
			GL_THROW("energy_storage:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map to one of the default power interface variables, an error occurred.  Please try again.
			If the error persists, please submit a bug report and your model file via the issue tracking system.
			*/
		}

		//Map the inverter current
		inverter_current_property = new gld_property(obj, "default_current_variable");

		//Check it
		if ((inverter_current_property->is_valid() != true) || (inverter_current_property->is_double() != true))
		{
			GL_THROW("energy_storage:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Map the inverter power
		inverter_power_property = new gld_property(obj, "default_power_variable");

		//Check it
		if ((inverter_power_property->is_valid() != true) || (inverter_power_property->is_double() != true))
		{
			GL_THROW("energy_storage:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Set the local voltage value
		default_voltage_array = default_line_voltage;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true; //Set the flag and off we go
	}

	if (deltamode_inclusive == true)
	{
		//Check global, for giggles
		if (enable_subsecond_models != true)
		{
			gl_warning("energy_storage:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The energy_storage object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++; //Increment the counter
			first_sync_delta_enabled = true;
		}

		//Make sure our parent is an inverter_dyn and deltamode enabled (otherwise this is dumb)
		if (gl_object_isa(parent, "inverter_dyn", "generators") == true)
		{
			//Make sure our parent has the flag set
			if ((parent->flags & OF_DELTAMODE) != OF_DELTAMODE)
			{
				GL_THROW("energy_storage:%d %s is attached to an inverter_dyn, but that inverter_dyn is not set up for deltamode", obj->id, (obj->name ? obj->name : "Unnamed"));
				/*  TROUBLESHOOT
				The energy_storage object is not parented to a deltamode-enabled inverter_dyn.  There is really no reason to have the energy_storage object deltamode-enabled,
				so this should be fixed.
				*/
			}
			//Default else, all is well
		} //Not a proper parent
		else
		{
			GL_THROW("energy_storage:%d %s is not parented to an inverter -- deltamode operations do not support this", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The energy_storage object is not parented to an inverter.  Deltamode only supports it being parented to a deltamode-enabled inverter.
			*/
		}
	}	 //End deltamode inclusive
	else //This particular model isn't enabled
	{
		if (enable_subsecond_models == true)
		{
			gl_warning("energy_storage:%d %s - Deltamode is enabled for the module, but not this energy_storage array!", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The energy_storage array is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this array may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	return SUCCESS; //Unless something could fail it
}

TIMESTAMP energy_storage::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj = OBJECTHDR(this);

	if (first_sync_delta_enabled == true) //Deltamode first pass
	{
		//TODO: LOCKING!
		if ((deltamode_inclusive == true) && (enable_subsecond_models == true)) //We want deltamode - see if it's populated yet
		{
			if ((gen_object_current == -1) || (delta_objects == NULL))
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current >= gen_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the generators module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the generator
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[gen_object_current] = obj;

			//Map up the function for interupdate
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj, "interupdate_gen_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s", obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			post_delta_functions[gen_object_current] = NULL; //No post-update function for us

			//Map up the function for preupdate
			delta_preupdate_functions[gen_object_current] = NULL;	//Not one of these either

			//Update pointer
			gen_object_current++;

			//Flag us as complete
			first_sync_delta_enabled = false;
		}	 //End deltamode specials - first pass
		else //Somehow, we got here and deltamode isn't properly enabled...odd, just deflag us
		{
			first_sync_delta_enabled = false;
		}
	} //End first delta timestep
	//default else - either not deltamode, or not the first timestep

	//******* QSTS Updates might go here - this may also be called by deltamode *******//

	return TS_NEVER;
}

//Deltamode interupdate function
//Module-level call
SIMULATIONMODE energy_storage::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltatimedbl, currentDBLtime;
	TIMESTAMP time_passin_value;

	//Get timestep value
	deltat = (double)dt / (double)DT_SECOND;

	if (iteration_count_val == 0) //Only update timestamp tracker on first iteration
	{
		//Get decimal timestamp value
		deltatimedbl = (double)delta_time / (double)DT_SECOND;

		//Update tracking variable
		currentDBLtime = (double)gl_globalclock + deltatimedbl;

		//Cast it back
		time_passin_value = (TIMESTAMP)currentDBLtime;

		//******* Some initialization routine may go here *******//
	}
	//Update is called by the inverter, so it doesn't need to do anything in here - it's all in the sub-function

	//energy_storage object never drives anything, so it's always ready to leave
	return SM_EVENT;
}

//DC update function
STATUS energy_storage::energy_storage_dc_update(OBJECT *calling_obj, bool init_mode)
{
	gld_wlock *test_rlock;
	STATUS temp_status = SUCCESS;
	double inv_I, inv_P;

	if (init_mode == true) //Initialization - if needed
	{
		//Pull P_In from the inverter - for now, this is singular (may need to be adjusted when multiple objects exist)
		inv_P = inverter_power_property->get_double();

		//******* Some initialization routine may go here *******//

		//Push the voltage back out to the inverter - this may need different logic when there are multiple objects
		//******* Just an example - may not be needed *******//
		inverter_voltage_property->setp<double>(ES_DC_Voltage, *test_rlock);
	}
	else //Standard runs
	{
		//Pull the values from the inverter - these may not all be needed - put here as examples
		ES_DC_Voltage = inverter_voltage_property->get_double();
		inv_I = inverter_current_property->get_double();
		inv_P = inverter_power_property->get_double();

		//******* Energy storage update equations would go here *******//

		//Push the changes
		inverter_current_property->setp<double>(inv_I, *test_rlock);
		inverter_power_property->setp<double>(inv_P, *test_rlock);
	}

	//Return our status
	return temp_status;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_energy_storage(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(energy_storage::oclass);
		if (*obj != NULL)
		{
			energy_storage *my = OBJECTDATA(*obj, energy_storage);
			gl_set_parent(*obj, parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(energy_storage);
}

EXPORT int init_energy_storage(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj != NULL)
			return OBJECTDATA(obj, energy_storage)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(energy_storage);
}

EXPORT TIMESTAMP sync_energy_storage(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	energy_storage *my = OBJECTDATA(obj, energy_storage);
	try
	{
		switch (pass)
		{
		case PC_PRETOPDOWN:
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass == clockpass)
			obj->clock = t1;
	}
	SYNC_CATCHALL(energy_storage);
	return t2;
}

//DELTAMODE Linkage
EXPORT SIMULATIONMODE interupdate_energy_storage(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	energy_storage *my = OBJECTDATA(obj, energy_storage);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_energy_storage(obj=%d;%s): %s", obj->id, obj->name ? obj->name : "unnamed", msg);
		return status;
	}
}

//DC Object calls from inverter linkage
EXPORT STATUS dc_object_update_energy_storage(OBJECT *us_obj, OBJECT *calling_obj, bool init_mode)
{
	energy_storage *me_energy_storage = OBJECTDATA(us_obj, energy_storage);
	STATUS temp_status;

	//Call our update function
	temp_status = me_energy_storage->energy_storage_dc_update(calling_obj, init_mode);

	//Return this value
	return temp_status;
}
