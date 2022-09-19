/** $Id: battery.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file battery.cpp
	@defgroup battery
	@ingroup generators

 @{
 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "battery.h"

#define HOUR 3600 * TS_SECOND

CLASS *battery::oclass = nullptr;
battery *battery::defaults = nullptr;

static PASSCONFIG passconfig = PC_BOTTOMUP|PC_POSTTOPDOWN;
static PASSCONFIG clockpass = PC_BOTTOMUP;

/* Class registration is only called once to register the class with the core */
battery::battery(MODULE *module)
{
	if (oclass==nullptr)
	{
		oclass = gl_register_class(module,"battery",sizeof(battery),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==nullptr)
			throw "unable to register class battery";
		else
			oclass->trl = TRL_PROOF;

		if (gl_publish_variable(oclass,
			PT_enumeration,"generator_mode",PADDR(gen_mode_v), PT_DESCRIPTION, "LEGACY MODEL: Selects generator control mode when using legacy model; in non-legacy models, this should be SUPPLY_DRIVEN.",
				PT_KEYWORD,"UNKNOWN",(enumeration)GM_UNKNOWN,
				PT_KEYWORD,"CONSTANT_V",(enumeration)GM_CONSTANT_V,
				PT_KEYWORD,"CONSTANT_PQ",(enumeration)GM_CONSTANT_PQ,
				PT_KEYWORD,"CONSTANT_PF",(enumeration)GM_CONSTANT_PF,
				PT_KEYWORD,"SUPPLY_DRIVEN",(enumeration)GM_SUPPLY_DRIVEN,
				PT_KEYWORD,"POWER_DRIVEN",(enumeration)GM_POWER_DRIVEN,
				PT_KEYWORD,"VOLTAGE_CONTROLLED",(enumeration)GM_VOLTAGE_CONTROLLED,
				PT_KEYWORD,"POWER_VOLTAGE_HYBRID",(enumeration)GM_POWER_VOLTAGE_HYBRID,

			PT_enumeration,"additional_controls",PADDR(additional_controls), PT_DESCRIPTION, "LEGACY MODEL: In conjunction with POWER_DRIVEN, VOLTAGE_CONTROLLED, and POWER_VOLTAGE_HYBRID, this will activate control set points that adjust with temperature",
				PT_KEYWORD,"NONE",(enumeration)AC_NONE,
				PT_KEYWORD,"LINEAR_TEMPERATURE",(enumeration)AC_LINEAR_TEMPERATURE,

			PT_enumeration,"generator_status",PADDR(gen_status_v), PT_DESCRIPTION, "describes whether the generator is online or offline",
				PT_KEYWORD,"OFFLINE",(enumeration)OFFLINE,
				PT_KEYWORD,"ONLINE",(enumeration)ONLINE,	

			PT_enumeration,"rfb_size", PADDR(rfb_size_v), PT_DESCRIPTION, "Default settings for certain sizes of batteries",
				PT_KEYWORD, "HOUSEHOLD", (enumeration)HOUSEHOLD,
				PT_KEYWORD, "SMALL", (enumeration)SMALL, 
				PT_KEYWORD, "MED_COMMERCIAL", (enumeration)MED_COMMERCIAL,
				PT_KEYWORD, "MED_HIGH_ENERGY", (enumeration)MED_HIGH_ENERGY,
				PT_KEYWORD, "LARGE", (enumeration)LARGE,

			PT_enumeration,"power_type",PADDR(power_type_v), PT_DESCRIPTION, "LEGACY MODEL: Not currently used", //This is not used anywhere in the code. I would suggest removing it from the model.
				PT_KEYWORD,"AC",(enumeration)AC,
				PT_KEYWORD,"DC",(enumeration)DC,

			PT_enumeration,"battery_state",PADDR(battery_state), PT_DESCRIPTION, "Describes the current state of the battery",
				PT_KEYWORD,"CHARGING",(enumeration)BS_CHARGING,
				PT_KEYWORD,"DISCHARGING",(enumeration)BS_DISCHARGING,
				PT_KEYWORD,"WAITING",(enumeration)BS_WAITING,
				PT_KEYWORD,"FULL",(enumeration)BS_FULL,
				PT_KEYWORD,"EMPTY",(enumeration)BS_EMPTY,
				PT_KEYWORD,"CONFLICTED",(enumeration)BS_CONFLICTED,

			PT_double, "number_battery_state_changes", PADDR(no_of_cycles),PT_DESCRIPTION, "LEGACY MODEL: Number of times battery switches between charging and discharging", 

			// Used for linear temperature control mode
			PT_double, "monitored_power[W]", PADDR(check_power), PT_DESCRIPTION, "LEGACY MODEL: output only; power output value of parent meter when performing load following modes (POWER_DRIVEN)",
			PT_double, "power_set_high[W]", PADDR(power_set_high), PT_DESCRIPTION, "LEGACY MODEL: high set point of dead band for load following (POWER_DRIVEN)",
			PT_double, "power_set_low[W]", PADDR(power_set_low), PT_DESCRIPTION, "LEGACY MODEL: low set point of dead band for load following (POWER_DRIVEN)",
			PT_double, "power_set_high_highT[W]", PADDR(power_set_high_highT), PT_DESCRIPTION, "LEGACY MODEL: high set point of dead band for load following at higher temperatures (POWER_DRIVEN + LINEAR_TEMPERATURE)",
			PT_double, "power_set_low_highT[W]", PADDR(power_set_low_highT), PT_DESCRIPTION, "LEGACY MODEL: low set point of dead band for load following at higher temperatures (POWER_DRIVEN + LINEAR_TEMPERATURE)",
			PT_double, "check_power_low[W]", PADDR(check_power_low), PT_DESCRIPTION, "LEGACY MODEL: high set point of dead band for load following at lower temperatures (POWER_DRIVEN + LINEAR_TEMPERATURE)",
			PT_double, "check_power_high[W]", PADDR(check_power_high), PT_DESCRIPTION, "LEGACY MODEL: low set point of dead band for load following at lower temperatures (POWER_DRIVEN + LINEAR_TEMPERATURE)",
			PT_double, "voltage_set_high[V]", PADDR(voltage_set_high), PT_DESCRIPTION, "LEGACY MODEL: high set point for voltage control",
			PT_double, "voltage_set_low[V]", PADDR(voltage_set_low), PT_DESCRIPTION, "LEGACY MODEL: low set point for voltage control",
			PT_double, "deadband[V]", PADDR(deadband), PT_DESCRIPTION, "LEGACY MODEL: voltage deadband",
			PT_double, "sensitivity", PADDR(sensitivity), PT_DESCRIPTION, "LEGACY MODEL: describes how sensitive the control is to temperature excursions; defines slope of linear control",
			PT_double, "high_temperature", PADDR(high_temperature), PT_DESCRIPTION, "LEGACY MODEL: high temperature of linear control; defines slope",
			PT_double, "midpoint_temperature", PADDR(midpoint_temperature), PT_DESCRIPTION, "LEGACY MODEL: midpoint temperature of linear control; defines slope",
			PT_double, "low_temperature", PADDR(low_temperature), PT_DESCRIPTION, "LEGACY MODEL: low temperature of linear control; defines slope",

			//Used for PQ mode
			PT_double, "scheduled_power[W]", PADDR(B_scheduled_power), PT_DESCRIPTION, "LEGACY MODEL: real power output of battery/inverter system",

			PT_double, "Rinternal[Ohm]", PADDR(Rinternal), PT_DESCRIPTION, "LEGACY MODEL: the internal resistance of the battery.",
			PT_double, "V_Max[V]", PADDR(V_Max), PT_DESCRIPTION, "LEGACY MODEL: the maximum terminal voltage of the battery.",
			PT_complex, "I_Max[A]", PADDR(I_Max), PT_DESCRIPTION, "LEGACY MODEL: the maximum current output of the battery.",
			PT_double, "E_Max[Wh]", PADDR(E_Max), PT_DESCRIPTION, "LEGACY MODEL: the maximum capacity of the battery.",
			PT_double, "P_Max[W]", PADDR(Max_P), PT_DESCRIPTION, "LEGACY MODEL: the maximum power output of the battery.",
			PT_double, "power_factor", PADDR(pf), PT_DESCRIPTION, "LEGACY MODEL: the power factor output of the battery.",
			PT_double, "Energy[Wh]",PADDR(Energy), PT_DESCRIPTION, "LEGACY MODEL: the available capacity of the battery.",
			PT_double, "efficiency[unit]", PADDR(efficiency), PT_DESCRIPTION, "LEGACY MODEL: the efficiency of the battery.",
			PT_double, "base_efficiency[unit]", PADDR(base_efficiency), PT_DESCRIPTION, "LEGACY MODEL: the efficiency of the battery at rated voltaged and current.",
			PT_double, "parasitic_power_draw[W]", PADDR(parasitic_power_draw), PT_DESCRIPTION, "LEGACY MODEL: the parasytic power draw of the battery when idle.",


			//PT_int64, "generator_mode_choice", PADDR(generator_mode_choice),

			//PT_double, "Rated_kV[kV]", PADDR(Rated_kV),
			PT_complex, "V_Out[V]", PADDR(V_Out), PT_DESCRIPTION, "LEGACY MODEL: the AC voltage at the terminals of the battery.",
			PT_complex, "I_Out[A]", PADDR(I_Out), PT_DESCRIPTION, "LEGACY MODEL: the AC current output of the battery.",
			PT_complex, "VA_Out[VA]", PADDR(VA_Out), PT_DESCRIPTION, "LEGACY MODEL: the power output of the battery.",
			PT_complex, "I_In[A]", PADDR(I_In), PT_DESCRIPTION, "LEGACY MODEL: the current flowing into the battery of the battery.",
			PT_complex, "V_Internal[V]", PADDR(V_Internal), PT_DESCRIPTION, "LEGACY MODEL: the internal voltage of the battery.",
			PT_complex, "I_Internal[A]",PADDR(I_Internal), PT_DESCRIPTION, "LEGACY MODEL: the internal current of the battery.",
			PT_complex, "I_Prev[A]", PADDR(I_Prev), PT_DESCRIPTION, "LEGACY MODEL: the previous current output of the battery.",

			PT_double,"power_transferred",PADDR(power_transferred), PT_DESCRIPTION, "LEGACY MODEL: the power output of the battery.",

			//resistasnces and max P, Q

			//internal battery model parameters
			PT_bool,"use_internal_battery_model", PADDR(use_internal_battery_model), PT_DESCRIPTION, "Enables the INTERNAL BATTERY MODEL.",
			PT_enumeration,"battery_type", PADDR(battery_type), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the type of the battery. Used to determine the soc vs voltage curve.",
				PT_KEYWORD, "UNKNOWON", (enumeration)UNKNOWN,
				PT_KEYWORD, "LI_ION", (enumeration)LI_ION,
				PT_KEYWORD, "LEAD_ACID", (enumeration)LEAD_ACID,

			PT_double,"nominal_voltage[V]", PADDR(v_max), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the rated DC voltage at the terminals of the battery.",
			PT_double,"rated_power[W]", PADDR(p_max), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the rated power output of the battery.",
			PT_double,"battery_capacity[Wh]", PADDR(e_max), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the rated battery capacity of the battery.",
			PT_double,"round_trip_efficiency[pu]", PADDR(eta_rt), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the round trip efficiency of the battery according to a full discharge/charge cycle.",
			PT_double,"state_of_charge[pu]", PADDR(soc), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the current state of charge of the battery.",
			PT_double,"battery_load[W]", PADDR(bat_load), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the current power output of the battery.",
			PT_double,"reserve_state_of_charge[pu]", PADDR(b_soc_reserve), PT_DESCRIPTION, "INTERNAL BATTERY MODEL: the reserve state of charge the battery can reach.",

			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(this,0,sizeof(battery));

		if (gl_publish_function(oclass,	"preupdate_battery_object", (FUNCTIONADDR)preupdate_battery)==nullptr)
			GL_THROW("Unable to publish battery deltamode function");
		if (gl_publish_function(oclass,	"interupdate_battery_object", (FUNCTIONADDR)interupdate_battery)==nullptr)
			GL_THROW("Unable to publish battery deltamode function");
		if (gl_publish_function(oclass,	"postupdate_battery_object", (FUNCTIONADDR)postupdate_battery)==nullptr)
			GL_THROW("Unable to publish battery deltamode function");


		/* TODO: set the default values of all properties here */
	}
}

/* Object creation is called once for each object that is created by the core */
int battery::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	
	gen_status_v = ONLINE;
	//rfb_size_v = SMALL;

	Rinternal = 10;
	V_Max = 0;
	I_Max = 0;
	E_Max = 0;
	Energy = -1;
	recalculate = true;
	margin = 1000;

	no_of_cycles = 0;
	deadband = 0;
	parasitic_power_draw = 0;
	additional_controls = AC_NONE;
	midpoint_temperature = 50;
	low_temperature = 0;
	high_temperature = 100;
	sensitivity = 0.5;
	
	Max_P = 0;//< maximum real power capacity in kW
	
	efficiency =  0;
	base_efficiency = 0;
	Iteration_Toggle = false;

	E_Next = 0;

	use_internal_battery_model = false;
	soc = -1;
	b_soc_reserve = 0;
	state_change_time = 0;
	
	first_run = true;
	enableDelta = false;
	state_change_time_delta = 0;
	Pout_delta =  0;
	pCircuit_V[0] = pCircuit_V[1] = pCircuit_V[2] = nullptr;
	pLine_I[0] = pLine_I[1] = pLine_I[2] = nullptr;
	pLine12 = nullptr;
	pPower = nullptr;
	pTout = nullptr;
	pSoc = nullptr;
	pBatteryLoad = nullptr;
	pSocReserve = nullptr;
	pRatedPower = nullptr;
	peff = nullptr;
	pinverter_VA_Out = nullptr;

	value_Circuit_V[0] = value_Circuit_V[1] = value_Circuit_V[2] = gld::complex(0.0,0.0);
	value_Line_I[0] = value_Line_I[1] = value_Line_I[2] = gld::complex(0.0,0.0);
	value_Line12 = gld::complex(0.0,0.0);

	value_Tout = 0.0;

	parent_is_meter = false;
	parent_is_triplex = false;
	parent_is_inverter = false;
	climate_object_found = false;	//Use default data, by default

	/* TODO: set the context-free initial value of properties */
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int battery::init(OBJECT *parent)
{
	OBJECT *obj = OBJECTHDR(this);
	gld_property *temp_property_pointer;
	double temp_value_SocReserve;
	enumeration temp_value_control_mode;

	if(parent != nullptr){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("battery::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	//See if the global flag is set - if so, add the object flag
	if (all_generator_delta)
	{
		obj->flags |= OF_DELTAMODE;
	}

	//Set the deltamode flag, if desired
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		deltamode_inclusive = true;	//Set the flag and off we go
	}

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (!enable_subsecond_models)
		{
			gl_warning("battery:%s indicates it wants to run deltamode, but the module-level flag is not set!",obj->name?obj->name:"unnamed");
			/*  TROUBLESHOOT
			The diesel_dg object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The generator
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			gen_object_count++;	//Increment the counter
		}
	}//End deltamode inclusive

	else	//Not enabled for this model
	{
		if (enable_subsecond_models)
		{
			gl_warning("battery:%d %s - Deltamode is enabled for the module, but not this generator!",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			The diesel_dg is not flagged for deltamode operations, yet deltamode simulations are enabled for the overall system.  When deltamode
			triggers, this generator may no longer contribute to the system, until event-driven mode resumes.  This could cause issues with the simulation.
			It is recommended all objects that support deltamode enable it.
			*/
		}
	}

	if(!use_internal_battery_model)
	{
		// find parent meter, if not defined, use a default meter (using static variable 'default_meter')
		if (parent!=nullptr)
		{
			if (gl_object_isa(parent, "meter", "powerflow"))
			{
				//Set the flags
				parent_is_meter = true;
				parent_is_triplex = false;
				parent_is_inverter = false;

				//Make the mappings
				pCircuit_V[0] = map_complex_value(parent,"voltage_A");
				pCircuit_V[1] = map_complex_value(parent,"voltage_B");
				pCircuit_V[2] = map_complex_value(parent,"voltage_C");

				pLine_I[0] = map_complex_value(parent,"current_A");
				pLine_I[1] = map_complex_value(parent,"current_B");
				pLine_I[2] = map_complex_value(parent,"current_C");

				pLine12 = nullptr;

				pPower = map_complex_value(parent,"measured_power");

				//Map and pull the phases
				temp_property_pointer = new gld_property(parent,"phases");

				//Make sure ti worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
				{
					GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
					/*  TROUBLESHOOT
					While attempting to map the phases property from the parent object, an error was encountered.
					Please check and make sure your parent object is a meter or triplex_meter inside the powerflow module and try
					again.  If the error persists, please submit your code and a bug report via the Trac website.
					*/
				}

				//Pull the phase information
				phases = temp_property_pointer->get_set();

				//Clear the temporary pointer
				delete temp_property_pointer;

				if ( (phases & 0x07) == 0x07 ){ // three phase
					number_of_phases_out = 3;
				}
				else if ( ((phases & 0x03) == 0x03) || ((phases & 0x05) == 0x05) || ((phases & 0x06) == 0x06) ){ // two-phase
					number_of_phases_out = 2;
					GL_THROW("Battery %d: The battery can only be connected to a meter with all three phases. Please check parent meter's phases.",obj->id);
					/* TROUBLESHOOT
					The battery's parent is a meter. The battery can only operate correctly when the parent meter is a three-phase meter.
					*/
				}
				else if ( ((phases & 0x01) == 0x01) || ((phases & 0x02) == 0x02) || ((phases & 0x04) == 0x04) ){ // single phase
					number_of_phases_out = 1;
					GL_THROW("Battery %d: The battery can only be connected to a meter with all three phases. Please check parent meter's phases.",obj->id);
					/* TROUBLESHOOT
					The battery's parent is a meter. The battery can only operate correctly when the parent meter is a three-phase meter.
					*/
				}
				else
				{
					//Never supposed to really get here
					GL_THROW("Invalid phase configuration specified!");
					/*  TROUBLESHOOT
					An invalid phase congifuration was specified when attaching to the "parent" object.  Please report this
					error.
					*/
				}

				//Pull the initial voltages in, just to init them
				value_Circuit_V[0] = pCircuit_V[0]->get_complex();
				value_Circuit_V[1] = pCircuit_V[1]->get_complex();
				value_Circuit_V[2] = pCircuit_V[2]->get_complex();
			}
			else if (gl_object_isa(parent, "triplex_meter", "powerflow"))
			{
				//Set flags
				parent_is_meter = true;
				parent_is_triplex = true;
				parent_is_inverter = false;

				//Map the variables
				pCircuit_V[0] = map_complex_value(parent,"voltage_12");
				pCircuit_V[1] = map_complex_value(parent,"voltage_1N");
				pCircuit_V[2] = map_complex_value(parent,"voltage_2N");

				pLine_I[0] = map_complex_value(parent,"current_1");
				pLine_I[1] = map_complex_value(parent,"current_2");
				pLine_I[2] = map_complex_value(parent,"current_N");

				pLine12 = map_complex_value(parent,"current_12");

				pPower = map_complex_value(parent,"measured_power");

				//Map and pull the phases
				temp_property_pointer = new gld_property(parent,"phases");

				//Make sure ti worked
				if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_set())
				{
					GL_THROW("Unable to map phases property - ensure the parent is a meter or triplex_meter");
					//Defined above
				}

				//Pull the phase information
				phases = temp_property_pointer->get_set();

				//Clear the temporary pointer
				delete temp_property_pointer;

				number_of_phases_out = 1;

				//Pull the initial voltages in, just to init them
				value_Circuit_V[0] = pCircuit_V[0]->get_complex();
				value_Circuit_V[1] = pCircuit_V[1]->get_complex();
				value_Circuit_V[2] = pCircuit_V[2]->get_complex();
			}
			else if (gl_object_isa(parent, "inverter", "generators"))
			{
				//Set flags
				parent_is_meter = true;
				parent_is_triplex = false;
				parent_is_inverter = true;

				number_of_phases_out = 0;
				phases = 0x00;

				//Map up the two inverter variabes
				pCircuit_V[0] = map_double_value(parent,"V_In");
				pCircuit_V[1] = nullptr;
				pCircuit_V[2] = nullptr;

				pLine_I[0] = map_double_value(parent,"I_In");
				pLine_I[1] = nullptr;
				pLine_I[2] = nullptr;

				peff = map_double_value(parent,"inverter_efficiency");
				pinverter_VA_Out = map_double_value(parent,"P_Out");

				//Pull the initial voltage value, to be consistent
				value_Circuit_V[0] = complex(pCircuit_V[0]->get_double(),0.0);
			}
			else	//Not a proper parent
			{
				GL_THROW("Battery must have a meter or triplex meter or inverter as it's parent");
				/*  TROUBLESHOOT
				Check the parent object of the inverter.  The battery is only able to be childed via a meter or 
				triplex meter when connecting into powerflow systems.  You can also choose to have no parent, in which
				case the battery will be a stand-alone application using default voltage values for solving purposes.
				*/
			}
		}
		else	//No parent
		{
			parent_is_meter = false;
			parent_is_triplex = false;
			parent_is_inverter = false;

			number_of_phases_out = 3;
			phases = 0x07;

			gl_warning("Battery:%d has no parent meter object defined; using static voltages", obj->id);

			//Assumes three-phase, so just populate with the module-level value
			value_Circuit_V[0].SetPolar(default_line_voltage,0);
			value_Circuit_V[1].SetPolar(default_line_voltage,-2.0/3.0*PI);
			value_Circuit_V[2].SetPolar(default_line_voltage,2.0/3.0*PI);
		}

		if (gen_mode_v==GM_UNKNOWN)
		{
			GL_THROW("Generator (id:%d) generator_mode is not specified",obj->id);
		}
		else if (gen_mode_v == GM_VOLTAGE_CONTROLLED)
		{	
			GL_THROW("VOLTAGE_CONTROLLED generator_mode is not yet implemented.");
		}
		else if (gen_mode_v == GM_CONSTANT_PF)
		{	
			GL_THROW("CONSTANT_PF generator_mode is not yet implemented.");
		}
		else if (gen_mode_v == GM_SUPPLY_DRIVEN)
		{	
			GL_THROW("SUPPLY_DRIVEN generator_mode is not yet implemented.");
		}
			
		if (additional_controls == AC_LINEAR_TEMPERATURE)
		{
			FINDLIST *climates = nullptr;

			climates = gl_find_objects(FL_NEW,FT_CLASS,SAME,"climate",FT_END);

			if (climates==nullptr)
			{
				climate_object_found = false;

				gl_warning("battery: no climate data found, using static data");

				//Assign the default from the module
				value_Tout = default_temperature_value;
			}
			if (climates!=nullptr)
			{
				if (climates->hit_count==0)
				{
					climate_object_found = false;

					//Assign the default from the module
					value_Tout = default_temperature_value;
				}
				else //climate data was found
				{
					climate_object_found = true;

					OBJECT *clim = gl_find_next(climates,NULL);

					if (clim->rank<=obj->rank)
						gl_set_dependent(clim,obj);

					//Map the temperature
					pTout = map_double_value(clim,"temperature");
				}
			}
		}

		if (gen_status_v == OFFLINE)
		{
			//OBJECT *obj = OBJECTHDR(this);
			gl_warning("Generator (id:%d) is out of service!",obj->id);
		}
		else
		{
			switch(rfb_size_v)
			{
				case HOUSEHOLD:
					V_Max = 260;
					I_Max = 15;
					Max_P = V_Max.Mag()*I_Max.Mag(); 
					E_Max = 6*Max_P; //Wh		 -- 6 hours of maximum
					base_efficiency = 0.9; // roundtrip
					break;
				case SMALL:
					V_Max = 75.2; //V
					I_Max = 250; //A
					Max_P = 18800; //W
					E_Max = 160000; //Wh
					base_efficiency = 0.7; // roundtrip
					break;
				case MED_COMMERCIAL:
					V_Max = 115; //V
					I_Max = 435; //A
					Max_P = 50000; //W
					E_Max = 175000; //Wh
					base_efficiency = 0.8; // roundtrip
					break;
				case MED_HIGH_ENERGY:
					V_Max = 115; //V
					I_Max = 435; //A
					Max_P = 50000; //W
					E_Max = 400000; //Wh
					base_efficiency = 0.8; // roundtrip
					break;
				case LARGE:
					V_Max = 8000; //V  
					I_Max = 30; //A
					Max_P = V_Max.Mag()*I_Max.Mag(); 
					E_Max = 24*Max_P; //Wh		 
					base_efficiency = 0.9; // roundtrip
					break;
				default:
					if (V_Max == 0)
					{
						gl_warning("V_max was not given -- using default value (battery: %d)",obj->id);
						V_Max = 115; //V
					}
					if (I_Max == 0)
					{
						gl_warning("I_max was not given -- using default value (battery: %d)",obj->id);
						I_Max = 435; //A
					}
					if (Max_P == 0)
					{
						gl_warning("Max_P was not given -- using default value (battery: %d)",obj->id);
						Max_P = V_Max.Re()*I_Max.Re(); //W
					}
					if (E_Max == 0)
					{
						gl_warning("E_max was not given -- using default value (battery: %d)",obj->id);
						E_Max = 6*Max_P; //Wh
					}
					if (base_efficiency == 0)
					{
						gl_warning("base_efficiency was not given -- using default value (battery: %d)",obj->id);
						base_efficiency = 0.8; // roundtrip
					}
					break;
			}
		}
		if (power_set_high <= power_set_low && (gen_mode_v == GM_POWER_DRIVEN || gen_mode_v == GM_POWER_VOLTAGE_HYBRID))
			gl_warning("power_set_high is less than power_set_low -- oscillations will most likely occur");

		if (Energy < 0)
		{
			gl_warning("Initial Energy was not set, or set to a negative number.  Randomizing initial value.");
			Energy = gl_random_normal(RNGSTATE,E_Max/2,E_Max/20);
		}

		recalculate = true;
		power_transferred = 0;

		first_time_step = 0;
	}
	else	//Use the internal battery model
	{
		if (parent!=nullptr)
		{
			//Promote our parent, as necessary
			gl_set_rank(parent,obj->rank+1);
		}

		// find parent inverter, if not defined, use a default meter (using static variable 'default_meter')
		if(parent == nullptr)
		{
			GL_THROW("Battery must have an inverter as it's parent when using the internal battery model");
			/*  TROUBLESHOOT
			When using the internal battery model parameter option for the battery object, it must be parented
			to an inverter to work.  If this is not done, the code will fail.
			*/
		}
		else if (!gl_object_isa(parent, "inverter", "generators"))
		{
			GL_THROW("Battery must have an inverter as it's parent");
			//Defined above
		}

		switch(rfb_size_v)
		{
			case HOUSEHOLD:
				v_max = 260; //V
				p_max = 3900; //W
				e_max = 23400; //Wh		 -- 6 hours of maximum
				eta_rt = 0.9; // roundtrip
				break;
			case SMALL:
				v_max = 75.2; //V
				p_max = 18800; //W
				e_max = 160000; //Wh
				eta_rt = 0.7; // roundtrip
				break;
			case MED_COMMERCIAL:
				v_max = 115; //V
				p_max = 50000; //W
				e_max = 175000; //Wh
				eta_rt = 0.8; // roundtrip
				break;
			case MED_HIGH_ENERGY:
				v_max = 115; //V
				p_max = 50000; //W
				e_max = 400000; //Wh
				eta_rt = 0.8; // roundtrip
				break;
			case LARGE:
				v_max = 8000; //V  
				p_max = 240000; //W
				e_max = 5760000; //Wh		 
				eta_rt = 0.9; // roundtrip
				break;
			default:
				if (v_max == 0)
				{
					gl_warning("v_max was not given -- using default value (battery: %d)",obj->id);
					v_max = 115; //V
				}
				if (e_max == 0)
				{
					gl_warning("e_max was not given -- using default value (battery: %d)",obj->id);
					e_max = 400000; //Wh
				}
				if (eta_rt == 0)
				{
					gl_warning("eta_rt was not given -- using default value (battery: %d)",obj->id);
					eta_rt = 0.8; // roundtrip
				}
				break;
		}
		
		//figure out the size of the battery module
		if (battery_type == UNKNOWN){
			gl_warning("(battery: %s) the battery type is unknown. Using static voltage curve",obj->name);
		} else if (battery_type == LI_ION){
			n_series = floor(v_max/4.1);
		} else {//battery_type is LEAD_ACID
			//find lead acid cell spec
		}

		//Map up the various inverter properties for interfacing with the battery
		pSocReserve = map_double_value(parent,"soc_reserve");
		pSoc = map_double_value(parent,"battery_soc");
		pBatteryLoad = map_double_value(parent,"power_in");
		pRatedPower = map_double_value(parent,"rated_battery_power");

		//Map the control mode into the temporary variable
		temp_property_pointer = new gld_property(parent,"four_quadrant_control_mode");

		//Make sure it is proper
		if (!temp_property_pointer->is_valid() || !temp_property_pointer->is_enumeration())
		{
			GL_THROW("battery:%d - %s - Unable to map the parent inverter four_quadrant_control_mode",obj->id,(obj->name ? obj->name : "Unnamed"));
			/*  TROUBLESHOOT
			While attempting to map the four_quadrant_control_mode variable from the parent inverter, an issue occurred.  Please try again.
			If the error persists, please submit you model and description via the issue tracking system.
			*/
		}

		//Pull the value
		temp_value_control_mode = temp_property_pointer->get_enumeration();

		//Delete the property pointer
		delete temp_property_pointer;

		//Fetch the initial value of SocReserve for testing
		temp_value_SocReserve = pSocReserve->get_double();

		if(temp_value_SocReserve == 0){
			b_soc_reserve = 0;
		} else {
			b_soc_reserve = temp_value_SocReserve;
		}
		if(battery_state == BS_EMPTY && soc != b_soc_reserve){
			soc = b_soc_reserve;
		}
		if(battery_state == BS_FULL && soc != 1){
			soc = 1;
		}
		if(soc < 0){
			gl_warning("no initial state of charge given -- using default value (battery: %s)",obj->name);
			soc = 1;
			battery_state = BS_FULL;
		} else if(soc > 1){
			gl_warning("initial state of charge is greater than 1 setting to 1 (battery: %s)",obj->name);
			soc = 1;
			battery_state = BS_FULL;
		}

		// Enable battery delta mode flag if its parent control mode is FQM_CONSTANT_PQ
		if ((temp_value_control_mode == 1) || (temp_value_control_mode == 9)) {
			enableDelta = true;
		}
	}

	return 1; /* return 1 on success, 0 on failure */
}


gld::complex battery::calculate_v_terminal(gld::complex v, gld::complex i){
	return v - (i * Rinternal);
}

TIMESTAMP battery::rfb_event_time(TIMESTAMP t0, gld::complex power, double e){
	if((e < margin) && (power < 0)){
		return TS_NEVER;
	}
	if((e > E_Max - margin)&& (power > 0)){
		return TS_NEVER;
	}
	
	if(power < 0){ //dscharging
		double t1 = (e) / power.Re(); // time until depleted in hours
		t1 = t1 * 60 * 60; //time until depleted in seconds
		return t0 + (TIMESTAMP)(t1 * TS_SECOND);
	}else if(power > 0){ //charging
		double t1 = (E_Max - e) / power.Re(); // time until full in hours
		t1 = t1 * 60 * 60; //time until full in seconds
		return t0 + (TIMESTAMP)(t1 * TS_SECOND);
	}else{ //p ==0
		return TS_NEVER;
	}

}


double battery::calculate_efficiency(gld::complex voltage, gld::complex current){
	if(voltage.Mag() == 0 || current.Mag() == 0){
		return 1;
	}
	else if(current.Mag() < 5){
		return 0.95;
	}
	else if(current.Mag() < 10){
		return 0.9;
	}
	else if(current.Mag() < 20){
		return 0.8;
	}else{
		return 0.7;
	}

}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP battery::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	gld_wlock *test_rlock = nullptr;

	if(use_internal_battery_model){
			double dt;
		if(t0 != 0){

			//Update the reserve value
			b_soc_reserve = pSocReserve->get_double();
			if(battery_state == BS_DISCHARGING || battery_state == BS_CHARGING){
				dt = (double)(t1-t0);
				if(soc >= 0 && soc <= 1){
					soc += (internal_battery_load*dt/3600)/e_max;
					internal_battery_load = 0;
					if(soc <= 0){
						battery_state = BS_EMPTY;
						soc = 0;
					} else if(soc >= 1){
						battery_state = BS_FULL;
						soc = 1;
					} else if(soc <= b_soc_reserve && t1 == state_change_time){
						battery_state = BS_EMPTY;
						soc = b_soc_reserve;
					}
				}
			}
		}

		//Push the SOC up
		pSoc->setp<double>(soc,*test_rlock);
	}
	TIMESTAMP t2 = TS_NEVER;
	/* TODO: implement pre-topdown behavior */
	return t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

/* Sync is called when the clock needs to advance on the bottom-up pass */
TIMESTAMP battery::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	gld::complex temp_complex_value;
	OBJECT *obj = OBJECTHDR(this);

	//First run allocation
	if (first_run)	//First run
	{
		//TODO: LOCKING!
		if (deltamode_inclusive && enable_subsecond_models )	//We want deltamode - see if it's populated yet
		{
			if (((gen_object_current == -1) || (delta_objects==nullptr)) && enable_subsecond_models)
			{
				//Call the allocation routine
				allocate_deltamode_arrays();
			}

			//Check limits of the array
			if (gen_object_current>=gen_object_count)
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
			delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"interupdate_battery_object"));

			//Make sure it worked
			if (delta_functions[gen_object_current] == nullptr)
			{
				GL_THROW("Failure to map deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for postupdate
			post_delta_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"postupdate_battery_object"));

			//Make sure it worked
			if (post_delta_functions[gen_object_current] == nullptr)
			{
				GL_THROW("Failure to map post-deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the postupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Map up the function for preupdate
			delta_preupdate_functions[gen_object_current] = (FUNCTIONADDR)(gl_get_function(obj,"preupdate_battery_object"));

			//Make sure it worked
			if (delta_preupdate_functions[gen_object_current] == nullptr)
			{
				GL_THROW("Failure to map pre-deltamode function for device:%s",obj->name);
				/*  TROUBLESHOOT
				Attempts to map up the preupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			gen_object_current++;

		}//End deltamode specials - first pass
		//Default else - no deltamode stuff

		first_run = false;

		return t1; //Force us to reiterate one
	}//End first timestep

	if(!use_internal_battery_model){
		if (gen_mode_v == GM_POWER_DRIVEN || gen_mode_v == GM_POWER_VOLTAGE_HYBRID) 
		{
			if (number_of_phases_out == 3)
			{
				if (gen_mode_v == GM_POWER_VOLTAGE_HYBRID)
					GL_THROW("generator_mode POWER_VOLTAGE_HYBRID is not supported in 3-phase yet (only split phase is supported");

				gld::complex volt[3];
				TIMESTAMP dt,t_energy_limit;

				for (int i=0;i<3;i++)
				{
					volt[i] = pCircuit_V[i]->get_complex();
				}

				if (first_time_step == 0)
				{
					dt = 0;
				}
				else if (prev_time == 0)
				{
					prev_time = t1;
					dt = 0;
				}
				else if (prev_time < t1)
				{
					dt = t1 - prev_time;
					prev_time = t1;
				}
				else
					dt = 0;
				
				if (prev_state == -1) //discharge
				{
					Energy = Energy - (1 / base_efficiency) * power_transferred * (double)dt / 3600;
					if (Energy < 0)
						Energy = 0;
				}
				else if (prev_state == 1) //charge
					Energy = Energy + base_efficiency * power_transferred * (double)dt / 3600;

				//Pull the measured power
				temp_complex_value = pPower->get_complex();
				check_power = temp_complex_value.Mag();

				if (first_time_step > 0)
				{
					if (volt[0].Mag() > V_Max.Mag() || volt[1].Mag() > V_Max.Mag() || volt[2].Mag() > V_Max.Mag())
					{
						gl_warning("The voltages at the batteries meter are higher than rated. No power output.");
						battery_state = BS_WAITING;

						last_current[0].SetPolar(parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());
						
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						return TS_NEVER;
					}
					else if (check_power > power_set_high && Energy > 0) //discharge
					{
						prev_state = -1;
						battery_state = BS_DISCHARGING;

						double test_current = Max_P / (volt[0].Mag() + volt[1].Mag() + volt[2].Mag());
						if (test_current > I_Max.Mag()/3)
							test_current = I_Max.Mag()/3;

						last_current[0].SetPolar(-test_current + parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(-test_current + parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(-test_current + parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());
						
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						power_transferred = 0;

						for (int i=0;i<3;i++)
						{	
							power_transferred += last_current[i].Mag()*volt[i].Mag();
						}

						VA_Out = power_transferred;
						
						t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);
						if (t_energy_limit == 0)
							t_energy_limit = 1;

						return -(t1 + t_energy_limit);
					}
					else if (check_power < power_set_low && Energy < E_Max) //charge
					{
						prev_state = 1;
						battery_state = BS_CHARGING;

						double test_current = Max_P / (volt[0].Mag() + volt[1].Mag() + volt[2].Mag());
						if (test_current > I_Max.Mag()/3)
							test_current = I_Max.Mag()/3;

						last_current[0].SetPolar(test_current + parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(test_current + parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(test_current + parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());
						
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						power_transferred = 0;

						for (int i=0;i<3;i++)
						{	
							power_transferred += last_current[i].Mag()*volt[i].Mag();
						}
						
						VA_Out = power_transferred;

						t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
						if (t_energy_limit == 0)
							t_energy_limit = 1;

						return -(t1 + t_energy_limit);
					}
					else if ( (check_power < power_set_high - Max_P) && (prev_state == 1) && (Energy < E_Max) ) //keep charging until out of "deadband"
					{
						prev_state = 1; //charging
						battery_state = BS_CHARGING;

						double test_current = Max_P / (volt[0].Mag() + volt[1].Mag() + volt[2].Mag());
						if (test_current > I_Max.Mag()/3)
							test_current = I_Max.Mag()/3;

						last_current[0].SetPolar(test_current + parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(test_current + parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(test_current + parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());
						
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						power_transferred = 0;

						for (int i=0;i<3;i++)
						{	
							power_transferred += last_current[i].Mag()*volt[i].Mag();
						}
						
						VA_Out = power_transferred;

						t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
						if (t_energy_limit == 0)
							t_energy_limit = 1;

						return -(t1 + t_energy_limit);
					}
					else if ( (check_power > power_set_low + Max_P) && (prev_state == -1) && (Energy > 0) ) //keep discharging until out of "deadband"
					{
						prev_state = -1;
						battery_state = BS_DISCHARGING;
						
						double test_current = Max_P / (volt[0].Mag() + volt[1].Mag() + volt[2].Mag());
						if (test_current > I_Max.Mag()/3)
							test_current = I_Max.Mag()/3;

						last_current[0].SetPolar(-test_current + parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(-test_current + parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(-test_current + parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());
						
						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						power_transferred = 0;

						for (int i=0;i<3;i++)
						{	
							power_transferred += last_current[i].Mag()*volt[i].Mag();
						}

						VA_Out = -power_transferred;
						
						t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

						if (t_energy_limit == 0)
							t_energy_limit = 1;

						return -(t1 + t_energy_limit);
					}
					else
					{
						if (Energy <= 0)
							battery_state = BS_EMPTY;
						else if (Energy >= E_Max)
							battery_state = BS_FULL;
						else
							battery_state = BS_WAITING;

						last_current[0].SetPolar(parasitic_power_draw/3/volt[0].Mag(),volt[0].Arg());
						last_current[1].SetPolar(parasitic_power_draw/3/volt[1].Mag(),volt[1].Arg());
						last_current[2].SetPolar(parasitic_power_draw/3/volt[2].Mag(),volt[2].Arg());

						value_Line_I[0] = last_current[0];
						value_Line_I[1] = last_current[1];
						value_Line_I[2] = last_current[2];

						//Do a powerflow update
						push_powerflow_currents();

						prev_state = 0;
						power_transferred = 0;
						VA_Out = power_transferred;
						return TS_NEVER;
					}
				}
				else
				{
					if (Energy <= 0)
						battery_state = BS_EMPTY;
					else if (Energy >= E_Max)
						battery_state = BS_FULL;
					else
						battery_state = BS_WAITING;

					first_time_step = 1;
					return TS_NEVER;
				}
			} //End three phase GM_POWER_DRIVEN and HYBRID
			else if ( ((phases & 0x0010) == 0x0010) && (number_of_phases_out == 1) ) // Split-phase
			{
				gld::complex volt;
				TIMESTAMP dt,t_energy_limit;

				volt = pCircuit_V[0]->get_complex(); // 240-V circuit (we're assuming it can only be hooked here now

				if (first_time_step == 0)
				{
					dt = 0;
				}
				else if (prev_time == 0)
				{
					prev_time = t1;
					dt = 0;
				}
				else if (prev_time < t1)
				{
					dt = t1 - prev_time;
					prev_time = t1;
				}
				else
					dt = 0;
				
				if (prev_state == -1) //discharge
				{
					Energy = Energy - (1 / base_efficiency) * power_transferred * (double)dt / 3600;
					if (Energy < 0)
						Energy = 0;
				}
				else if (prev_state == 1) //charge
				{
					Energy = Energy + base_efficiency * power_transferred * (double)dt / 3600;
					if (Energy > E_Max)
						Energy = E_Max;
				}

				//Pull the power value
				temp_complex_value = pPower->get_complex();
				check_power = temp_complex_value.Mag();

				if (additional_controls == AC_LINEAR_TEMPERATURE)
				{
					double sens2 = (1 - sensitivity)/(-sensitivity);

					// high setpoint - high temperature
					double slope1_hi = power_set_high_highT / (high_temperature - midpoint_temperature * sens2);
					double yint1_hi = -midpoint_temperature * sens2 * slope1_hi;

					// high setpoint - low temperature
					double slope1_lo = (power_set_high - (slope1_hi * midpoint_temperature + yint1_hi)) / (low_temperature - midpoint_temperature);
					double yint1_lo = power_set_high - low_temperature * slope1_lo;

					// low setpoint - high temperature
					double slope2_hi = power_set_low_highT / (high_temperature - midpoint_temperature * sens2);
					double yint2_hi = -midpoint_temperature * sens2 * slope2_hi;

					// low setpoint - low temperature
					double slope2_lo = (power_set_low - (slope2_hi * midpoint_temperature + yint2_hi)) / (low_temperature - midpoint_temperature);
					double yint2_lo = power_set_low - low_temperature * slope2_lo;
				
					//Fetch the temperature
					if (climate_object_found)
					{
						value_Tout = pTout->get_double();
					}

					if (value_Tout > midpoint_temperature)
					{
						check_power_high = slope1_hi * value_Tout + yint1_hi;
						check_power_low = slope2_hi * value_Tout + yint2_hi;
					}
					else
					{
						check_power_high = slope1_lo * value_Tout + yint1_lo;
						check_power_low = slope2_lo * value_Tout + yint2_lo;
					}
				}
				else
				{
					check_power_high = power_set_high;
					check_power_low = power_set_low;
				}

				if (first_time_step > 0)
				{
					if (volt.Mag() > V_Max.Mag() || volt.Mag()/240 < 0.9 || volt.Mag()/240 > 1.1)
					{
						gl_verbose("The voltages at the batteries meter are higher than rated, or outside of ANSI emergency specifications. No power output.");
						battery_state = BS_WAITING;

						last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
						value_Line12 = last_current[0];

						//Do a powerflow update
						push_powerflow_currents();

						return TS_NEVER;
					}
					// if the power flowing through the transformer has exceeded our setpoint, or the voltage has dropped below
					// our setpoint (if in HYBRID mode), then start discharging to help out
					else if ((check_power > check_power_high || (volt.Mag() < voltage_set_low && gen_mode_v == GM_POWER_VOLTAGE_HYBRID)) && Energy > 0) //start discharging
					{
						if (volt.Mag() > voltage_set_high) // conflicting states between power and voltage control
						{
							if (prev_state != 0)
								no_of_cycles += 1;

							last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
							value_Line12 = last_current[0];
							
							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = 0;
							battery_state = BS_CONFLICTED;

							return TS_NEVER;
						}
						else
						{
							if (prev_state != -1)
								no_of_cycles +=1;

							prev_state = -1;  //discharging
							battery_state = BS_DISCHARGING;
							double power_desired = check_power - check_power_high;
							if (power_desired <= 0) // we're in voltage support mode
							{
								last_current[0].SetPolar(-I_Max.Mag(),volt.Arg());
								value_Line12 = last_current[0]; //generation, pure real power
								
							}
							else // We're load following
							{
								if (power_desired >= Max_P)
									power_desired = Max_P;

								double current_desired = -power_desired/volt.Mag();
								last_current[0].SetPolar(current_desired,volt.Arg());
								value_Line12 = last_current[0];
							}

							//Do a powerflow update
							push_powerflow_currents();
							
							power_transferred = last_current[0].Mag()*volt.Mag();

							VA_Out = power_transferred;
							
							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);
							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
					// If the power has dropped below our setpoint, or voltage has risen to above our setpoint,
					// then start charging to help lower voltage and/or increase power through the transformer
					else if ((check_power < check_power_low || (gen_mode_v == GM_POWER_VOLTAGE_HYBRID && volt.Mag() > voltage_set_high)) && Energy < E_Max) //charge
					{
						if (volt.Mag() < voltage_set_low) // conflicting states between power and voltage control
						{
							if (prev_state != 0)
								no_of_cycles += 1;
							last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = 0;
							battery_state = BS_CONFLICTED;

							return TS_NEVER;
						}
						else
						{
							if (prev_state != 1)
								no_of_cycles +=1;

							prev_state = 1;  //charging
							battery_state = BS_CHARGING;
							double power_desired = check_power_low - check_power;

							if (power_desired <= 0) // We're in voltage management mode
							{
								last_current[0].SetPolar(I_Max.Mag(),volt.Arg());
								value_Line12 = last_current[0]; //load, pure real power
								
							}
							else // We're in load tracking mode
							{
								if (power_desired >= Max_P)
									power_desired = Max_P;

								double current_desired = power_desired/volt.Mag();
								last_current[0].SetPolar(current_desired,volt.Arg());
								value_Line12 = last_current[0];
							}
			
							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = last_current[0].Mag()*volt.Mag();
							
							VA_Out = power_transferred;

							t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
					//keep charging until out of "deadband"
					else if ((check_power < check_power_low || (gen_mode_v == GM_POWER_VOLTAGE_HYBRID && volt.Mag() > voltage_set_high - deadband))&& prev_state == 1 && Energy < E_Max) 
					{
						if (volt.Mag() < voltage_set_low) // conflicting states between power and voltage control
						{
							if (prev_state != 0)
								no_of_cycles += 1;

							last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = 0;
							battery_state = BS_CONFLICTED;

							return TS_NEVER;
						}
						else
						{
							if (prev_state != 1)
								no_of_cycles +=1;

							prev_state = 1; //charging
							battery_state = BS_CHARGING;

							double power_desired = check_power_low - check_power;

							if (power_desired <= 0) // We're in voltage management mode
							{
								last_current[0].SetPolar(I_Max.Mag(),volt.Arg());
								value_Line12 = last_current[0]; //load, pure real power
								
							}
							else // We're in load tracking mode
							{
								if (power_desired >= Max_P)
									power_desired = Max_P;

								double current_desired = power_desired/volt.Mag();
								last_current[0].SetPolar(current_desired, volt.Arg());
								value_Line12 = last_current[0];
							}
			
							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = last_current[0].Mag()*volt.Mag();
							
							VA_Out = power_transferred;

							t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);
							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
					//keep discharging until out of "deadband"
					else if ((check_power > check_power_high || (gen_mode_v == GM_POWER_VOLTAGE_HYBRID && volt.Mag() < voltage_set_low + deadband)) && prev_state == -1 && Energy > 0) 
					{
						if (volt.Mag() > voltage_set_high) // conflicting states between power and voltage control
						{
							if (prev_state != 0)
								no_of_cycles += 1;

							last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = 0;
							battery_state = BS_CONFLICTED;

							return TS_NEVER;
						}
						else
						{
							if (prev_state != -1)
								no_of_cycles +=1;

							prev_state = -1; //discharging
							battery_state = BS_DISCHARGING;

							double power_desired = check_power - check_power_high;
							if (power_desired <= 0) // we're in voltage support mode
							{
								last_current[0].SetPolar(-I_Max.Mag(),volt.Arg());
								value_Line12 = last_current[0]; //generation, pure real power
								
							}
							else // We're load following
							{
								if (power_desired >= Max_P)
									power_desired = Max_P;

								double current_desired = -power_desired/volt.Mag();
								last_current[0].SetPolar(current_desired,volt.Arg());
								value_Line12 = last_current[0];						
							}

							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = last_current[0].Mag()*volt.Mag();

							VA_Out = power_transferred;
							
							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
					else
					{
						if (prev_state != 0)
							no_of_cycles +=1;

						last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
						value_Line12 = last_current[0];

						//Do a powerflow update
						push_powerflow_currents();

						prev_state = 0;
						if (Energy <= 0)
							battery_state = BS_EMPTY;
						else if (Energy >= E_Max)
							battery_state = BS_FULL;
						else
							battery_state = BS_WAITING;

						power_transferred = 0;
						VA_Out = power_transferred;
						return TS_NEVER;
					}
				}
				else
				{
					if (Energy <= 0)
						battery_state = BS_EMPTY;
					else if (Energy >= E_Max)
						battery_state = BS_FULL;
					else
						battery_state = BS_WAITING;

					first_time_step = 1;
					return TS_NEVER;
				}
			} //End split phase GM_POWER_DRIVEN and HYBRID
		}// End GM_POWER_DRIVEN and HYBRID controls
		else if (gen_mode_v == GM_CONSTANT_PQ)
		{
			if (number_of_phases_out == 3) //Three phase meter
			{
				gld::complex volt[3];
				double high_voltage = 0, low_voltage = 999999999;
				TIMESTAMP dt,t_energy_limit;

				for (int jj=0; jj<3; jj++)
				{
					if (parent_is_meter)
					{
						value_Circuit_V[jj] = pCircuit_V[jj]->get_complex();
					}

					volt[jj] = value_Circuit_V[jj]; // 240-V circuit (we're assuming it can only be hooked here now)
					if (volt[jj].Mag() > high_voltage)
						high_voltage = volt[jj].Mag();
					if (volt[jj].Mag() < low_voltage)
						low_voltage = volt[jj].Mag();
				}
				if (first_time_step == 0)
				{
					dt = 0;
				}
				else if (prev_time == 0)
				{
					prev_time = t1;
					dt = 0;
				}
				else if (prev_time < t1)
				{
					dt = t1 - prev_time;
					prev_time = t1;
				}
				else
					dt = 0;
				
				if (prev_state == -1) //discharge
				{
					Energy = Energy - (1 / base_efficiency) * power_transferred * (double)dt / 3600;
					if (Energy < 0)
						Energy = 0;
				}
				else if (prev_state == 1) //charge
				{
					Energy = Energy + base_efficiency * power_transferred * (double)dt / 3600;
					if (Energy > E_Max)
						Energy = E_Max;
				}

				if (first_time_step > 0)
				{
					if (high_voltage > V_Max.Mag())
					{
						gl_verbose("The voltages at the battery meter are higher than rated. No power output.");
						battery_state = BS_WAITING;
						prev_state = 0;

						for (int jj=0; jj<3; jj++)
						{
							last_current[jj].SetPolar(parasitic_power_draw/3/volt[jj].Mag(),volt[jj].Arg());
							value_Line_I[jj] = last_current[jj];
						}

						//Do a powerflow update
						push_powerflow_currents();
						
						return TS_NEVER;
					}
					else // We're within the voltage limits, so follow the schedule
					{
						double power_desired;
						if (fabs(B_scheduled_power) < fabs(Max_P))
							 power_desired = B_scheduled_power;
						else
							power_desired = Max_P;

						if (power_desired < 0.0 && Energy < E_Max)
						{
							battery_state = BS_CHARGING;
							prev_state = 1;

							VA_Out = power_transferred = 0;
							for (int jj=0; jj<3; jj++)
							{
								last_current[jj].SetPolar((-power_desired + parasitic_power_draw)/3/volt[jj].Mag(),volt[jj].Arg());
								value_Line_I[jj] = last_current[jj];
								VA_Out += last_current[jj].Mag()*volt[jj].Mag();
							}					

							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = VA_Out.Mag();

							t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else if (power_desired > 0.0 && Energy > 0.0)
						{
							battery_state = BS_DISCHARGING;
							prev_state = -1;

							VA_Out = power_transferred = 0;
							for (int jj=0; jj<3; jj++)
							{
								last_current[jj].SetPolar((-power_desired + parasitic_power_draw)/3/volt[jj].Mag(),volt[jj].Arg());
								value_Line_I[jj] = last_current[jj];
								VA_Out += last_current[jj].Mag()*volt[jj].Mag();
							}					
							
							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = VA_Out.Mag();

							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else if (Energy <= 0.0)
						{
							battery_state = BS_EMPTY;
							prev_state = 0;

							VA_Out = power_transferred = 0.0;
							last_current[0] = last_current[1] = last_current[2] = gld::complex(0,0);
							return TS_NEVER;
						}
						else if (Energy >= E_Max)
						{
							battery_state = BS_FULL;
							prev_state = -1;

							VA_Out = power_transferred = 0;
							for (int jj=0; jj<3; jj++)
							{
								last_current[jj].SetPolar((parasitic_power_draw)/3/volt[jj].Mag(),volt[jj].Arg());
								value_Line_I[jj] = last_current[jj];
								VA_Out += last_current[jj].Mag()*volt[jj].Mag();
							}					
							
							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = VA_Out.Mag();
							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else
						{
							battery_state = BS_WAITING;
							prev_state = -1;

							VA_Out = power_transferred = 0;
							for (int jj=0; jj<3; jj++)
							{
								last_current[jj].SetPolar((parasitic_power_draw)/3/volt[jj].Mag(),volt[jj].Arg());
								value_Line_I[jj] = last_current[jj];
								VA_Out += last_current[jj].Mag()*volt[jj].Mag();
							}					
							
							//Do a powerflow update
							push_powerflow_currents();

							power_transferred = VA_Out.Mag();

							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
				}
				else
				{
					if (Energy <= 0)
					{
						battery_state = BS_EMPTY;
						prev_state = 0;
					}
					else if (Energy >= E_Max)
					{
						battery_state = BS_FULL;
						prev_state = -1;
					}
					else
					{
						battery_state = BS_WAITING;
						prev_state = 0;
					}

					first_time_step = 1;
					return TS_NEVER;
				}
			}// End 3-phase meter
			else if ( ((phases & 0x0010) == 0x0010) && (number_of_phases_out == 1) ) // Split-phase
			{
				gld::complex volt;
				TIMESTAMP dt,t_energy_limit;

				if (parent_is_meter && parent_is_triplex)
				{
					value_Circuit_V[0] = pCircuit_V[0]->get_complex();
				}

				volt = value_Circuit_V[0]; // 240-V circuit (we're assuming it can only be hooked here now)

				if (first_time_step == 0)
				{
					dt = 0;
				}
				else if (prev_time == 0)
				{
					prev_time = t1;
					dt = 0;
				}
				else if (prev_time < t1)
				{
					dt = t1 - prev_time;
					prev_time = t1;
				}
				else
					dt = 0;
				
				if (prev_state == -1) //discharge
				{
					Energy = Energy - (1 / base_efficiency) * power_transferred * (double)dt / 3600;
					if (Energy < 0)
						Energy = 0;
				}
				else if (prev_state == 1) //charge
				{
					Energy = Energy + base_efficiency * power_transferred * (double)dt / 3600;
					if (Energy > E_Max)
						Energy = E_Max;
				}

				if (first_time_step > 0)
				{
					if (volt.Mag() > V_Max.Mag() || volt.Mag()/240 < 0.9 || volt.Mag()/240 > 1.1)
					{
						gl_verbose("The voltages at the battery meter are higher than rated, or outside of ANSI emergency specifications. No power output.");
						battery_state = BS_WAITING;
						prev_state = -1;

						last_current[0].SetPolar(parasitic_power_draw/volt.Mag(),volt.Arg());
						value_Line12 = last_current[0];

						//Do a powerflow update
						push_powerflow_currents();

						return TS_NEVER;
					}
					else 
					{
						double power_desired;
						if (fabs(B_scheduled_power) < fabs(Max_P))
							 power_desired = B_scheduled_power;
						else
							power_desired = Max_P;

						if (power_desired < 0.0 && Energy < E_Max)
						{
							battery_state = BS_CHARGING;
							prev_state = 1;

							double current_desired = -(power_desired - parasitic_power_draw) / volt.Mag();
							last_current[0].SetPolar(current_desired,volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = last_current[0].Mag()*volt.Mag();

							t_energy_limit = TIMESTAMP(3600 * (E_Max - Energy) / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else if (power_desired > 0.0 && Energy > 0.0)
						{
							battery_state = BS_DISCHARGING;
							prev_state = -1;

							double current_desired = -(power_desired - parasitic_power_draw) / volt.Mag();
							last_current[0].SetPolar(current_desired,volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = last_current[0].Mag()*volt.Mag();

							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else if (Energy <= 0.0)
						{
							battery_state = BS_EMPTY;
							prev_state = 0;

							VA_Out = power_transferred = 0.0;
							last_current[0] = gld::complex(0,0);
							return TS_NEVER;
						}
						else if (Energy >= E_Max)
						{
							battery_state = BS_FULL;
							prev_state = -1;

							double current_desired = (parasitic_power_draw) / volt.Mag();
							last_current[0].SetPolar(current_desired,volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = last_current[0].Mag()*volt.Mag();

							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
						else
						{
							battery_state = BS_WAITING;
							prev_state = -1;

							double current_desired = (parasitic_power_draw) / volt.Mag();
							last_current[0].SetPolar(current_desired,volt.Arg());
							value_Line12 = last_current[0];

							//Do a powerflow update
							push_powerflow_currents();

							VA_Out = power_transferred = last_current[0].Mag()*volt.Mag();

							t_energy_limit = TIMESTAMP(3600 * Energy / power_transferred);

							if (t_energy_limit == 0)
								t_energy_limit = 1;

							return -(t1 + t_energy_limit);
						}
					}
				}
				else
				{
					if (Energy <= 0)
					{
						battery_state = BS_EMPTY;
						prev_state = 0;
					}
					else if (Energy >= E_Max)
					{
						battery_state = BS_FULL;
						prev_state = -1;
					}
					else
					{
						battery_state = BS_WAITING;
						prev_state = 0;
					}

					first_time_step = 1;
					return TS_NEVER;
				}
			}// End Split-phase
		}// End Constant_PQ
		else
		{
			//gather V_Out
			//gather I_Out
			//gather P_Out
			////gl_verbose("battery sync: entered");

			//See if the property was valid (since not sure what we are here -- meter or inverter)
			if (pCircuit_V[0] != nullptr)
			{
				if (parent_is_inverter)
				{
					value_Circuit_V[0] = complex(pCircuit_V[0]->get_double(), 0.0);
				}
				else
				{
					value_Circuit_V[0] = pCircuit_V[0]->get_complex();
				}
			}

			if (pLine_I[0] != nullptr)
			{
				if (parent_is_inverter)
				{
					value_Line_I[0] = complex(pLine_I[0]->get_double(), 0.0);
				}
				else
				{
					value_Line_I[0] = pLine_I[0]->get_complex();
				}
			}

			V_Out = value_Circuit_V[0];
			I_Out = value_Line_I[0];

			//gl_verbose("battery sync: V_Out from parent is: (%f , %f)", V_Out.Re(), V_Out.Im());
			//gl_verbose("battery sync: I_Out from parent is: (%f , %f)", I_Out.Re(), V_Out.Im());

			V_Internal = calculate_v_terminal(V_Out, I_Out);
			//V_Out = V_internal;

			//finds the CURRENT efficiency
			efficiency = base_efficiency * calculate_efficiency(V_Out, I_Out);

			//gl_verbose("battery sync: base_efficiency is: %f", base_efficiency);
			//gl_verbose("battery sync: efficiency is: %f", efficiency);

			if (I_Out > 0){ // charging
				I_Internal = I_Out * efficiency; //actual stored current is less than what was put in
			}

			if(I_Out < 0){ // discharging
				I_Internal = I_Out / efficiency; //actual released current is more than what is obtained by parent
			}


			//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
			//gl_verbose("battery sync: V_Internal to push is: (%f , %f)", V_Internal.Re(), V_Internal.Im());
			//gl_verbose("battery sync: I_Internal to push is: (%f , %f)", I_Internal.Re(), I_Internal.Im());

			VA_Out = V_Out * (~I_Out);
			VA_Internal = V_Internal * I_Internal;  // charging rate is slower than termainl voltage suggests, similarly,
												//discharge rate is faster than the terminal voltage suggests
				
			//gl_verbose("battery sync: VA_Out from parent is: (%f , %f)", VA_Out.Re(), VA_Out.Im());
			//gl_verbose("battery sync: VA_Internal calculated is: (%f , %f)", VA_Internal.Re(), VA_Internal.Im());
			

			if(!recalculate){//if forced recalculate is false, check where the time is
			//gl_verbose("battery sync: don't recalculate");
				if(t0 == prev_time){
					//gl_verbose("battery sync: reached expected time, set energy and don't recalculate");
					Energy = E_Next; // we're all set this time around, just set the energy level onward
					//gl_verbose("battery sync: Energy level is %f", Energy);
					//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
					//gl_verbose("battery sync: I_In to push is: (%f , %f)", I_In.Re(), I_In.Im());
					recalculate = true; //recalculate the next time around
					
					return battery::sync(t0, t1);
				}else{
					//gl_verbose("battery sync: Didn't reach expected time, force recalculate");
					//gl_verbose("battery sync: V_In to push is: (%f , %f)", V_In.Re(), V_In.Im());
					//gl_verbose("battery sync: I_In to push is: (%f , %f)", I_In.Re(), I_In.Im());
					Energy = E_Next;
					//TIMESTAMP prev_time2 = prev_time;
					//prev_time = t0;
					recalculate = true; //set forced recalculate to true becuase we've reached an unexpected time
					//return battery::sync(prev_time2, t0);
					return battery::sync(prev_time, t1);
				}
				
			}else{
				//////////////////// need this in hours //////////////////
				//TIMESTAMP t2 = t1 - t0;
				//gl_verbose("battery sync: recalculate!");
				//gl_verbose("battery sync: energy level pre changes is: %f", Energy);
				//gl_verbose("battery sync: energy level to go to next is: %f", E_Next);

				///need to use timestamp_to_hours from the core, not here, how to reference core?
				//double t2 = (timestamp_to_hours((TIMESTAMP)t1) - timestamp_to_hours((TIMESTAMP)t0));
				double t2 = (gl_tohours((TIMESTAMP)t1) - gl_tohours((TIMESTAMP)t0));

				if(fabs((double)V_Out.Re()) > fabs((double)V_Max.Re())){
					//gl_verbose("battery sync: V_Out exceeded allowable V_Out, setting to max");
					V_Out = V_Max;
					V_Internal = V_Out - (I_Out * Rinternal);
				}

				if(fabs((double)I_Out.Re()) > fabs((double)I_Max.Re())){
					//gl_verbose("battery sync: I_Out exceeded allowable I_Out, setting to max");
					I_Out = I_Max;
				}

				if(fabs((double)VA_Out.Re()) > fabs((double)Max_P)){
					//gl_verbose("battery sync: VA_Out exceeded allowable VA_Out, setting to max");
					VA_Out = gld::complex(Max_P , 0);
					VA_Internal = VA_Out - (I_Out * I_Out * Rinternal);
				}



			prev_time = t1;

			if(VA_Out < 0){ //discharging
				//gl_verbose("battery sync: discharging");
				if(Energy == 0 || Energy <= margin){ 
					//gl_verbose("battery sync: battery is empty!");
					// if(connected){//Connected was always true
						//gl_verbose("battery sync: empty BUT it is connected, passing request onward");
						I_In = I_Max + gld::complex(fabs(I_Out.Re()), fabs(I_Out.Im())); //power was asked for to discharge but battery is empty, forward request along the line
						I_Prev = I_Max / efficiency;
						//Get as much as you can from the microturbine --> the load asked for as well as the max
						recalculate = false;
						E_Next = Energy + (((I_In - gld::complex(fabs(I_Out.Re()), fabs(I_Out.Im()))) * V_Internal / efficiency) * t2).Re();  // the energy level at t1
						TIMESTAMP t3 = rfb_event_time(t0, (I_In - gld::complex(fabs(I_Out.Re()), fabs(I_Out.Im()))) * V_Internal / efficiency, Energy);
						return t3;
					// }
					// else{
					// 	//gl_verbose("battery sync: battery is empty with nothing connected!  drop request!");
					// 	I_In = 0;
					// 	I_Prev = 0;
					// 	V_In = V_Out;
					// 	VA_Out = 0;
					// 	E_Next = 0;
					// 	recalculate = false;
					// 	return TS_NEVER;
					// }
				}

				if((Energy + (V_Internal * I_Prev.Re()).Re() * t2) <= margin){ //headed to empty
					//gl_verbose("battery sync: battery is headed to empty");
					// if(connected){//Connected was always true
						//gl_verbose("battery sync: BUT battery is connected, so pass request onward");
						I_In = I_Max + gld::complex(fabs(I_Out.Re()), fabs(I_Out.Im())); //this won't let the battery go empty... change course
						I_Prev = I_Max / efficiency;
						//if the battery is connected to something which serves the load and charges the battery
						E_Next = margin;
						recalculate = false;
						TIMESTAMP t3 = rfb_event_time(t0, (I_In - gld::complex(fabs(I_Out.Re()), fabs(I_Out.Im()))) * V_Internal / efficiency, Energy);
						return t3;
					// }else{
					// 	//gl_verbose("battery sync: battery is about to be empty with nothing connected!!");
					// 	TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					// 	E_Next = 0; //expecting return when time is empty
					// 	I_In = 0;
					// 	I_Prev = 0;
					// 	recalculate = false;
					// 	return t3;
					// }
				}else{ // doing fine
					//gl_verbose("battery sync: battery is not empty, demand supplied from the battery");
					E_Next = Energy + (VA_Internal.Re() * t2);
					I_In = 0; //nothign asked for
					I_Prev = 0;
					recalculate = false;
					TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					return t3;
				}
			}else if (VA_Out > 0){ //charging
				if(Energy >= (E_Max - margin)){
					//gl_verbose("battery sync: battery is full!");
					// if(connected){//Connected was always true
						//attempt to let other items serve the load if the battery is full instead of draining the battery
						//gl_verbose("battery sync: battery is full and connected, passing the power request onward");
						E_Next = Energy;
						I_In = I_Out = 0; //drop the request to charge
						I_Prev = 0;
						recalculate = false;
						return TS_NEVER;
					// }else{
					// 	//gl_verbose("battery sync: battery is full, and charging");
					// 	I_In = I_Out = 0; //can't charge any more... drop the current somehow?
					// 	I_Prev = 0;
					// 	V_Out = V_Out; // don't drop V_Out in this case
					// 	VA_Out = 0;
					// 	E_Next = Energy;
					// 	return TS_NEVER;
					// }
				}

				if(Energy + ((V_Internal * I_Prev.Re()) * efficiency * t2).Re() >= (E_Max - margin)){ //if it is this far, it is charging at max
					//gl_verbose("battery sync: battery is about to be full");
					TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					I_In = 0;
					I_Prev = 0;
					E_Next = E_Max - margin;
					recalculate = false;
					return t3;
				}else{
					// if(connected){	//Connected was always true
						//gl_verbose("battery sync: battery is charging but not yet full, connected");
						//if it is connected, use whatever is connected to help it charge;
						I_In = I_Max - I_Out; // total current in is now I_Max
						I_Prev = I_Max * efficiency;
						E_Next = Energy + ((I_Max * V_Internal) * efficiency * t2).Re();
						recalculate = false;
						TIMESTAMP t3 = rfb_event_time(t0, (I_Max * V_Internal * efficiency), Energy);
						return t3;
					// }else{
					// 	//gl_verbose("battery sync: battery is charging but not yet full, not connected");
					// 	I_In = 0;
					// 	I_Prev = 0;
					// 	E_Next = Energy + (VA_Internal * t2).Re();
					// 	recalculate = false;
					// 	TIMESTAMP t3 = rfb_event_time(t0, VA_Internal, Energy);
					// 	return t3;
					// }
				}
			}else{// VA_Out = 0
				//gl_verbose("battery sync: battery is not charging or discharging");
				recalculate = false;
				I_In = 0;
				I_Prev = 0;
				E_Next = Energy;
				return TS_NEVER;
			}

			}
		}
	}
	return TS_NEVER;
}

/* Postsync is called when the clock needs to advance on the second top-down pass */
TIMESTAMP battery::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	double temp_double_value;

	if(!use_internal_battery_model){
		TIMESTAMP result;

		Iteration_Toggle = !Iteration_Toggle;
		if (number_of_phases_out == 3)//has all three phases
		{
			value_Line_I[0] = -last_current[0];
			value_Line_I[1] = -last_current[1];
			value_Line_I[2] = -last_current[2];
		}
		else if ( ((phases & 0x0010) == 0x0010) && (number_of_phases_out == 1) )
		{
			value_Line12 = -last_current[0];
		}
		result = TS_NEVER;

		//Do a powerflow update
		push_powerflow_currents();

		if (Iteration_Toggle)
			return result;
		else
			return TS_NEVER;
	} else {//use_internal_battery_model is true
		//Pull the load
		temp_double_value = pBatteryLoad->get_double();
		bat_load = -temp_double_value;
		p_max = pRatedPower->get_double();
		//figure out the the actual power coming out of the battery due to the battery load and the battery efficiency
		if(t0 != 0 && bat_load != 0){
			if(bat_load < 0 && battery_state != BS_EMPTY){
				if(bat_load < -p_max){
					gl_warning("battery_load is greater than rated. Setting to plate rating.");
					bat_load = -p_max;
				}
				battery_state = BS_DISCHARGING;
				p_br = p_max/pow(eta_rt,0.5);
			} else if(bat_load > 0 && battery_state != BS_FULL){
				if(bat_load > p_max){
					gl_warning("battery_load is greater than rated. Setting to plate rating.");
					bat_load = p_max;
				}
				battery_state = BS_CHARGING;
				p_br = p_max*pow(eta_rt,0.5);
			} else {
				return TS_NEVER;
			}
			if(battery_type == LI_ION){
				if(soc <= 1 && soc > 0.1){
					v_oc = n_series*(((4.1-3.6)/0.9)*soc + (4.1-((4.1-3.6)/0.9)));//voltage curve for sony
				} else if(soc <= 0.1 && soc >= 0){
					v_oc = n_series*(((3.6-3.2)/0.1)*soc + 3.2);
				}
			} else if(battery_type == LEAD_ACID){
				v_oc = v_max;// no voltage curve for LEAD_ACID using static voltage
			} else {//unknown battery type
				v_oc = v_max;
			}
			r_in = v_oc*v_oc*fabs(p_br-p_max)/(p_br*p_br);
			v_t = (v_oc+pow((v_oc*v_oc+(4*bat_load*r_in)),0.5))/2;
			internal_battery_load = v_oc*bat_load/v_t;

			//Pull the value
			b_soc_reserve = pSocReserve->get_double();
			if(internal_battery_load < 0){
				state_change_time = t1 + (TIMESTAMP)ceil((b_soc_reserve-soc)*e_max*3600/internal_battery_load);
			} else if(internal_battery_load > 0){
				state_change_time = t1 + (TIMESTAMP)ceil((1-soc)*e_max*3600/internal_battery_load);
			}
			return state_change_time;
		}
		return TS_NEVER;
	}
}

//Map Complex value
gld_property *battery::map_complex_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_complex())
	{
		GL_THROW("battery:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in battery.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Map double value
gld_property *battery::map_double_value(OBJECT *obj, const char *name)
{
	gld_property *pQuantity;
	OBJECT *objhdr = OBJECTHDR(this);

	//Map to the property of interest
	pQuantity = new gld_property(obj,name);

	//Make sure it worked
	if (!pQuantity->is_valid() || !pQuantity->is_double())
	{
		GL_THROW("battery:%d %s - Unable to map property %s from object:%d %s",objhdr->id,(objhdr->name ? objhdr->name : "Unnamed"),name,obj->id,(obj->name ? obj->name : "Unnamed"));
		/*  TROUBLESHOOT
		While attempting to map a quantity from another object, an error occurred in battery.  Please try again.
		If the error persists, please submit your system and a bug report via the ticketing system.
		*/
	}

	//return the pointer
	return pQuantity;
}

//Function to resynch the various current accumulators
//Unlike other objects, meter checks are inside (mostly for copy-paste laziness above)
void battery::push_powerflow_currents(void)
{
	gld::complex temp_complex_val;
	gld_wlock *test_rlock = nullptr;
	int indexval;

	if (parent_is_meter)
	{
		//See if it is triplex
		if (parent_is_triplex)	//It is
		{
			//Shouldn't need to NULL check this one, but do it anyways, for consistency
			if (pLine12 != nullptr)
			{
				//**** Current value ***/
				//Pull current value again, just in case
				temp_complex_val = pLine12->get_complex();

				//Add the difference
				temp_complex_val += value_Line12;

				//Push it back up
				pLine12->setp<gld::complex>(temp_complex_val,*test_rlock);
			}//End pLine_I valid
			//Default else -- it's null, so skip it
		}//End is triplex
		else	//Three-phase, or a variant...theoretically
		{
			for (indexval=0; indexval<3; indexval++)
			{
				//Null check them, out of paranoia
				if (pLine_I[indexval] != nullptr)
				{
					//**** Current value ***/
					//Pull current value again, just in case
					temp_complex_val = pLine_I[indexval]->get_complex();

					//Add the difference
					temp_complex_val += value_Line_I[indexval];

					//Push it back up
					pLine_I[indexval]->setp<gld::complex>(temp_complex_val,*test_rlock);
				}//End pLine_I valid
				//Default else -- it's null, so skip it
			}
		}//End not-triplex
	}//End is meter
	//Not a meter, so do nothing?
}
//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////

//Preupdate
STATUS battery::pre_deltaupdate(TIMESTAMP t0, unsigned int64 delta_time)
{
	STATUS stat_val;
	OBJECT *obj = OBJECTHDR(this);

	// Battery delta mode operation is only when enableDelta is true
	if (!enableDelta) {
		// Do nothing for battery during delta mode - soc is not changed
		gl_warning("battery:%s does not use internal_battery_model, or its parent inverter is not FQM_CONSTANT_PQ. No actions executed for this battery during delta mode",obj->name?obj->name:"unnamed");

	}
	//Just return a pass - not sure how we'd fail
	return SUCCESS;
}

//Module-level call
SIMULATIONMODE battery::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	SIMULATIONMODE simmode_return_value = SM_EVENT;

	//Get timestep value
	deltat = (double)dt/(double)DT_SECOND;

	if(enableDelta){

		// Initialization - update the soc based on inverter output before entering the delta mode
		if ((delta_time==0) && (iteration_count_val==0))	//First run of new delta call
		{
			// Update soc based on internal_battery_load value obtained before entering the delta mode
			update_soc(delta_time);

			// Then have to check the delta time step that the soc will be out of limit && update internal_battery_load
			state_change_time_delta = check_state_change_time_delta(delta_time, dt);

			simmode_return_value =  SM_DELTA_ITER; // iterate since inverter is doing so
		}
		// Need to update soc at iteration_count_val == 0, so that later at iteration_count_val == 1, inverter can check this soc value
		else if ((delta_time != 0) && (iteration_count_val == 0)) {

			// Update soc based on internal_battery_load value obtained previously
			update_soc(delta_time);

			simmode_return_value =  SM_DELTA_ITER;
		}

		// Based on the inverter output calculated at iteration_count_val == 1, update internal_battery_load value
		else if ((delta_time != 0) && (iteration_count_val == 1)) {

			// Check the delta time step that the soc will be out of limit && update internal_battery_load
			state_change_time_delta = check_state_change_time_delta(delta_time, dt);

			simmode_return_value = SM_EVENT; // Just return event-driven mode - inverter/other will drive it forward

		}

	} // End if enableDelta == true

	// else: Do nothing for the battery during delta mode if it is not use_internal_battery_mode and its parent inverter is not PQ_CONSTANT

	return simmode_return_value;

}

void battery::update_soc(unsigned int64 delta_time)
{
	gld_wlock *test_rlock = nullptr;

	b_soc_reserve = pSocReserve->get_double();
	if(battery_state == BS_DISCHARGING || battery_state == BS_CHARGING){
		if(soc >= 0 && soc <= 1){
			soc += (internal_battery_load*deltat/3600)/e_max;
			internal_battery_load = 0;
			if(soc <= 0){
				battery_state = BS_EMPTY;
				soc = 0;
			} else if(soc >= 1){
				battery_state = BS_FULL;
				soc = 1;
			} else if(soc <= b_soc_reserve && delta_time >= state_change_time_delta){
				battery_state = BS_EMPTY;
				soc = b_soc_reserve;
			}
		}
	}
	pre_soc = soc;
	//Push the SOC up
	pSoc->setp<double>(soc,*test_rlock);
}

double battery::check_state_change_time_delta(unsigned int64 delta_time, unsigned long dt)
{
	double time_return;
	gld::complex inv_VA_out_value;
	double inv_eff_value;

	//Retrieve the inverter properties
	if (parent_is_inverter)
	{
		inv_VA_out_value = complex(pinverter_VA_Out->get_double(), 0.0);
		inv_eff_value = peff->get_double();
	}

	// Update battery output based on true inverter output
	if (inv_VA_out_value.Re() > 0.0)	//Discharging
	{
		Pout_delta = inv_VA_out_value.Re()/inv_eff_value;
	}
	else if (inv_VA_out_value.Re() == 0.0)	//Idle
	{
		Pout_delta = 0.0;
	}
	else	//Must be positive, so charging
	{
		Pout_delta = inv_VA_out_value.Re()*inv_eff_value;
	}

	// Use the battery output to calculate expected time reaching soc limit && update internal_battery_load
	bat_load = -(Pout_delta);
	p_max = pRatedPower->get_double();
	//figure out the the actual power coming out of the battery due to the battery load and the battery efficiency
	if(bat_load != 0){
		if(bat_load < 0 && battery_state != BS_EMPTY){
			if(bat_load < -p_max){
				gl_warning("battery_load is greater than rated. Setting to plate rating.");
				bat_load = -p_max;
			}
			battery_state = BS_DISCHARGING;
			p_br = p_max/pow(eta_rt,0.5);
		} else if(bat_load > 0 && battery_state != BS_FULL){
			if(bat_load > p_max){
				gl_warning("battery_load is greater than rated. Setting to plate rating.");
				bat_load = p_max;
			}
			battery_state = BS_CHARGING;
			p_br = p_max*pow(eta_rt,0.5);
		} else {
			return TS_NEVER;
		}
		if(battery_type == LI_ION){
			if(soc <= 1 && soc > 0.1){
				v_oc = n_series*(((4.1-3.6)/0.9)*soc + (4.1-((4.1-3.6)/0.9)));//voltage curve for sony
			} else if(soc <= 0.1 && soc >= 0){
				v_oc = n_series*(((3.6-3.2)/0.1)*soc + 3.2);
			}
		} else if(battery_type == LEAD_ACID){
			v_oc = v_max;// no voltage curve for LEAD_ACID using static voltage
		} else {//unknown battery type
			v_oc = v_max;
		}
		r_in = v_oc*v_oc*fabs(p_br-p_max)/(p_br*p_br);
		v_t = (v_oc+pow((v_oc*v_oc+(4*bat_load*r_in)),0.5))/2;
		internal_battery_load = v_oc*bat_load/v_t;
		b_soc_reserve = pSocReserve->get_double();
		if(internal_battery_load < 0){
			time_return = (delta_time) + (TIMESTAMP)ceil((b_soc_reserve-soc)*e_max*3600/internal_battery_load);
		} else if(internal_battery_load > 0){
			time_return = (delta_time) + (TIMESTAMP)ceil((1-soc)*e_max*3600/internal_battery_load);
		}
		return time_return;
	}

	return TS_NEVER;

}

STATUS battery::post_deltaupdate(gld::complex *useful_value, unsigned int mode_pass)
{
	return SUCCESS;	//Always succeeds right now
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_battery(OBJECT **obj, OBJECT *parent) 
{
	try 
	{
		*obj = gl_create_object(battery::oclass);
		if (*obj!=nullptr)
		{
			battery *my = OBJECTDATA(*obj,battery);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	} 
	CREATE_CATCHALL(battery);
}

EXPORT int init_battery(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=nullptr)
			return OBJECTDATA(obj,battery)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(battery);
}

EXPORT TIMESTAMP sync_battery(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	battery *my = OBJECTDATA(obj,battery);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock,t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;		
	}
	SYNC_CATCHALL(battery);
	return t2;
}

EXPORT STATUS preupdate_battery(OBJECT *obj, TIMESTAMP t0, unsigned int64 delta_time)
{
	battery *my = OBJECTDATA(obj,battery);
	STATUS status_output = FAILED;

	try
	{
		status_output = my->pre_deltaupdate(t0,delta_time);
		return status_output;
	}
	catch (char *msg)
	{
		gl_error("preupdate_battery(obj=%d;%s): %s",obj->id, (obj->name ? obj->name : "unnamed"), msg);
		return status_output;
	}
}

EXPORT SIMULATIONMODE interupdate_battery(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	battery *my = OBJECTDATA(obj,battery);
	SIMULATIONMODE status = SM_ERROR;
	try
	{
		status = my->inter_deltaupdate(delta_time,dt,iteration_count_val);
		return status;
	}
	catch (char *msg)
	{
		gl_error("interupdate_battery(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}

EXPORT STATUS postupdate_battery(OBJECT *obj, gld::complex *useful_value, unsigned int mode_pass)
{
	battery *my = OBJECTDATA(obj,battery);
	STATUS status = FAILED;
	try
	{
		status = my->post_deltaupdate(useful_value, mode_pass);
		return status;
	}
	catch (char *msg)
	{
		gl_error("postupdate_battery(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
		return status;
	}
}
