/** $Id: thermal_storage.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file lights.cpp
	@addtogroup residential_lights Lights
	@ingroup residential

	The thermal storage unit is a model of an air conditioning unit that
	shifts some load from peak to off peak load times.  The model is based
	on the Ice Bear unit manufactured by Ice Energy.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "residential.h"
#include "house_e.h"
#include "thermal_storage.h"

//////////////////////////////////////////////////////////////////////////
// Default schedules - enacted if not driven externall
//////////////////////////////////////////////////////////////////////////
struct s_thermal_default_schedule_list {
	char *schedule_name;
	char *schedule_definition;
} thermal_default_schedule_list[] =
{
	{	"thermal_storage_discharge_default", 
		"dischargetime {"
		"* 0-10 * * * 0.0;"
		"* 11-20 * * * 1.0;"
		"* 21-23 * * * 0.0;"
		"}"
	},
	{	"thermal_storage_charge_default",
		"chargetime {"
		"* 0-10 * * * 1.0;"
		"* 11-20 * * * 0.0;"
		"* 21-23 * * * 1.0;"
		"}"
	}
};

//////////////////////////////////////////////////////////////////////////
// thermal_storage CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* thermal_storage::oclass = NULL;
CLASS* thermal_storage::pclass = NULL;

// the constructor registers the class and properties and sets the defaults
thermal_storage::thermal_storage(MODULE *mod)
: residential_enduse(mod)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;

		// register the class definition
		oclass = gl_register_class(mod,"thermal_storage",sizeof(thermal_storage),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register class thermal_storage");
			/* TROUBLESHOOT
				The file that implements the thermal_storage in the residential module cannot register the class.
				This is an internal error.  Contact support for assistance.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double, "total_capacity[Btu]", PADDR(total_capacity), PT_DESCRIPTION, "total storage capacity of unit",
			PT_double, "stored_capacity[Btu]", PADDR(stored_capacity), PT_DESCRIPTION, "amount of capacity that is stored",
			PT_double,"recharge_power[kW]",PADDR(recharge_power), PT_DESCRIPTION, "installed compressor power usage",
			PT_double,"discharge_power[kW]",PADDR(discharge_power), PT_DESCRIPTION, "installed pump power usage",
			PT_double,"recharge_pf",PADDR(recharge_power_factor), PT_DESCRIPTION, "installed compressor power factor",
			PT_double,"discharge_pf",PADDR(discharge_power_factor), PT_DESCRIPTION, "installed pump power factor",
			PT_enumeration, "discharge_schedule_type", PADDR(discharge_schedule_type),PT_DESCRIPTION,"Scheduling method for discharging",
				PT_KEYWORD, "INTERNAL", (enumeration)INTERNAL,
				PT_KEYWORD, "EXTERNAL", (enumeration)EXTERNAL,
			PT_enumeration, "recharge_schedule_type", PADDR(recharge_schedule_type),PT_DESCRIPTION,"Scheduling method for charging",
				PT_KEYWORD, "INTERNAL", (enumeration)INTERNAL,
				PT_KEYWORD, "EXTERNAL", (enumeration)EXTERNAL,
			PT_double, "recharge_time", PADDR(recharge_time), PT_DESCRIPTION, "Flag indicating if recharging is available at the current time (1 or 0)",		//schedule?
			PT_double, "discharge_time", PADDR(discharge_time), PT_DESCRIPTION, "Flag indicating if discharging is available at the current time (1 or 0)",			//schedule?
			PT_double, "discharge_rate[Btu/h]", PADDR(discharge_rate), PT_DESCRIPTION, "rating of discharge or cooling",
			PT_double, "SOC[%]", PADDR(state_of_charge), PT_DESCRIPTION, "state of charge as percentage of total capacity",		//storage/stored capacity
			PT_double, "k[W/m/K]", PADDR(k), PT_DESCRIPTION, "coefficient of thermal conductivity (W/m/K)",
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The file that implements the specified class cannot publish the variables in the class.
				This is an internal error.  Contact support for assistance.
			 */
	}
}

