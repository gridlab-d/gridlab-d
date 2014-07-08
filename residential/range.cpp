/** $Id: range.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file range.cpp
	@addtogroup range
	@ingroup residential

	The range simulation is based on a demand profile attached to the object.
	The internal heat gain is calculated as the demand fraction of installed power.

	@{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "range.h"

#define HEIGHT_PRECISION 0.01

//////////////////////////////////////////////////////////////////////////
// range CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* range::oclass = NULL;
CLASS* range::pclass = NULL;

/**  Register the class and publish range object properties
 **/
range::range(MODULE *module) : residential_enduse(module){
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		// register the class definition
		oclass = gl_register_class(module,"range",sizeof(range),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"oven_volume[gal]",PADDR(oven_volume), PT_DESCRIPTION, "the volume of the oven",
			PT_double,"oven_UA[Btu/degF*h]",PADDR(oven_UA), PT_DESCRIPTION, "the UA of the oven (surface area divided by R-value)",
			PT_double,"oven_diameter[ft]",PADDR(oven_diameter), PT_DESCRIPTION, "the diameter of the oven",

			PT_double,"oven_demand[gpm]",PADDR(oven_demand), PT_DESCRIPTION, "the hot food take out from the oven",

			PT_double,"heating_element_capacity[kW]",PADDR(heating_element_capacity), PT_DESCRIPTION,  "the power of the heating element",
			PT_double,"inlet_food_temperature[degF]",PADDR(Tinlet), PT_DESCRIPTION,  "the inlet temperature of the food",
			PT_enumeration,"heat_mode",PADDR(heat_mode), PT_DESCRIPTION, "the energy source for heating the oven",
			PT_KEYWORD,"ELECTRIC",(enumeration)ELECTRIC,
			PT_KEYWORD,"GASHEAT",(enumeration)GASHEAT,
			PT_enumeration,"location",PADDR(location), PT_DESCRIPTION, "whether the range is inside or outside",
			PT_KEYWORD,"INSIDE",(enumeration)INSIDE,
			PT_KEYWORD,"GARAGE",(enumeration)GARAGE,
			PT_double,"oven_setpoint[degF]",PADDR(oven_setpoint), PT_DESCRIPTION, "the temperature around which the oven will heat its contents",
			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband), PT_DESCRIPTION, "the degree to heat the food in the oven, when needed",
			PT_double,"temperature[degF]",PADDR(Tw), PT_DESCRIPTION, "the outlet temperature of the oven",
			PT_double,"height[ft]",PADDR(h), PT_DESCRIPTION, "the height of the oven",

			PT_double,"food_density",PADDR(food_density), PT_DESCRIPTION, "food density",
			PT_double,"specificheat_food",PADDR(specificheat_food),
			
			PT_double,"queue_cooktop[unit]",PADDR(enduse_queue_cooktop), PT_DESCRIPTION, "number of loads accumulated",

			PT_double,"queue_oven[unit]",PADDR(enduse_queue_oven), PT_DESCRIPTION, "number of loads accumulated",

			PT_double,"queue_min[unit]",PADDR(queue_min),
			PT_double,"queue_max[unit]",PADDR(queue_max),
			
			PT_double,"time_cooktop_operation",PADDR(time_cooktop_operation),
			PT_double,"time_cooktop_setting",PADDR(time_cooktop_setting),

			PT_double,"cooktop_run_prob",PADDR(cooktop_run_prob),
			PT_double,"oven_run_prob",PADDR(oven_run_prob),

			PT_double,"cooktop_coil_setting_1[kW]",PADDR(cooktop_coil_power[0]),
			PT_double,"cooktop_coil_setting_2[kW]",PADDR(cooktop_coil_power[1]),
			PT_double,"cooktop_coil_setting_3[kW]",PADDR(cooktop_coil_power[2]),

			PT_double,"total_power_oven[kW]",PADDR(total_power_oven),
			PT_double,"total_power_cooktop[kW]",PADDR(total_power_cooktop),
			PT_double,"total_power_range[kW]",PADDR(total_power_range),

			PT_double,"demand_cooktop[unit/day]",PADDR(enduse_demand_cooktop), PT_DESCRIPTION, "number of loads accumulating daily",
			PT_double,"demand_oven[unit/day]",PADDR(enduse_demand_oven), PT_DESCRIPTION, "number of loads accumulating daily",
			
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),

			
			PT_double,"time_oven_operation[s]", PADDR(time_oven_operation),
			PT_double,"time_oven_setting[s]", PADDR(time_oven_setting),

			PT_enumeration,"state_cooktop", PADDR(state_cooktop),
			PT_KEYWORD,"CT_STOPPED",CT_STOPPED,
			PT_KEYWORD,"STAGE_6_ONLY",CT_STAGE_1_ONLY,
			PT_KEYWORD,"STAGE_7_ONLY",CT_STAGE_2_ONLY,
			PT_KEYWORD,"STAGE_8_ONLY",CT_STAGE_3_ONLY,
			PT_KEYWORD,"CT_STALLED",CT_STALLED,
			PT_KEYWORD,"CT_TRIPPED",CT_TRIPPED,

			PT_double,"cooktop_energy_baseline[kWh]", PADDR(cooktop_energy_baseline),
			PT_double,"cooktop_energy_used", PADDR(cooktop_energy_used),
			PT_double,"Toff", PADDR(Toff),
			PT_double,"Ton", PADDR(Ton),

			PT_double,"cooktop_interval_setting_1[s]", PADDR(cooktop_interval[0]),
			PT_double,"cooktop_interval_setting_2[s]", PADDR(cooktop_interval[1]),
			PT_double,"cooktop_interval_setting_3[s]", PADDR(cooktop_interval[2]),
			PT_double,"cooktop_energy_needed[kWh]",PADDR(cooktop_energy_needed),

			PT_bool,"heat_needed",PADDR(heat_needed),
			PT_bool,"oven_check",PADDR(oven_check),
			PT_bool,"remainon",PADDR(remainon),
			PT_bool,"cooktop_check",PADDR(cooktop_check),

			PT_double,"actual_load[kW]",PADDR(actual_load),PT_DESCRIPTION, "the actual load based on the current voltage across the coils",
			PT_double,"previous_load[kW]",PADDR(prev_load),PT_DESCRIPTION, "the actual load based on current voltage stored for use in controllers",
			PT_complex,"actual_power[kVA]",PADDR(range_actual_power), PT_DESCRIPTION, "the actual power based on the current voltage across the coils",
			PT_double,"is_range_on",PADDR(is_range_on),PT_DESCRIPTION, "simple logic output to determine state of range (1-on, 0-off)",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

