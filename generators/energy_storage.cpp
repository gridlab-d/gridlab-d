// $Id$
// Copyright (C) 2021 Battelle Memorial Institute

#include "energy_storage.h"
#include <cmath>

using namespace std;

/* Framework */
CLASS *energy_storage::oclass = nullptr;
energy_storage *energy_storage::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
energy_storage::energy_storage(MODULE *module)
{
	if (oclass == nullptr)
	{
		oclass = gl_register_class(module, "energy_storage", sizeof(energy_storage), passconfig | PC_AUTOLOCK);
		if (oclass == nullptr)
			throw "unable to register class energy_storage";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			//These are basically just placeholders - feel free to change them
			PT_double, "output_voltage[V]", PADDR(ES_DC_Voltage), PT_DESCRIPTION, "Energy storage DC output voltage",
			PT_double, "output_current[A]", PADDR(ES_DC_Current), PT_DESCRIPTION, "Energy storage DC output current",
			PT_double, "output_power[W]", PADDR(ES_DC_Power_Val), PT_DESCRIPTION, "Energy storage DC output power",

			PT_double, "Vbase_ES[V]", PADDR(Vbase_ES), PT_DESCRIPTION, "Rated voltage of ES",
			PT_double, "Sbase_ES[VA]", PADDR(Sbase_ES), PT_DESCRIPTION, "Rated rating of ES",
			PT_double, "Qbase_ES[Wh]", PADDR(Qbase_ES), PT_DESCRIPTION, "Rated capacity of ES, unit Wh",
			PT_double, "SOC_0_ES[pu]", PADDR(SOC_0_ES), PT_DESCRIPTION, "Initial state of charge",
			PT_double, "SOC_ES[pu]", PADDR(SOC_ES), PT_DESCRIPTION, "State of charge",
			PT_double, "Q_ES[Wh]", PADDR(Q_ES), PT_DESCRIPTION, "Energy of ES",

			PT_double, "E_ES_pu[pu]", PADDR(E_ES_pu), PT_DESCRIPTION, "No-load voltage, per unit",
			PT_double, "E0_ES_pu[pu]", PADDR(E0_ES_pu), PT_DESCRIPTION, "Battery constant voltage, per unit",
			PT_double, "K_ES_pu[pu]", PADDR(K_ES_pu), PT_DESCRIPTION, "Polarisation voltage, per unit",
			PT_double, "A_ES_pu[pu]", PADDR(A_ES_pu), PT_DESCRIPTION, "Exponential zone amplitude, per unit",
			PT_double, "B_ES_pu[pu]", PADDR(B_ES_pu), PT_DESCRIPTION, "Exponential zone time constant inverse, per unit",

			PT_double, "V_ES_pu[pu]", PADDR(V_ES_pu), PT_DESCRIPTION, "ES terminal voltage, per unit",
			PT_double, "I_ES_pu[pu]", PADDR(I_ES_pu), PT_DESCRIPTION, "ES output current, per unit",
			PT_double, "R_ES_pu[pu]", PADDR(R_ES_pu), PT_DESCRIPTION, "ES internal resitance, per unit",
			PT_double, "P_ES_pu[pu]", PADDR(P_ES_pu), PT_DESCRIPTION, "ES output active power, per unit",


			NULL) < 1)
				GL_THROW("unable to publish properties in %s", __FILE__);

		//Deltamode linkage
		if (gl_publish_function(oclass, "interupdate_gen_object", (FUNCTIONADDR)interupdate_energy_storage) == nullptr)
			GL_THROW("Unable to publish energy_storage deltamode function");
		if (gl_publish_function(oclass, "DC_gen_object_update", (FUNCTIONADDR)dc_object_update_energy_storage) == nullptr)
			GL_THROW("Unable to publish energy_storage DC object update function");
	}
}