// create is called every time a new object is loaded
int thermal_storage::create(void)
{
	int res = residential_enduse::create();
	
	// name of enduse
	load.name = oclass->name;
	load.power = 0;
	load.power_factor = 1;
	load.heatgain = 0;

	total_capacity = 0;
	stored_capacity = -1;
	recharge_power = 0;
	discharge_power = 0;
	recharge_power_factor = 0;
	discharge_power_factor = 0;
	recharge_time = 0;
	discharge_time = 0;
	discharge_rate = 0;
	state_of_charge = -1;
	k = -1;

	//Pointers to house
	thermal_storage_available = NULL;
	thermal_storage_active = NULL;

	//Pointers for values
	recharge_time_ptr = NULL;
	discharge_time_ptr = NULL;

	//Pointers for schedules
	recharge_schedule_vals = NULL;
	discharge_schedule_vals = NULL;

	//Set scheduling type for internal initially
	discharge_schedule_type=INTERNAL;
	recharge_schedule_type=INTERNAL;

	return res;
}

int thermal_storage::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("thermal_storage::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;
	double *design_cooling_capacity;

	//Make sure the parent is a house
	if (!(gl_object_isa(parent,"house","residential")))
	{
		GL_THROW("thermal_storage:%s must be parented to a house!",hdr->name);
		/*  TROUBLESHOOT
		The thermal_storage model is only valid for house objects.  Please parent it appropriately.
		*/
	}

	//Pull a house link, we'll use it for addresses
	house_e *house_lnk = OBJECTDATA(parent,house_e);

	//Link the variables to the parent values (house values)
	design_cooling_capacity = &house_lnk->design_cooling_capacity;
	outside_temperature = &house_lnk->outside_temperature;
	thermal_storage_available = &house_lnk->thermal_storage_present;
	thermal_storage_active = &house_lnk->thermal_storage_inuse;

	//Check the cooling capacity
	if (*design_cooling_capacity == NULL)
	{
		gl_warning("\'design_cooling_capacity\' not specified in parent ~ default to 5 ton or 60,000 Btu/hr");
		/* TROUBLESHOOT
			The thermal_storage did not reference a parent object that publishes design_cooling_capacity, so 5 ton was assumed.
			Confirm or change the parent reference and try again.
		*/
		discharge_rate = 5 * 12000; //Btu/hr, is set to 5 ton when not defined
		water_capacity = 1.7413;	//m^3, is set to the same as a 5 ton unit
	} else {
		discharge_rate = *design_cooling_capacity;
		water_capacity = 1.7413 * (discharge_rate / (5 * 12000));
	}

	surface_area = 6 * pow(water_capacity, 0.6667); //suface area of a cube calculated from volume

	if (total_capacity == 0)			total_capacity = (30 / 5) * discharge_rate; //Btu

	if (state_of_charge < 0 && stored_capacity < 0) //Btu
	{
		stored_capacity = total_capacity;
		state_of_charge = 100;
	} else if (state_of_charge < 0 && stored_capacity >= 0)
	{
		state_of_charge = stored_capacity / total_capacity;
	} else if (state_of_charge >= 0 && stored_capacity < 0)
	{
		stored_capacity = (state_of_charge / 100) * total_capacity;
	} else if (state_of_charge >= 0 && stored_capacity >= 0)
	{
		stored_capacity = (state_of_charge / 100) * total_capacity;
		gl_warning("stored_capacity and SOC are both defined, SOC being used for initial energy state");
		/*  TROUBLESHOOT
		During the initialization of the system, a value was specified for both the stored_capacity and SOC (state of charge).
		The thermal energy storage object gives precedence to the SOC variable, so the initial stored_capacity will be the SOC
		percentage of the total_capacity.
		*/
	}

	if (recharge_power == 0)			recharge_power = (3.360 * discharge_rate) / (5 * 12000); //kW
	if (discharge_power == 0)			discharge_power = (0.300 * discharge_rate) / (5 * 12000); //kW
	if (recharge_power_factor == 0)		recharge_power_factor = 0.97; //same as used for HVAC compressor in house_e
	if (discharge_power_factor == 0)	discharge_power_factor = 1; //assume ideal pump
	if (k < 0)							k = 0; //assume no thermal conductivity
	k = k * 0.00052667;				//convert k from W/m/K to BTU/sec/m/degF

	//Determine how to read the scheduling information - charging
	if (recharge_schedule_type==INTERNAL)
	{
		//See if someone else has already created such a schedule
		recharge_schedule_vals = gl_schedule_find(thermal_default_schedule_list[1].schedule_name);

		//If not found, create
		if (recharge_schedule_vals == NULL)
		{
			//Populate schedules - charging
			recharge_schedule_vals = gl_schedule_create(thermal_default_schedule_list[1].schedule_name,thermal_default_schedule_list[1].schedule_definition);

			//Make sure it worked
			if (recharge_schedule_vals==NULL)
			{
				GL_THROW("Failure to create default charging schedule");
				/*  TROUBLESHOOT
				While attempting to create the default charging schedule in the thermal_storage object, an error occurred.  Please try again.
				If the error persists, please submit your code and a bug report via the track website.
				*/
			}
		}

		gl_verbose("thermal_storage charging defaulting to internal schedule");
		/*  TROUBLESHOOT
		recharge_schedule_type was not set to EXTERNAL, so the internal schedule definition will be used
		for the recharging schedule.
		*/

		//Assign to the schedule value
		recharge_time_ptr = &recharge_schedule_vals->value;
	}
	else
	{
		//Assign the to published property
		recharge_time_ptr = &recharge_time;
	}

	//Determine how to read the scheduling information - discharging
	if (discharge_schedule_type==INTERNAL)
	{
		//See if someone else has already created such a schedule
		discharge_schedule_vals = gl_schedule_find(thermal_default_schedule_list[0].schedule_name);

		//If not found, create
		if (discharge_schedule_vals == NULL)
		{
			//Populate schedules - discharging
			discharge_schedule_vals = gl_schedule_create(thermal_default_schedule_list[0].schedule_name,thermal_default_schedule_list[0].schedule_definition);

			//Make sure it worked
			if (discharge_schedule_vals==NULL)
			{
				GL_THROW("Failure to create default discharging schedule");
				/*  TROUBLESHOOT
				While attempting to create the default discharging schedule in the thermal_storage object, an error occurred.  Please try again.
				If the error persists, please submit your code and a bug report via the track website.
				*/
			}
		}

		gl_verbose("thermal_storage discharging defaulting to internal schedule");
		/*  TROUBLESHOOT
		discharge_schedule_type was not set to EXTERNAL, so the internal schedule definition will be used
		for the discharging availability schedule.
		*/

		//Assign to the schedule value
		discharge_time_ptr = &discharge_schedule_vals->value;
	}
	else
	{
		//Assigned to the published property
		discharge_time_ptr = &discharge_time;
	}

	// waiting this long to initialize the parent class is normal
	return residential_enduse::init(parent);
}