range::~range()
{
}

int range::create() 
{

	OBJECT *hdr = OBJECTHDR(this);
	int res = residential_enduse::create();

	// initialize public values
	
	
	oven_diameter = 1.5;  // All heaters are 1.5-ft wide for now...
	Tinlet = 60.0;		// default set here, but published by the model for users to set this value
	oven_demand = 0.0;	
	
	heat_needed = FALSE;	
	heat_mode = ELECTRIC;
	is_range_on = 0;
	Tw = 0.0;
	time_oven_operation = 0;
	oven_check = false;
	remainon = false;
	cooktop_check = false;
	time_cooktop_operation = 0;
	oven_volume = 5;
	heating_element_capacity = 1;
	oven_setpoint = 100;
	Tw = 70;
	thermostat_deadband = 8;
	location = INSIDE;
	oven_UA = 2.9;
	oven_demand = 0.01;
	food_density = 5;
	specificheat_food = 1;
	time_oven_setting = 3600;
	

	enduse_queue_oven = 0.85;


	// location...mostly in garage, a few inside...
	location = gl_random_bernoulli(&hdr->rng_state,0.80) ? GARAGE : INSIDE;

	// initialize randomly distributed values
	oven_setpoint 		= clip(gl_random_normal(&hdr->rng_state,130,10),100,160);
	thermostat_deadband	= clip(gl_random_normal(&hdr->rng_state,5, 1),1,10);

	/* initialize oven thermostat */
	oven_setpoint = gl_random_normal(&hdr->rng_state,125,5);
	if (oven_setpoint<90) oven_setpoint = 90;
	if (oven_setpoint>160) oven_setpoint = 160;

	/* initialize oven deadband */
	thermostat_deadband = fabs(gl_random_normal(&hdr->rng_state,2,1))+1;
	if (thermostat_deadband>10)
		thermostat_deadband = 10;

	oven_UA = clip(gl_random_normal(&hdr->rng_state,2.0, 0.20),0.1,10) * oven_volume/50;  
	if(oven_UA <= 1.0)
		oven_UA = 2.0;	// "R-13"

	// name of enduse
	load.name = oclass->name;

	load.breaker_amps = 30;
	load.config = EUC_IS220;
	load.power_fraction = 0.0;
	load.impedance_fraction = 1.0;
	load.heatgain_fraction = 0.0; /* power has no effect on heat loss */

	state_cooktop = CT_STOPPED;
	TSTAT_PRECISION= 0.01;

	cooktop_energy_baseline = 0.5;
           		  
	cooktop_coil_power[0] = 2;
    cooktop_coil_power[1] = 1.0;
    cooktop_coil_power[2] = 1.7;
           
    cooktop_interval[0] = 240;
    cooktop_interval[1] = 900;
    cooktop_interval[2] = 120;
           
    time_cooktop_setting = 2000;

	enduse_queue_cooktop = 0.99;

	return res;

}

/** Initialize oven model properties - randomized defaults for all published variables
 **/