/* Object creation is called once for each object that is created by the core */
int energy_storage::create(void)
{
	//Initialize variables
	ES_DC_Voltage = 0.0;
	ES_DC_Current = 0.0;
	ES_DC_Power_Val = 0.0;

	SOC_0_ES = 1.0;  // Assume the ES is fully charged

	E0_ES_pu = 1.0374; // Based on a typical Li-on battery
	K_ES_pu = 0.0024333;  // Based on a typical Li-on battery
	A_ES_pu = 0.13; // Based on a typical Li-on battery
	B_ES_pu = 3.5294; //Based on a typical Li-on battery
	R_ES_pu = 0.025; //Based on a typical Li-on battery

	prev_time = 0.0; // Previous model update time


	//Deltamode flags
	deltamode_inclusive = false;

	//Inverter pointers
	inverter_voltage_property = nullptr;
	inverter_current_property = nullptr;
	inverter_power_property = nullptr;

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


	if (parent != nullptr)
	{
		if ((parent->flags & OF_INIT) != OF_INIT)
		{
			char objname[256];
			gl_verbose("energy_storage::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	prev_time = (double)gl_globalclock;

	SOC_ES = SOC_0_ES;  // Initialize the SOC_ES

	if ((SOC_ES < 0.0)||(SOC_ES > 1.0)) // energy_storage has a PARENT and PARENT is an INVERTER - old-school inverter
	{
		//Throw an error for now - we'll have to think if we want to support old school inverters or not
		GL_THROW("energy_storage:%d - %s - The battery SOC has to be between 0.0 and 1.0", obj->id, (obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		The battery SOC is specified as a per-unit value, not a percentage.
		*/
	}


	// find parent inverter, if not defined, use a default voltage
	if (parent != nullptr)
	{
		if (gl_object_isa(parent, "inverter", "generators")) // energy_storage has a PARENT and PARENT is an INVERTER - old-school inverter
		{
			//Throw an error for now - we'll have to think if we want to support old school inverters or not
			GL_THROW("energy_storage:%d - %s - Only inverter_dyn objects are supported at this time", obj->id, (obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The energy_storage object only supports connections to inverter_dyn right now.  Older inverter functionality may be added in the future.
			*/
		}
		else if (gl_object_isa(parent, "inverter_dyn", "generators")) // energy_storage has a PARENT and PARENT is an inverter_dyn object
		{
			//Map the inverter voltage
			inverter_voltage_property = new gld_property(parent, "V_In");

			//Check it
			if (!inverter_voltage_property->is_valid() || !inverter_voltage_property->is_double())
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
			if (!inverter_current_property->is_valid() || !inverter_current_property->is_double())
			{
				GL_THROW("energy_storage:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//Map the inverter power value
			inverter_power_property = new gld_property(parent, "P_In");

			//Check it
			if (!inverter_power_property->is_valid() || !inverter_power_property->is_double())
			{
				GL_THROW("energy_storage:%d - %s - Unable to map inverter power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
				//Defined above
			}

			//"Register" us with the inverter

			//Map the function
			temp_fxn = (FUNCTIONADDR)(gl_get_function(parent, "register_gen_DC_object"));

			//See if it was located
			if (temp_fxn == nullptr)
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
		if (!inverter_voltage_property->is_valid() || !inverter_voltage_property->is_double())
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
		if (!inverter_current_property->is_valid() || !inverter_current_property->is_double())
		{
			GL_THROW("energy_storage:%d - %s - Unable to map a default power interface field", obj->id, (obj->name ? obj->name : "Unnamed"));
			//Defined above
		}

		//Map the inverter power
		inverter_power_property = new gld_property(obj, "default_power_variable");

		//Check it
		if (!inverter_power_property->is_valid() || !inverter_power_property->is_double())
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

	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("energy_storage:%s indicates it wants to run deltamode, but the module-level flag is not set!", obj->name ? obj->name : "unnamed");
			/*  TROUBLESHOOT
			The energy_storage object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			//Add us to the list
			fxn_return_status = add_gen_delta_obj(obj,false);

			//Check it
			if (fxn_return_status == FAILED)
			{
				GL_THROW("energy_storage:%s - failed to add object to generator deltamode object list", obj->name ? obj->name : "unnamed");
				/*  TROUBLESHOOT
				The energy_storage object encountered an issue while trying to add itself to the generator deltamode object list.  If the error
				persists, please submit an issue via GitHub.
				*/
			}
		}

		//Make sure our parent is an inverter_dyn and deltamode enabled (otherwise this is dumb)
		if (gl_object_isa(parent, "inverter_dyn", "generators"))
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
		if (enable_subsecond_models)
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
	double curr_time;
	double deltat;

	OBJECT *obj = OBJECTHDR(this);

	//******* QSTS Updates might go here - this may also be called by deltamode *******//

	curr_time = (double)t1;
	if (curr_time > prev_time)
	{
		deltat = curr_time - prev_time;

		ES_DC_Current = inverter_current_property->get_double(); // Get the updated current
		SOC_ES = SOC_ES - (ES_DC_Current * deltat / 3600.00) / (Qbase_ES/Vbase_ES);  // Calculate the SOC, second to hour conversion (3600)

		prev_time = curr_time;

	}



	return TS_NEVER;
}

//Deltamode interupdate function
//Module-level call
SIMULATIONMODE energy_storage::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat, deltatimedbl, currentDBLtime;


	if (iteration_count_val == 0) //Only update timestamp tracker on first iteration
	{
		//Get decimal timestamp value
		deltatimedbl = (double)delta_time / (double)DT_SECOND;

		//Update tracking variable
		currentDBLtime = (double)gl_globalclock + deltatimedbl;

		deltat = currentDBLtime - prev_time;

		//******* Some initialization routine may go here *******//
		ES_DC_Current = inverter_current_property->get_double(); // Get the updated current
		SOC_ES = SOC_ES - (ES_DC_Current * deltat / 3600.00) / (Qbase_ES/Vbase_ES);  // Calculate the SOC, second to hour conversion (3600)

		prev_time = currentDBLtime;

	}
	//Update is called by the inverter, so it doesn't need to do anything in here - it's all in the sub-function

	//energy_storage object never drives anything, so it's always ready to leave
	return SM_EVENT;
}

//DC update function
STATUS energy_storage::energy_storage_dc_update(OBJECT *calling_obj, bool init_mode)
{
	gld_wlock *test_rlock = nullptr;
	STATUS temp_status = SUCCESS;
	//double  P_ES_pu;


	if (init_mode) //Initialization - if needed
	{
		//Pull P_In from the inverter - for now, this is singular (may need to be adjusted when multiple objects exist)
		ES_DC_Power_Val = inverter_power_property->get_double();
		P_ES_pu = ES_DC_Power_Val/Sbase_ES;


		//******* Initialize the ES output voltage and current *******//

		E_ES_pu = E0_ES_pu - K_ES_pu/SOC_0_ES + A_ES_pu * exp(-B_ES_pu * (1.0-SOC_0_ES));  // Calculate the battery internal votlage
		V_ES_pu = (E_ES_pu + sqrt(E_ES_pu*E_ES_pu - 4*P_ES_pu*R_ES_pu))/2.0;  // Calculate the terminal voltage
		I_ES_pu = P_ES_pu/V_ES_pu;  //Calculate the output current

		ES_DC_Voltage = V_ES_pu * Vbase_ES;
		ES_DC_Current = I_ES_pu * (Sbase_ES / Vbase_ES);


		//Push the voltage back out to the inverter - this may need different logic when there are multiple objects
		//******* Just an example - may not be needed *******//
		inverter_voltage_property->setp<double>(ES_DC_Voltage, *test_rlock);
		inverter_current_property->setp<double>(ES_DC_Current, *test_rlock);

	}
	else //Standard runs
	{
		//Pull the values from the inverter - these may not all be needed - put here as examples
		ES_DC_Voltage = inverter_voltage_property->get_double();
		//ES_DC_Current = inverter_current_property->get_double();
		//inv_P = inverter_power_property->get_double();

		//******* Energy storage update equations would go here *******//

		V_ES_pu = ES_DC_Voltage / Vbase_ES;

		E_ES_pu = E0_ES_pu - K_ES_pu/SOC_ES + A_ES_pu * exp(-B_ES_pu * (1.0-SOC_ES));  // Calculate the battery internal votlage

		I_ES_pu = (E_ES_pu - V_ES_pu)/R_ES_pu;  // Calculate the battery output current

		ES_DC_Current = I_ES_pu * (Sbase_ES/Vbase_ES);

		P_ES_pu = V_ES_pu * I_ES_pu;  // Calculate the battery output power


		//Push the changes
		inverter_current_property->setp<double>(ES_DC_Current, *test_rlock);
		//inverter_power_property->setp<double>(inv_P, *test_rlock);
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
		if (*obj != nullptr)
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
		if (obj != nullptr)
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