int thermal_storage::isa(char *classname)
{
	return (strcmp(classname,"thermal_storage")==0 || residential_enduse::isa(classname));
}

TIMESTAMP thermal_storage::sync(TIMESTAMP t0, TIMESTAMP t1)
{
	double val = 0.0;
	TIMESTAMP t2 = TS_NEVER;
	next_timestep = TS_NEVER;
	double actual_recharge_power;

	//Make sure we aren't on the first run
	if (t0 != 0)
	{
		if (*recharge_time_ptr == 1 && *discharge_time_ptr == 1)
		{
			gl_warning("recharge and discharge can not both be scheduled to be concurrently on ~ defaulting to recharge");
			/*  TROUBLEHSOOT
			The schedules or control values used to determine when the thermal energy storage can charge or discharge are overlapping.  In this
			case, the system gives precedence to recharging the thermal energy storage.  If this behavior is undesired, please ensure proper time
			separation of the controlling charge and discharge signals.
			*/
			*discharge_time_ptr = 0;
		}

		if (*recharge_time_ptr == 1 && *discharge_time_ptr != 1 && state_of_charge < 100)
		{
			if (recharge == 0)	last_timestep = t0; //don't run energy calculations when first turned on
			recharge = 1;
		} else {
			recharge = 0;
		}
		
		//Ice energy loss calculations
		//Rate of loss = k*A*(T1-T2)/d [Btu/sec]
		//k is the coefficient of thermal conductivity [W/m/K - converted to BTU/sec/m/degF]
		//A is the surface area [m^2]
		//(T1 - T2) is the inside temperature minus the outside temperature
		//d is the thickness of the insullation [m]
		//storage loss = rate of loss * delta time [Btu]
		if (stored_capacity > 0)
		{
			surface_area = 6 * pow(water_capacity, 0.6667); //suface area of a cube calculated from volume
			stored_capacity = stored_capacity + ((k * surface_area * (32 - *outside_temperature) / 0.05) * (t0 - last_timestep));
			state_of_charge = stored_capacity / total_capacity * 100;
			if (stored_capacity < 0)
			{
				stored_capacity = 0;
				state_of_charge = 0;
			}
			if (stored_capacity >= total_capacity)
			{
				stored_capacity = total_capacity;
				state_of_charge = 100;
			}
		}

		//Recharge cycle
		actual_recharge_power = recharge_power * (1 + (75 - *outside_temperature) * 0.0106);
		if (recharge && *recharge_time_ptr == 1 && *outside_temperature >= 15 && *outside_temperature <=115)
		//needs to recharge, is set to recharge and the outside temperature is between 15 and 155 deg F
		{
			//Recharge cycle, so ES is not available
			*thermal_storage_available = 0;
			actual_recharge_power = recharge_power * (1 + (75 - *outside_temperature) * 0.0106);
			if (last_timestep != t0) //only calculate energy once per timestep
			{
				stored_capacity = stored_capacity + (((total_capacity / 30) / (9 * *outside_temperature + 705)) * (t0 - last_timestep));
			}
			if (stored_capacity >= total_capacity)
			{
				recharge = 0;
				stored_capacity = total_capacity;
				state_of_charge = 100;
				load.power = 0;
				load.power_factor = recharge_power_factor;
				load.heatgain = 0;
			} else
			{
				recharge = 1;
				state_of_charge = stored_capacity / total_capacity * 100;
				next_timestep = (TIMESTAMP)((total_capacity - stored_capacity) / ((total_capacity / 30) / (9 * *outside_temperature + 705)));
				if (next_timestep == 0)	next_timestep = 1;
				load.power = actual_recharge_power;
				load.power_factor = recharge_power_factor;
				load.heatgain = 0;
			}
		}

		if (*recharge_time_ptr != 1 && *discharge_time_ptr == 1 && state_of_charge > 0 && *outside_temperature >=15)
		//not set to recharge, set to discharge, has charge to use and the outside air temperature is above 15 deg F
		{
			if (*thermal_storage_available == 0)	last_timestep = t0; //don't run energy calculations when first turned on
			*thermal_storage_available = 1;
		} else {
			*thermal_storage_available = 0;
		}

		//Discharge cycle
		if (recharge == 0 && *discharge_time_ptr == 1 && (*thermal_storage_active > 0))
		{
			if (stored_capacity > 0)
			{
				//Capacity available - flag as available
				*thermal_storage_available = 1;
				if (last_timestep != t0) //only calculate energy once per timestep
				{
					stored_capacity = stored_capacity - (discharge_rate * (t0 - last_timestep) / 3600);
				}
				state_of_charge = stored_capacity / total_capacity * 100;
				next_timestep = (TIMESTAMP)((stored_capacity / discharge_rate) * 3600);
				if (next_timestep == 0)	next_timestep = 1;
				load.power = discharge_power;
				load.power_factor = discharge_power_factor;
				load.heatgain = -discharge_rate;
			} else
			{
				//Out of capacity - flag as unavailable
				*thermal_storage_available = 0;
				state_of_charge = 0;
				load.power = 0;
				load.power_factor = discharge_power_factor;
				load.heatgain = 0;
			}
			if (stored_capacity <= 0)	stored_capacity = 0;
		} else {
			load.heatgain = 0;
			if (*discharge_time_ptr == 1 && *recharge_time_ptr != 1)
			{
				load.power = 0;
				load.power_factor = 1;
			}
		}

		t2 = residential_enduse::sync(t0,t1);
		if (t2 > t0 + next_timestep)	t2 = t0 + next_timestep;

		last_timestep = t0;
	}
	else	//First run
		last_timestep = t1;	//Initialize variable

	return t2;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_thermal_storage(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(thermal_storage::oclass);
	if (*obj!=NULL)
	{
		thermal_storage *my = OBJECTDATA(*obj,thermal_storage);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			/* TROUBLESHOOT
				The create operation of the specified object failed.
				The message given provide additional details and can be looked up under the Exceptions section.
			 */
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT int init_thermal_storage(OBJECT *obj)
{
	thermal_storage *my = OBJECTDATA(obj,thermal_storage);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		/* TROUBLESHOOT
			The initialization operation of the specified object failed.
			The message given provide additional details and can be looked up under the Exceptions section.
		 */
		return 0;
	}
}

EXPORT int isa_thermal_storage(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,thermal_storage)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_thermal_storage(OBJECT *obj, TIMESTAMP t1)
{
	thermal_storage *my = OBJECTDATA(obj,thermal_storage);
	try {
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		obj->clock = t1;
		return t2;
	}
	catch (char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		/* TROUBLESHOOT
			The synchronization operation of the specified object failed.
			The message given provide additional details and can be looked up under the Exceptions section.
		 */
		return 0;
	}
}

/**@}**/