int range::init(OBJECT *parent)
{
	// @todo This class has serious problems and should be deleted and started from scratch. Fuller 9/27/2013.
	
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("range::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	static double sTair = 74;
	static double sTout = 68;
	if (heat_fraction==0) heat_fraction = 0.2;

	if(parent){
		pTair = gl_get_double_by_name(parent, "air_temperature");
		pTout = gl_get_double_by_name(parent, "outdoor_temperature");
	}

	if(pTair == 0){
		pTair = &sTair;
		gl_warning("range parent lacks \'air_temperature\' property, using default");
	}
	if(pTout == 0){
		pTout = &sTout;
		gl_warning("range parent lacks \'outside_temperature\' property, using default");
	}

	/* sanity checks */
	/* initialize oven volume */
	if(oven_volume <= 0.0){
//		oven_volume = 5*floor((1.0/5.0)*gl_random_uniform(0.90, 1.10) * 50.0 * (pHouse->get_floor_area() /2000.0));  // [gal]
		if (oven_volume > 100.0)
			oven_volume = 100.0;		
		else if (oven_volume < 20.0) 
			oven_volume = 20.0;
	} 

	if (oven_setpoint<90 || oven_setpoint>160)
		GL_THROW("This model is experimental and not validated: oven thermostat is set to %f and is outside the bounds of 90 to 160 degrees Fahrenheit (32.2 - 71.1 Celsius).", oven_setpoint);
		/*	TROUBLESHOOT
			TODO.
		*/

	/* initialize oven deadband */
	if (thermostat_deadband>10 || thermostat_deadband < 0.0)
		GL_THROW("oven deadband of %f is outside accepted bounds of 0 to 10 degrees (5.6 degC).", thermostat_deadband);

	// initial range UA
	if (oven_UA <= 0.0)
		GL_THROW("Range UA value is negative.");
		

	// Set heating element capacity if not provided by the user
	if (heating_element_capacity <= 0.0)
	{
		if (oven_volume >= 50)
			heating_element_capacity = 4.500;
		else 
		{
			
			double randVal = gl_random_uniform(&hdr->rng_state,0,1);
			if (randVal < 0.33)
				heating_element_capacity = 3.200;
			else if (randVal < 0.67)
				heating_element_capacity = 3.500;
			else
				heating_element_capacity = 4.500;
		}
	}

	// Other initial conditions

	if(Tw < Tinlet){ // uninit'ed temperature
		Tw = gl_random_uniform(&hdr->rng_state,oven_setpoint - thermostat_deadband, oven_setpoint + thermostat_deadband);
	}
	current_model = NONE;
	load_state = STABLE;

	// initial demand
	Tset_curtail	= oven_setpoint - thermostat_deadband/2 - 10;  // Allow T to drop only 10 degrees below lower cut-in T...

	// Setup derived characteristics...
	area 		= (pi * pow(oven_diameter,2))/4;
	height 		= oven_volume/GALPCF / area;
	Cw 			= oven_volume/GALPCF * food_density * specificheat_food;  // [Btu/F]
	
	h = height;

	// initial food temperature
	if(h == 0){
		// discharged
		Tlower = Tinlet;
		Tupper = Tinlet + TSTAT_PRECISION;
	} else {
		Tlower = Tinlet;
	}

	/* schedule checks */
	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("range does not support fixed energy shaping");
				/*	TROUBLESHOOT
					Though it is possible to drive the demand of a oven,
					it is not possible to shape its power or energy draw.  Its heater
					is either on or off, not in between.
					Change the load shape to not specify the power or energy and try
					again.
				*/
			} else if (shape.params.analog.power == 0){

				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				oven_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ oven demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated oven usage is nonsensical for residential ovens");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, it is thoroughly
					counterintuitive to the normal usage of the range.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ oven demand X kW, limited by C + Q * h */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("range load shape has an unknown state!");
			break;
	}
	return residential_enduse::init(parent);
}

int range::isa(char *classname)
{
	return (strcmp(classname,"range")==0 || residential_enduse::isa(classname));
}


void range::thermostat(TIMESTAMP t0, TIMESTAMP t1){
	Ton  = oven_setpoint - thermostat_deadband/2;
	Toff = oven_setpoint + thermostat_deadband/2;
	OBJECT *hdr = OBJECTHDR(this);

	switch(range_state()){

		case FULL:

			if (enduse_queue_oven>1)// && dryer_on == true)
			{			
								
				oven_run_prob = double(gl_random_uniform(&hdr->rng_state,queue_min,queue_max));
			
					if(Tw-TSTAT_PRECISION < Ton && (oven_run_prob > enduse_queue_oven || oven_run_prob >1 ) && (time_oven_operation <time_oven_setting))
				
					{	
					
						heat_needed = TRUE;
						oven_check = true;
						remainon = true;
						if (time_oven_operation == 0)
						enduse_queue_oven --;					

					}
			}				
			
			if (Tw+TSTAT_PRECISION > Toff || (time_oven_operation >= time_oven_setting))
			{
					heat_needed = FALSE;
					oven_check = false;
					
			} 
			if (Tw-TSTAT_PRECISION < Ton && (time_oven_operation <time_oven_setting) && remainon == true)
			{
					heat_needed = TRUE;
					oven_check = true;
			}
			if (time_oven_operation >= time_oven_setting)
			{
				remainon = false;
				time_oven_operation = 0;
			}
			else 
			{
			; // no change
			}
			
			
			break;
		case PARTIAL:
		case EMPTY:
			heat_needed = TRUE; // if we aren't full, fill 'er up!
			break;
		default:
			GL_THROW("range thermostat() detected that the oven is in an unknown state");
	}
	//return TS_NEVER; // this thermostat is purely reactive and will never drive the system
}

/** oven plc control code to set the oven 'heat_needed' state
	The thermostat set point, deadband, oven state and 
	current food temperature are used to determine 'heat_needed' state.
 **/
TIMESTAMP range::presync(TIMESTAMP t0, TIMESTAMP t1){
	/* time has passed ~ calculate internal gains, height change, temperature change */
	double nHours = (gl_tohours(t1) - gl_tohours(t0))/TS_SECOND;
	OBJECT *my = OBJECTHDR(this);

	// update temperature and height
	update_T_and_or_h(nHours);

	if(Tw > 212.0){
		//GL_THROW("the range is boiling!");
		gl_warning("range:%i is using an experimental model and should not be considered reliable", my->id);
		/*	TROUBLESHOOT
			The range object has a number of VERY experimental features and development is incomplete.
			If you are receiving this error message, reccommend no longer using this particular feature
			without considerable overhaul.
		*/
	}
	
	/* determine loadshape effects */
	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("range does not support fixed energy shaping");
				/*	TROUBLESHOOT
					Though it is possible to drive the demand of a oven,
					it is not possible to shape its power or energy draw.  Its heater
					is either on or off, not in between.
					Change the load shape to not specify the power or energy and try
					again.
				*/
			} else if (shape.params.analog.power == 0){

				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				oven_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ oven demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated oven usage is nonsensical for residential ovens");
				/*	TROUBLESHOOT
					Though it is possible to put a constant,it is thoroughly
					counterintuitive to the normal usage of the oven.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ range demand X kW, limited by C + Q * h */
				oven_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("range load shape has an unknown state!");
			break;
	}

	return TS_NEVER;
	//return residential_enduse::sync(t0,t1);
}

/** oven synchronization determines the time to next
	synchronization state and the power drawn since last synch
 **/
TIMESTAMP range::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double internal_gain = 0.0;
	double nHours = (gl_tohours(t1) - gl_tohours(t0))/TS_SECOND;
	double Tamb = get_Tambient(location);
	double dt = gl_toseconds(t0>0?t1-t0:0);

	if (oven_check == true || remainon == true)	
	time_oven_operation +=dt;

	if (remainon == false) 
	time_oven_operation=0;

	enduse_queue_oven += enduse_demand_oven * dt/3600/24;
	

			if (t0>TS_ZERO && t1>t0)
		{
			// compute the total energy usage in this interval
			load.energy += load.total * dt/3600.0;
		}		

	if(re_override == OV_ON){
		heat_needed = TRUE;
	} else if(re_override == OV_OFF){
		heat_needed = FALSE;
	}

	if(Tw > 212.0 - thermostat_deadband){ // if it's trying boil, turn it off!
		heat_needed = FALSE;
		is_range_on = 0;
	}
	// determine the power used
	if (heat_needed == TRUE){
		/* power_kw */ load.total = heating_element_capacity * (heat_mode == GASHEAT ? 0.01 : 1.0);
		is_range_on = 1;
	} else {
		/* power_kw */ load.total = 0.0;
		is_range_on = 0;
	}

	TIMESTAMP t2 = residential_enduse::sync(t0,t1);
	
	set_time_to_transition();

	if (location == INSIDE){
		if(this->current_model == ONENODE){
			internal_gain = oven_UA * (Tw - get_Tambient(location));
		} 

	} else {
		internal_gain = 0;
	}

	dt = update_state(dt, t1);

	

	//load.total = load.power = /* power_kw */ load.power;
	load.power = load.total * load.power_fraction;
	load.admittance = load.total * load.impedance_fraction;
	load.current = load.total * load.current_fraction;
	load.heatgain = internal_gain;

	range_actual_power = load.power + (load.current + load.admittance * load.voltage_factor )* load.voltage_factor;
	actual_load = range_actual_power.Re();
	if (heat_needed == true)
	total_power_oven = actual_load;
	else
	total_power_oven =0;

	if (actual_load != 0.0)
	{
		prev_load = actual_load;
		power_state = PS_ON;
	}
	else
		power_state = PS_OFF;

//	gl_enduse_sync(&(residential_enduse::load),t1);

	if(re_override == OV_NORMAL){
		if (time_to_transition < dt)
		{
			if (time_to_transition >= (1.0/3600.0))	// 0.0167 represents one second
			{
				TIMESTAMP t_to_trans = (t1+time_to_transition*3600.0/TS_SECOND);
				return -(t_to_trans); // negative means soft transition
			}
			// less than one second means never
			else
				return TS_NEVER; 
		}
		else
			return (TIMESTAMP)(t1+dt);
	} else {
		return TS_NEVER; // keep running until the forced state ends
	}


}

double range::update_state(double dt1,TIMESTAMP t1)
{	
	OBJECT *hdr = OBJECTHDR(this);
	cooktop_energy_used += total_power_cooktop* dt1/3600;


switch(state_cooktop) {

	case CT_STOPPED:

		if (enduse_queue_cooktop>1)
		
		cooktop_run_prob = double(gl_random_uniform(&hdr->rng_state,queue_min,queue_max));
				
		if (enduse_queue_cooktop > 1 && (cooktop_run_prob > enduse_queue_cooktop))
			{
				state_cooktop = CT_STAGE_1_ONLY;
				cooktop_energy_needed = cooktop_energy_baseline;
				cycle_duration_cooktop = 1000 * (cooktop_energy_needed - cooktop_energy_used) /cooktop_coil_power[0] * 60 * 60;
				cycle_time_cooktop = cooktop_interval[0];
				cooktop_check = true;								
				enduse_queue_cooktop--;
				
			}
			
			else
			{
				state_cooktop = CT_STOPPED;
				state_time = 0;
				cycle_time_cooktop = 0;
				cooktop_energy_used = 0;
				cooktop_check = false;
				//enduse_queue_cooktop--;
			}
			
		break;

	case CT_STAGE_1_ONLY:
		
		
		if (cooktop_energy_used >= cooktop_energy_needed || time_cooktop_operation >= time_cooktop_setting || cycle_time_cooktop <= 0)
		{
				if (cooktop_energy_used >= cooktop_energy_needed || time_cooktop_operation >= time_cooktop_setting)
				{  // The dishes are clean
					state_cooktop = CT_STOPPED;
					cycle_time_cooktop = 0;
					cooktop_energy_used = 0;
					cooktop_check = false;
		
				}
							
				else if (cycle_time_cooktop <= 0)
				{  
					state_cooktop = CT_STAGE_2_ONLY;
					double cycle_t = 1000 * (cooktop_energy_needed - cooktop_energy_used) / cooktop_coil_power[2] * 60 * 60;
					double interval = cooktop_interval[1];
					cooktop_check = true;
					if (cycle_t > interval)
						cycle_time_cooktop = interval;
					else
						cycle_time_cooktop = cycle_t;
					
				}	
		
				else if (pCircuit->pV->Mag()<stall_voltage)
				{
					state_cooktop = CT_STALLED;
					state_time = 0;
				}

		}
		break;

case CT_STAGE_2_ONLY:

	if (cooktop_energy_used >= cooktop_energy_needed || time_cooktop_operation >= time_cooktop_setting || cycle_time_cooktop <= 0)
	{
			if (cooktop_energy_used >= cooktop_energy_needed || time_cooktop_operation >= time_cooktop_setting)
			{ 
				state_cooktop = CT_STOPPED;
				cycle_time_cooktop = 0;
				cooktop_energy_used = 0;			
				cooktop_check = false;
			}
					
			else if (cycle_time_cooktop <= 0)
			{  
				state_cooktop = CT_STAGE_3_ONLY;
				cooktop_check = true;
				cycle_time_cooktop = time_cooktop_setting - cooktop_interval[0] - cooktop_interval[1]; 
			}	

			else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state_cooktop = CT_STALLED;
				state_time = 0;
			}

	}
break;

case CT_STAGE_3_ONLY:

			if (cooktop_energy_used >= cooktop_energy_needed ||  cycle_time_cooktop <= 0)
			{  // The dishes are clean
				state_cooktop = CT_STOPPED;
				cycle_time_cooktop = 0;
				cooktop_energy_used = 0;
				cooktop_check = false;
					
			}					

			else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state_cooktop = CT_STALLED;
				state_time = 0;
			}


	break;

	case CT_STALLED:
		if (pCircuit->pV->Mag()>start_voltage)
		{
			state_cooktop = CT_STAGE_1_ONLY;
			state_time = cycle_time_cooktop;
			cooktop_check = false;
		}

		break;

	case CT_TRIPPED:
		if (state_time>reset_delay)
		{
			if (pCircuit->pV->Mag()>start_voltage)
				state_cooktop = CT_STAGE_1_ONLY;
			else
				state_cooktop = CT_STALLED;
			state_time = 0;
		}

		break;
	}
	


	// advance the state_time
	state_time += dt1;

	// accumulating units in the queue no matter what happens
//	enduse_queue_oven += enduse_demand_oven * dt/3600/24;
	enduse_queue_cooktop += enduse_demand_cooktop * dt1/3600/24;

	if (cooktop_check == true)
	time_cooktop_operation += 1;
	else
	time_cooktop_operation = 0;


	// now implement current state


	switch(state_cooktop) {
	case CT_STOPPED: 
		
		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);
		
		// time to next expected state_cooktop change
		//dt = (enduse_demand_cooktop<=0) ? -1 : 	dt = 3600/enduse_demand_cooktop; 
		dt1 = (enduse_queue_cooktop>=1) ? 0 : ((1-enduse_queue_cooktop)*3600)/(enduse_queue_cooktop*24); 	

		break;

	case CT_STAGE_1_ONLY:	

		//motor_on_off = motor_coil_on_off = both_coils_on_off = 1;
		cycle_time_cooktop -= dt1;

		load.power = load.current = complex(0,0,J);
		load.admittance = complex((cooktop_coil_power[0])/1000,0,J);

		dt1 = cycle_time_cooktop;
		break;

	case CT_STAGE_2_ONLY:	

	//motor_on_off = motor_coil_on_off = both_coils_on_off = 1;
	cycle_time_cooktop -= dt1;

	load.power = load.current = complex(0,0,J);
	load.admittance = complex((cooktop_coil_power[1])/1000,0,J);

	dt1 = cycle_time_cooktop;
	break;

	case CT_STAGE_3_ONLY:	

	//motor_on_off = motor_coil_on_off = both_coils_on_off = 1;
	cycle_time_cooktop -= dt1;

	load.power = load.current = complex(0,0,J);
	load.admittance = complex((cooktop_coil_power[2])/1000,0,J);

	dt1 = cycle_time_cooktop;
	break;

	case CT_STALLED:

		// running in constant impedance mode
		load.power = load.current = complex(0,0,J);
		load.admittance = complex(1)/stall_impedance;

		// time to trip
		dt1 = trip_delay;

		break;

	case CT_TRIPPED:

		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);
		
		// time to next expected state change
		dt1 = reset_delay; 

		break;

	
	default:

		throw "unexpected motor state";
		/*	TROUBLESHOOT
			This is an error.  Please submit a bug report along with at the range
			object & class sections from the relevant GLM file, and from the dump file.
		*/
		break;
	}

	load.total += load.power + load.current + load.admittance;
	total_power_cooktop = (load.power.Re() + (load.current.Re() + load.admittance.Re()*load.voltage_factor)*load.voltage_factor)*1000;


//End of cook top


	//total_power_range = total_power_cooktop + total_power_oven;
	// compute the total heat gain
	load.heatgain = load.total.Mag() * heat_fraction;


	if (dt1 > 0 && dt1 < 1)
		dt1 = 1;

	return dt1;

	}

TIMESTAMP range::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}
int range::commit(){
	Tw_old = Tw;
	Tupper_old = /*Tupper*/ Tw;
	Tlower_old = Tlower;
	oven_demand_old = oven_demand;
	return 1;
}

/** Oven state determined based on the height of the column of hot food
 **/
enumeration range::range_state(void)
{
	if ( h >= height-HEIGHT_PRECISION )
		return FULL;
	else if ( h <= HEIGHT_PRECISION)
		return EMPTY;
	else
		return PARTIAL;
}

/** Calculate the time to transition from the current state to new state
 **/
void range::set_time_to_transition(void)
{
	// set the model and load state
	set_current_model_and_load_state();

	time_to_transition = -1;

	switch (current_model) {
		case ONENODE:
			if (heat_needed == FALSE)
				time_to_transition = new_time_1node(Tw, Ton);
			else if (load_state == RECOVERING)
				time_to_transition = new_time_1node(Tw, Toff);
			else
				time_to_transition = -1;
			break;

	}
	return;
}

/** Set the oven model and its state based on the estimated
	temperature differential along the height of the food column when it is full, 
	emplty or partial at the current height.
 **/
enumeration range::set_current_model_and_load_state(void)
{
	double dhdt_now = dhdt(h);
	double dhdt_full = dhdt(height);
	double dhdt_empty = dhdt(0.0);
	current_model = NONE;		// by default set it to onenode
	load_state = STABLE;		// by default

	enumeration oven_status = range_state();

	switch(oven_status) 
	{
		case EMPTY:
			if (dhdt_empty <= 0.0) 
			{
				current_model = ONENODE;
				load_state = DEPLETING;
				Tw = Tupper = Tinlet + HEIGHT_PRECISION;
				Tlower = Tinlet;
				h = height;

			}
			else if (dhdt_full > 0)
			{
				// overriding the plc code ignoring thermostat logic
				// heating will always be on while in two zone model
				heat_needed = TRUE;
				//current_model = TWONODE;//
				current_model = ONENODE;
				load_state = RECOVERING;
			}
			else
				load_state = STABLE;
			break;

		case FULL:

			if (dhdt_full < 0)
			{

				bool cur_heat_needed = heat_needed;
				heat_needed = TRUE;
				double dhdt_full_temp = dhdt(height);
				if (dhdt_full_temp < 0)
				{
					current_model = ONENODE;
					load_state = DEPLETING;
				}
				else
				{
					current_model = ONENODE;
					
					heat_needed = cur_heat_needed;
					load_state = heat_needed ? RECOVERING : DEPLETING;
				}
			}
			else if (dhdt_empty > 0)
			{
				current_model = ONENODE;
				load_state = RECOVERING;
			}
			else
				load_state = STABLE;
			break;

		case PARTIAL:

			current_model = ONENODE;

			heat_needed = TRUE;

			if (dhdt_now < 0 && (dhdt_now * dhdt_empty) >= 0)
				load_state = DEPLETING;
			else if (dhdt_now > 0 && (dhdt_now * dhdt_full) >= 0) 
				load_state = RECOVERING;
			else 
			{
				current_model = NONE;
				load_state = STABLE;
			}
			break;
	}

	return load_state;
}

void range::update_T_and_or_h(double nHours)
{
	/*
		When this gets called (right after the range gets sync'd),
		all states are exactly as they were at the end of the last sync.
		We calculate what has happened to the food temperature (or the
		warm/cold boundarly location, depending on the current state)
		in the interim.  If nHours equals our previously requested
		timeToTransition, we should find things landing on a new state.
		If not, we should find ourselves in the same state again.  But
		this routine doesn't try to figure that out...it just calculates
		the new T/h.
	*/

	// set the model and load state
	switch (current_model) 
	{
		case ONENODE:
			// Handy that the 1-node model doesn't care which way
			// things are moving (RECOVERING vs DEPLETING)...
SingleZone:
			Tw = new_temp_1node(Tw, nHours);
			/*Tupper*/ Tw = Tw;
			Tlower = Tinlet;
			break;

			// Correct h if it overshot...
			if (h < ROUNDOFF) 
			{
				// We've over-depleted the oven slightly.  Make a quickie
				// adjustment to Tlower/Tw to account for it...

				double vol_over = oven_volume/GALPCF * h/height;  // Negative...
				double energy_over = vol_over * food_density * specificheat_food * (/*Tupper*/ Tw - Tlower);
				double Tnew = Tlower + energy_over/Cw;
				Tw = Tlower = Tnew;
				h = 0;
			} 
			else if (h > height) 
			{
				// Ditto for over-recovery...
				double vol_over = oven_volume/GALPCF * (h-height)/height;
				double energy_over = vol_over * food_density * specificheat_food * (/*Tupper*/ Tw - Tlower);
				double Tnew = /*Tupper*/ Tw + energy_over/Cw;
				Tw = /*Tupper*/ Tw = Tnew;
				Tlower = Tinlet;
				h = height;
			} 
			else 
			{
				Tw = Tw;
			}
			break;

		default:
			break;
	}

	if (heat_needed == TRUE)
		power_state = PS_ON;
	else
		power_state = PS_OFF;

	return;
}

/* the key to picking the equations apart is that the goal is to calculate the temperature differences relative to the
 *	temperature of the lower node (or inlet temp, if 1node).

 */
double range::dhdt(double h)
{
	if (/*Tupper*/ Tw - Tlower < ROUNDOFF)
		return 0.0; // if /*Tupper*/ Tw and Tlower are same then dh/dt = 0.0;

	// Pre-set some algebra just for efficiency...
	const double mdot = oven_demand * 60 * food_density / GALPCF;		// lbm/hr...
    const double c1 = food_density * specificheat_food * area * (/*Tupper*/ Tw - Tlower);	// Btu/ft...

	if (oven_demand > 0.0)
		double aaa=1;
	
    // check c1 before dividing by it
    if (c1 <= ROUNDOFF)
        return 0.0; //Possible only when /*Tupper*/ Tw and Tlower are very close, and the difference is negligible

	const double cA = -mdot / (food_density * area) + (actual_kW() * BTUPHPKW + oven_UA * (get_Tambient(location) - Tlower)) / c1;
	const double cb = (oven_UA / height) * (/*Tupper*/ Tw - Tlower) / c1;

	// Returns the rate of change of 'h'
	return cA - cb*h;
}

double range::actual_kW(void)
{
	OBJECT *obj = OBJECTHDR(this);
	const double nominal_voltage = 240.0; //@TODO:  Determine if this should be published or how we want to obtain this from the equipment/network
    static int trip_counter = 0;

	// calculate rated heat capacity adjusted for the current line voltage
	if (heat_needed && re_override != OV_OFF)
    {
		if(heat_mode == GASHEAT){
			return heating_element_capacity; /* gas heating is voltage independent. */
		}
		const double actual_voltage = pCircuit ? pCircuit->pV->Mag() : nominal_voltage;
        if (actual_voltage > 2.0*nominal_voltage)
        {
            if (trip_counter++ > 10)
				GL_THROW("oven line voltage for range:%d is too high, exceeds twice nominal voltage.",obj->id);
			/*	TROUBLESHOOT
				The range is receiving twice the nominal voltage consistantly, or about 480V on what
				should be a 240V circuit.  Please sanity check your powerflow model as it feeds to the
				meter and to the house.
			*/
            else
                return 0.0;         // @TODO:  This condition should trip the breaker with a counter
        }
		double test = heating_element_capacity * (actual_voltage*actual_voltage) / (nominal_voltage*nominal_voltage);
		return test;
    }
	else
		return 0.0;
}

inline double range::new_time_1node(double T0, double T1)
{
	const double mdot_Cp = specificheat_food * oven_demand * 60 * food_density / GALPCF;

    if (Cw <= ROUNDOFF)
        return -1.0;

	const double c1 = ((actual_kW()*BTUPHPKW + oven_UA * get_Tambient(location)) + mdot_Cp*Tinlet) / Cw;
	const double c2 = -(oven_UA + mdot_Cp) / Cw;

    if (fabs(c1 + c2*T1) <= ROUNDOFF || fabs(c1 + c2*T0) <= ROUNDOFF || fabs(c2) <= ROUNDOFF)
        return -1.0;

	const double new_time = (log(fabs(c1 + c2 * T1)) - log(fabs(c1 + c2 * T0))) / c2;	// [hr]
	return new_time;
}

inline double range::new_temp_1node(double T0, double delta_t)
{
	// old because this happens in presync and needs previously used demand
	const double mdot_Cp = specificheat_food * oven_demand_old * 60 * food_density / GALPCF;
	// Btu / degF.lb * gal/hr * lb/cf * cf/gal = Btu / degF.hr

    if (Cw <= ROUNDOFF || (oven_UA+mdot_Cp) <= ROUNDOFF)
        return T0;

	const double c1 = (oven_UA + mdot_Cp) / Cw;
	const double c2 = (actual_kW()*BTUPHPKW + mdot_Cp*Tinlet + oven_UA*get_Tambient(location)) / (oven_UA + mdot_Cp);

//	return  c2 - (c2 + T0) * exp(c1 * delta_t);	// [F]
	return  c2 - (c2 - T0) * exp(-c1 * delta_t);	// [F]
}


double range::get_Tambient(enumeration loc)
{
	double ratio;
	OBJECT *parent = OBJECTHDR(this)->parent;

	switch (loc) {
	case GARAGE: // temperature is about 1/2 way between indoor and outdoor
		ratio = 0.5;
		break;
	case INSIDE: // temperature is all indoor
	default:
		ratio = 1.0;
		break;
	}

	// return temperature of location
	//house *pHouse = OBJECTDATA(OBJECTHDR(this)->parent,house);
	//return pHouse->get_Tair()*ratio + pHouse->get_Tout()*(1-ratio);
	return *pTair * ratio + *pTout *(1-ratio);
}

void range::wrong_model(enumeration msg)
{
	char *errtxt[] = {"model is not one-zone","model is not two-zone"};
	OBJECT *obj = OBJECTHDR(this);
	gl_warning("%s (range:%d): %s", obj->name?obj->name:"(anonymous object)", obj->id, errtxt[msg]);
	throw msg; // this must be caught by the range code, not by the core
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_range(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(range::oclass);
	if (*obj!=NULL)
	{
		range *my = OBJECTDATA(*obj,range);;
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_range(OBJECT *obj)
{
	range *my = OBJECTDATA(obj,range);
	return my->init(obj->parent);
}

EXPORT int isa_range(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,range)->isa(classname);
	} else {
		return 0;
	}
}


EXPORT TIMESTAMP sync_range(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	range *my = OBJECTDATA(obj, range);
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the object clock if it has not been set yet
	try {
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return my->presync(obj->clock, t0);
		case PC_BOTTOMUP:
			return my->sync(obj->clock, t0);
		case PC_POSTTOPDOWN:
			t1 = my->postsync(obj->clock, t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	catch (int m)
	{
		gl_error("%s (range:%d) model zone exception (code %d) not caught", obj->name?obj->name:"(anonymous range)", obj->id, m);
	}
	catch (char *msg)
	{
		gl_error("%s (range:%d) %s", obj->name?obj->name:"(anonymous range)", obj->id, msg);
	}
	return TS_INVALID;
}

EXPORT int commit_range(OBJECT *obj)
{
	range *my = OBJECTDATA(obj,range);
	return my->commit();
}

EXPORT TIMESTAMP plc_range(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the range
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	range *my = OBJECTDATA(obj,range);
	my->thermostat(obj->clock, t0);
	
	// no changes to timestamp will be made by the internal oven thermostat
	/// @todo If external plc codes return a timestamp, it will allow sync sooner but not later than oven time to transition (ticket #147)
	return TS_NEVER;  
}

/** $Id: range.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file range.cpp
	@addtogroup range Electric range
	@ingroup residential

	The residential electric range uses a hybrid thermal model that is capable
	of tracking a single-mass of food.

 @{
 **/
/**@}**/

