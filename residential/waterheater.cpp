/** $Id: waterheater.cpp,v 1.51 2008/02/15 00:24:14 d3j168 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file waterheater.cpp
	@addtogroup waterheater Electric waterheater
	@ingroup residential

	The residential electric waterheater uses a hybrid thermal model that is capable
	of tracking either a single-mass of water, or a dual-mass with a varying thermocline.

	The driving dynamic parameters of the waterheater are
	- <b>demand</b>: the current consumption of water in gallons/minute; the higher
	  the demand, the more quickly the thermocline drops.
	- <b>voltage</b>: the line voltage for the coil; the lower the voltage, the
	  more slowly the thermocline rises.
	- <b>inlet water temperature</b>: the inlet water temperature; the lower the inlet
	  water temperature, the more heat is needed to raise it to the set point
	- <b>indoor air temperature</b>: the higher the indoor temperature, the less heat
	  loss through the jacket.
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "waterheater.h"

#define TSTAT_PRECISION 0.01
#define HEIGHT_PRECISION 0.01

//////////////////////////////////////////////////////////////////////////
// waterheater CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* waterheater::oclass = NULL;
CLASS* waterheater::pclass = NULL;

/**  Register the class and publish water heater object properties
 **/
waterheater::waterheater(MODULE *module) : residential_enduse(module){
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		// register the class definition
		oclass = gl_register_class(module,"waterheater",sizeof(waterheater),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"tank_volume[gal]",PADDR(tank_volume), PT_DESCRIPTION, "the volume of water in the tank when it is full",
			PT_double,"tank_UA[Btu*h/degF]",PADDR(tank_UA), PT_DESCRIPTION, "the UA of the tank (surface area divided by R-value)",
			PT_double,"tank_diameter[ft]",PADDR(tank_diameter), PT_DESCRIPTION, "the diameter of the water heater tank",
			PT_double,"water_demand[gpm]",PADDR(water_demand), PT_DESCRIPTION, "the hot water draw from the water heater",
			PT_double,"heating_element_capacity[kW]",PADDR(heating_element_capacity), PT_DESCRIPTION,  "the power of the heating element",
			PT_double,"inlet_water_temperature[degF]",PADDR(Tinlet), PT_DESCRIPTION,  "the inlet temperature of the water tank",
			PT_enumeration,"heat_mode",PADDR(heat_mode), PT_DESCRIPTION, "the energy source for heating the water heater",
				PT_KEYWORD,"ELECTRIC",(enumeration)ELECTRIC,
				PT_KEYWORD,"GASHEAT",(enumeration)GASHEAT,
			PT_enumeration,"location",PADDR(location), PT_DESCRIPTION, "whether the water heater is inside or outside",
				PT_KEYWORD,"INSIDE",(enumeration)INSIDE,
				PT_KEYWORD,"GARAGE",(enumeration)GARAGE,
			PT_double,"tank_setpoint[degF]",PADDR(tank_setpoint), PT_DESCRIPTION, "the temperature around which the water heater will heat its contents",
			PT_double,"thermostat_deadband[degF]",PADDR(thermostat_deadband), PT_DESCRIPTION, "the degree to heat the water tank, when needed",
			PT_double,"temperature[degF]",PADDR(Tw), PT_DESCRIPTION, "the outlet temperature of the water tank",
			PT_double,"height[ft]",PADDR(h), PT_DESCRIPTION, "the height of the hot water column within the water tank",
			PT_complex,"demand[kVA]",PADDR(load.total), PT_DESCRIPTION, "the water heater power consumption",
			PT_double,"actual_load[kW]",PADDR(actual_load),PT_DESCRIPTION, "the actual load based on the current voltage across the coils",
			PT_double,"previous_load[kW]",PADDR(prev_load),PT_DESCRIPTION, "the actual load based on current voltage stored for use in controllers",
			PT_complex,"actual_power[kVA]",PADDR(waterheater_actual_power), PT_DESCRIPTION, "the actual power based on the current voltage across the coils",
			PT_double,"is_waterheater_on",PADDR(is_waterheater_on),PT_DESCRIPTION, "simple logic output to determine state of waterheater (1-on, 0-off)",
			PT_double,"gas_fan_power[kW]",PADDR(gas_fan_power),PT_DESCRIPTION, "load of a running gas waterheater",
			PT_double,"gas_standby_power[kW]",PADDR(gas_standby_power),PT_DESCRIPTION, "load of a gas waterheater in standby",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

waterheater::~waterheater()
{
}

int waterheater::create() 
{
	int res = residential_enduse::create();

	// initialize public values
	tank_volume = 50.0;
	tank_UA = 0.0;
	tank_diameter	= 1.5;  // All heaters are 1.5-ft wide for now...
	Tinlet = 60.0;		// default set here, but published by the model for users to set this value
	water_demand = 0.0;	// in gpm
	heating_element_capacity = 0.0;
	heat_needed = FALSE;
	location = GARAGE;
	heat_mode = ELECTRIC;
	tank_setpoint = 0.0;
	thermostat_deadband = 0.0;
	is_waterheater_on = 0;
//	power_kw = complex(0,0);
	Tw = 0.0;

	// location...mostly in garage, a few inside...
	location = gl_random_bernoulli(RNGSTATE,0.80) ? GARAGE : INSIDE;

	// initialize randomly distributed values
	tank_setpoint 		= clip(gl_random_normal(RNGSTATE,130,10),100,160);
	thermostat_deadband	= clip(gl_random_normal(RNGSTATE,5, 1),1,10);

	/* initialize water tank thermostat */
	tank_setpoint = gl_random_normal(RNGSTATE,125,5);
	if (tank_setpoint<90) tank_setpoint = 90;
	if (tank_setpoint>160) tank_setpoint = 160;

	/* initialize water tank deadband */
	thermostat_deadband = fabs(gl_random_normal(RNGSTATE,2,1))+1;
	if (thermostat_deadband>10)
		thermostat_deadband = 10;

	tank_UA = clip(gl_random_normal(RNGSTATE,2.0, 0.20),0.1,10) * tank_volume/50;  
	if(tank_UA <= 1.0)
		tank_UA = 2.0;	// "R-13"

	// name of enduse
	load.name = oclass->name;

	load.breaker_amps = 30;
	load.config = EUC_IS220;
	load.power_fraction = 0.0;
	load.impedance_fraction = 1.0;
	load.heatgain_fraction = 0.0; /* power has no effect on heat loss */

	gas_fan_power = -1.0;
	gas_standby_power = -1.0;

	return res;

}

/** Initialize water heater model properties - randomized defaults for all published variables
 **/
int waterheater::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("waterheater::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}

	hdr->flags |= OF_SKIPSAFE;

	static double sTair = 74;
	static double sTout = 68;

	if(parent){
		pTair = gl_get_double_by_name(parent, "air_temperature");
		pTout = gl_get_double_by_name(parent, "outdoor_temperature");
	}

	if(pTair == 0){
		pTair = &sTair;
		gl_warning("waterheater parent lacks \'air_temperature\' property, using default");
	}
	if(pTout == 0){
		pTout = &sTout;
		gl_warning("waterheater parent lacks \'outside_temperature\' property, using default");
	}

	/* sanity checks */
	/* initialize water tank volume */
	if(tank_volume <= 0.0){
//		tank_volume = 5*floor((1.0/5.0)*gl_random_uniform(0.90, 1.10) * 50.0 * (pHouse->get_floor_area() /2000.0));  // [gal]
		if (tank_volume > 100.0)
			tank_volume = 100.0;		
		else if (tank_volume < 20.0) 
			tank_volume = 20.0;
	} else {
		if (tank_volume > 100.0 || tank_volume < 20.0){
			gl_error("watertank volume of %f outside the volume bounds of 20 to 100 gallons.", tank_volume);
			/*	TROUBLESHOOT
				All waterheaters must be set between 40 and 100 gallons.  Most waterheaters are assumed to be 50 gallon tanks.
			*/
		}
	}

	if (tank_setpoint<90 || tank_setpoint>160)
		gl_error("watertank thermostat is set to %f and is outside the bounds of 90 to 160 degrees Fahrenheit (32.2 - 71.1 Celsius).", tank_setpoint);
		/*	TROUBLESHOOT
			All waterheaters must be set between 90 degF and 160 degF.

	/* initialize water tank deadband */
	if (thermostat_deadband>10 || thermostat_deadband < 0.0)
		GL_THROW("watertank deadband of %f is outside accepted bounds of 0 to 10 degrees (5.6 degC).", thermostat_deadband);

	// initial tank UA
	if (tank_UA <= 0.0)
		GL_THROW("Tank UA value is negative.");
		

	// Set heating element capacity if not provided by the user
	if (heating_element_capacity <= 0.0)
	{
		if (tank_volume >= 50)
			heating_element_capacity = 4.500;
		else 
		{
			// Smaller tanks can be either 3200, 3500, or 4500...
			double randVal = gl_random_uniform(RNGSTATE,0,1);
			if (randVal < 0.33)
				heating_element_capacity = 3.200;
			else if (randVal < 0.67)
				heating_element_capacity = 3.500;
			else
				heating_element_capacity = 4.500;
		}
	}

	// set gas electric loads, if not provided by the user
	if(0 > gas_fan_power){
		gas_fan_power = heating_element_capacity * 0.01;
	}

	if(0 > gas_standby_power){
		gas_standby_power = 0.0; // some units consume 3-5W
	}

	// Other initial conditions

	if(Tw < Tinlet){ // uninit'ed temperature
		Tw = gl_random_uniform(RNGSTATE,tank_setpoint - thermostat_deadband, tank_setpoint + thermostat_deadband);
	}
	current_model = NONE;
	load_state = STABLE;

	// initial demand
	Tset_curtail	= tank_setpoint - thermostat_deadband/2 - 10;  // Allow T to drop only 10 degrees below lower cut-in T...

	// Setup derived characteristics...
	area 		= (pi * pow(tank_diameter,2))/4;
	height 		= tank_volume/GALPCF / area;
	Cw 			= tank_volume/GALPCF * RHOWATER * Cp;  // [Btu/F]

	h = height;

	// initial water temperature
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
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("waterheater does not support fixed energy shaping");
				/*	TROUBLESHOOT
					Though it is possible to drive the water demand of a water heater,
					it is not possible to shape its power or energy draw.  Its heater
					is either on or off, not in between.
					Change the load shape to not specify the power or energy and try
					again.
				*/
			} else if (shape.params.analog.power == 0){
				 /* power-driven ~ cheat with W/degF*gpm */
//				double heat_per_gallon = RHOWATER * // lb/cf
//										 CFPGAL *	// lb/gal
//										 CWATER *	// BTU/degF / gal
//										 KWBTUPH /	// kW/gal
//										 1000.0;	// W/gal
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				water_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ draws water to consume X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated water usage is nonsensical for residential water heaters");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, low-level water draw on a water heater, it is thoroughly
					counterintuitive to the normal usage of the waterheater.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ draws water to consume X kW, limited by C + Q * h */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("waterheater load shape has an unknown state!");
			break;
	}
	return residential_enduse::init(parent);
}

int waterheater::isa(char *classname)
{
	return (strcmp(classname,"waterheater")==0 || residential_enduse::isa(classname));
}


void waterheater::thermostat(TIMESTAMP t0, TIMESTAMP t1){
	Ton  = tank_setpoint - thermostat_deadband/2;
	Toff = tank_setpoint + thermostat_deadband/2;

	switch(tank_state()){
		case FULL:
			if(Tw-TSTAT_PRECISION < Ton){
				heat_needed = TRUE;
			} else if (Tw+TSTAT_PRECISION > Toff){
				heat_needed = FALSE;
			} else {
				; // no change
			}
			break;
		case PARTIAL:
		case EMPTY:
			heat_needed = TRUE; // if we aren't full, fill 'er up!
			break;
		default:
			GL_THROW("waterheater thermostat() detected that the water heater tank is in an unknown state");
	}
	//return TS_NEVER; // this thermostat is purely reactive and will never drive the system
}

/** Water heater plc control code to set the water heater 'heat_needed' state
	The thermostat set point, deadband, tank state(height of hot water column) and 
	current water temperature are used to determine 'heat_needed' state.
 **/
TIMESTAMP waterheater::presync(TIMESTAMP t0, TIMESTAMP t1){
	/* time has passed ~ calculate internal gains, height change, temperature change */
	double nHours = (gl_tohours(t1) - gl_tohours(t0))/TS_SECOND;
	OBJECT *my = OBJECTHDR(this);

	// update temperature and height
	update_T_and_or_h(nHours);

	if(Tw > 212.0){
		//GL_THROW("the waterheater is boiling!");
		gl_warning("waterheater:%i is boiling", my->id);
		/*	TROUBLESHOOT
			The temperature model for the waterheater has broken, or the environment around the
			waterheater has burst into flames.  Please post this with your model and dump files
			attached to the bug report.
		 */
	}
	
	/* determine loadshape effects */
	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("waterheater does not support fixed energy shaping");
				/*	TROUBLESHOOT
					Though it is possible to drive the water demand of a water heater,
					it is not possible to shape its power or energy draw.  Its heater
					is either on or off, not in between.
					Change the load shape to not specify the power or energy and try
					again.
				*/
			} else if (shape.params.analog.power == 0){
				 /* power-driven ~ cheat with W/degF*gpm */
//				double heat_per_gallon = RHOWATER * // lb/cf
//										 CFPGAL *	// lb/gal
//										 CWATER *	// BTU/degF / gal
//										 KWBTUPH /	// kW/gal
//										 1000.0;	// W/gal
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				water_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ draws water to consume X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated water usage is nonsensical for residential water heaters");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, low-level water draw on a water heater, it is thoroughly
					counterintuitive to the normal usage of the waterheater.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ draws water to consume X kW, limited by C + Q * h */
				water_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("waterheater load shape has an unknown state!");
			break;
	}

	return TS_NEVER;
	//return residential_enduse::sync(t0,t1);
}

/** Water heater synchronization determines the time to next
	synchronization state and the power drawn since last synch
 **/
TIMESTAMP waterheater::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double internal_gain = 0.0;
	double nHours = (gl_tohours(t1) - gl_tohours(t0))/TS_SECOND;
	double Tamb = get_Tambient(location);

	// use re_override to control heat_needed state
	// runs after thermostat() but before "the usual" calculations
	if(re_override == OV_ON){
		heat_needed = TRUE;
	} else if(re_override == OV_OFF){
		heat_needed = FALSE;
	}

	if(Tw > 212.0 - thermostat_deadband){ // if it's trying boil, turn it off!
		heat_needed = FALSE;
		is_waterheater_on = 0;
	}


	TIMESTAMP t2 = residential_enduse::sync(t0,t1);
	
	// Now find our current temperatures and boundary height...
	// And compute the time to the next transition...
	//Adjusted because shapers go on sync, not presync

	set_time_to_transition();

	// determine internal gains
	if (location == INSIDE){
		if(this->current_model == ONENODE){
			internal_gain = tank_UA * (Tw - get_Tambient(location));
		} else if(this->current_model == TWONODE){
			internal_gain = tank_UA * (Tw - Tamb) * h / height;
			internal_gain += tank_UA * (Tlower - Tamb) * (1 - h / height);
		}
	} else {
		internal_gain = 0;
	}

	// determine the power used
	if (heat_needed == TRUE){
		/* power_kw */ load.total = (heat_mode == GASHEAT ? gas_fan_power : heating_element_capacity);
		is_waterheater_on = 1;
	} else {
		/* power_kw */ load.total = (heat_mode == GASHEAT ? gas_standby_power : 0.0);
		is_waterheater_on = 0;
	}

	//load.total = load.power = /* power_kw */ load.power;
	load.power = load.total * load.power_fraction;
	load.admittance = load.total * load.impedance_fraction;
	load.current = load.total * load.current_fraction;
	load.heatgain = internal_gain;

	waterheater_actual_power = load.power + (load.current + load.admittance * load.voltage_factor )* load.voltage_factor;
	actual_load = waterheater_actual_power.Re();

	if (actual_load != 0.0)
	{
		prev_load = actual_load;
		power_state = PS_ON;
	}
	else
		power_state = PS_OFF;

//	gl_enduse_sync(&(residential_enduse::load),t1);

	if(re_override == OV_NORMAL){
		if (time_to_transition >= (1.0/3600.0))	// 0.0167 represents one second
		{
			TIMESTAMP t_to_trans = (TIMESTAMP)(t1+time_to_transition*3600.0/TS_SECOND);
			return -(t_to_trans); // negative means soft transition
		}
		// less than one second means never
		else
			return TS_NEVER; 
	} else {
		return TS_NEVER; // keep running until the forced state ends
	}
}

TIMESTAMP waterheater::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP waterheater::commit(){
	Tw_old = Tw;
	Tupper_old = /*Tupper*/ Tw;
	Tlower_old = Tlower;
	water_demand_old = water_demand;
	return TS_NEVER;
}

/** Tank state determined based on the height of the hot water column
 **/
enumeration waterheater::tank_state(void)
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
void waterheater::set_time_to_transition(void)
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

		case TWONODE:
			switch (load_state) {
				case STABLE:
					time_to_transition = -1; // Negative implies TS_NEVER;
					break;
				case DEPLETING:
					time_to_transition = new_time_2zone(h, 0);
					break;
				case RECOVERING:
					time_to_transition = new_time_2zone(h, height);
					break;
			}
	}
	return;
}

/** Set the water heater model and tank state based on the estimated
	temperature differential along the height of the water column when it is full, 
	emplty or partial at the current height, given the current water draw.
 **/
enumeration waterheater::set_current_model_and_load_state(void)
{
	double dhdt_now = dhdt(h);
	double dhdt_full = dhdt(height);
	double dhdt_empty = dhdt(0.0);
	current_model = NONE;		// by default set it to onenode
	load_state = STABLE;		// by default

	enumeration tank_status = tank_state();

	switch(tank_status) 
	{
		case EMPTY:
			if (dhdt_empty <= 0.0) 
			{
				// If the tank is empty, a negative dh/dt means we're still
				// drawing water, so we'll be switching to the 1-zone model...
				
				/* original plan */
				//current_model = NONE;
				//load_state = DEPLETING;
				
				current_model = ONENODE;
				load_state = DEPLETING;
				Tw = Tupper = Tinlet + HEIGHT_PRECISION;
				Tlower = Tinlet;
				h = height;
				/* empty of hot water? full of cold water! */
				/* it is reconized that this causes a discontinuous jump in the water temperature.
				 * despite that, energy is mostly conserved, since Q => dh until h = 0 (thus no heat in the water).
				 * the +0.01 degF fudge factor for the dhdt() T_diff=0 catch adds about 0.05% of a tank of heat,
				 * less than expected errors from other sources. */
			}
			else if (dhdt_full > 0)
			{
				// overriding the plc code ignoring thermostat logic
				// heating will always be on while in two zone model
				heat_needed = TRUE;
				current_model = TWONODE;
				load_state = RECOVERING;
			}
			else
				load_state = STABLE;
			break;

		case FULL:
			// If the tank is full, a negative dh/dt means we're depleting, so
			// we'll also be switching to the 2-zone model...
			if (dhdt_full < 0)
			{
				// overriding the plc code ignoring thermostat logic
				// heating will always be on while in two zone model
				bool cur_heat_needed = heat_needed;
				heat_needed = TRUE;
				double dhdt_full_temp = dhdt(height);
				if (re_override == OV_OFF)
				{
					current_model = ONENODE;
					load_state = DEPLETING;
					heat_needed = FALSE;
				}
				else if (dhdt_full_temp < 0)
				{
					current_model = TWONODE;
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
			// We're definitely in 2-zone mode.  We have to watch for the
			// case where h's movement stalls out...
			current_model = TWONODE;
			// overriding the plc code ignoring thermostat logic
			// heating will always be on while in two zone model
			heat_needed = TRUE;

			if (dhdt_now < 0 && (dhdt_now * dhdt_empty) >= 0)
				load_state = DEPLETING;
			else if (dhdt_now > 0 && (dhdt_now * dhdt_full) >= 0) 
				load_state = RECOVERING;
			else 
			{
				// dhdt_now is 0, so nothing's happening...
				current_model = NONE;
				load_state = STABLE;
			}
			break;
	}

	return load_state;
}

void waterheater::update_T_and_or_h(double nHours)
{
	/*
		When this gets called (right after the waterheater gets sync'd),
		all states are exactly as they were at the end of the last sync.
		We calculate what has happened to the water temperature (or the
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

		case TWONODE:
			// overriding the plc code ignoring thermostat logic
			// heating will always be on while in two zone model
			heat_needed = TRUE;
			switch (load_state) 
			{
				case STABLE:
					// Change nothing...
					break;
				case DEPLETING:
					// Fall through...
				case RECOVERING:
					try {
						h = new_h_2zone(h, nHours);
					} catch (WRONGMODEL m)
					{
						if (m==MODEL_NOT_2ZONE)
						{
							current_model = ONENODE;
							goto SingleZone;
						}
						else
							GL_THROW("unexpected exception in update_T_and_or_h(%+.1f hrs)", nHours);
					}
					break;
			}

			// Correct h if it overshot...
			if (h < ROUNDOFF) 
			{
				// We've over-depleted the tank slightly.  Make a quickie
				// adjustment to Tlower/Tw to account for it...

				double vol_over = tank_volume/GALPCF * h/height;  // Negative...
				double energy_over = vol_over * RHOWATER * Cp * (/*Tupper*/ Tw - Tlower);
				double Tnew = Tlower + energy_over/Cw;
				Tw = Tlower = Tnew;
				h = 0;
			} 
			else if (h > height) 
			{
				// Ditto for over-recovery...
				double vol_over = tank_volume/GALPCF * (h-height)/height;
				double energy_over = vol_over * RHOWATER * Cp * (/*Tupper*/ Tw - Tlower);
				double Tnew = /*Tupper*/ Tw + energy_over/Cw;
				Tw = /*Tupper*/ Tw = Tnew;
				Tlower = Tinlet;
				h = height;
			} 
			else 
			{
				// Note that as long as h stays between 0 and height, we don't
				// adjust Tlower, even if the Tinlet has changed.  This avoids
				// the headache of adjusting h and is of minimal consequence because
				// Tinlet changes so slowly...
				/*Tupper*/ Tw = Tw;
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
 * cA is the volume change from water draw, heating element, and thermal jacket given a uniformly cold tank
 * cB is the volume change from the extra energy within the hot water node
 */
double waterheater::dhdt(double h)
{
	if (/*Tupper*/ Tw - Tlower < ROUNDOFF)
		return 0.0; // if /*Tupper*/ Tw and Tlower are same then dh/dt = 0.0;

	// Pre-set some algebra just for efficiency...
	const double mdot = water_demand * 60 * RHOWATER / GALPCF;		// lbm/hr...
    const double c1 = RHOWATER * Cp * area * (/*Tupper*/ Tw - Tlower);	// Btu/ft...
	
    // check c1 before dividing by it
    if (c1 <= ROUNDOFF)
        return 0.0; //Possible only when /*Tupper*/ Tw and Tlower are very close, and the difference is negligible

	const double cA = -mdot / (RHOWATER * area) + (actual_kW() * BTUPHPKW + tank_UA * (get_Tambient(location) - Tlower)) / c1;
	const double cb = (tank_UA / height) * (/*Tupper*/ Tw - Tlower) / c1;

	// Returns the rate of change of 'h'
	return cA - cb*h;
}

double waterheater::actual_kW(void)
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
				GL_THROW("Water heater line voltage for waterheater:%d is too high, exceeds twice nominal voltage.",obj->id);
			/*	TROUBLESHOOT
				The waterheater is receiving twice the nominal voltage consistantly, or about 480V on what
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

inline double waterheater::new_time_1node(double T0, double T1)
{
	const double mdot_Cp = Cp * water_demand * 60 * RHOWATER / GALPCF;

    if (Cw <= ROUNDOFF)
        return -1.0;

	const double c1 = ((actual_kW()*BTUPHPKW + tank_UA * get_Tambient(location)) + mdot_Cp*Tinlet) / Cw;
	const double c2 = -(tank_UA + mdot_Cp) / Cw;

    if (fabs(c1 + c2*T1) <= ROUNDOFF || fabs(c1 + c2*T0) <= ROUNDOFF || fabs(c2) <= ROUNDOFF)
        return -1.0;

	const double new_time = (log(fabs(c1 + c2 * T1)) - log(fabs(c1 + c2 * T0))) / c2;	// [hr]
	return new_time;
}

inline double waterheater::new_temp_1node(double T0, double delta_t)
{
	// old because this happens in presync and needs previously used demand
	const double mdot_Cp = Cp * water_demand_old * 60 * RHOWATER / GALPCF;
	// Btu / degF.lb * gal/hr * lb/cf * cf/gal = Btu / degF.hr

    if (Cw <= ROUNDOFF || (tank_UA+mdot_Cp) <= ROUNDOFF)
        return T0;

	const double c1 = (tank_UA + mdot_Cp) / Cw;
	const double c2 = (actual_kW()*BTUPHPKW + mdot_Cp*Tinlet + tank_UA*get_Tambient(location)) / (tank_UA + mdot_Cp);

//	return  c2 - (c2 + T0) * exp(c1 * delta_t);	// [F]
	return  c2 - (c2 - T0) * exp(-c1 * delta_t);	// [F]
}


inline double waterheater::new_time_2zone(double h0, double h1)
{
	const double c0 = RHOWATER * Cp * area * (/*Tupper*/ Tw - Tlower);
	double dhdt0, dhdt1;

    if (fabs(c0) <= ROUNDOFF || height <= ROUNDOFF)
        return -1.0;    // c0 or height should never be zero.  if one of these is zero, there is no definite time to transition

	const double cb = (tank_UA / height) * (/*Tupper*/ Tw - Tlower) / c0;

    if (fabs(cb) <= ROUNDOFF)
        return -1.0;
	dhdt1 = fabs(dhdt(h1));
	dhdt0 = fabs(dhdt(h0));
	double last_timestep = (log(dhdt1) - log(dhdt0)) / -cb;	// [hr]
	return last_timestep;
}

inline double waterheater::new_h_2zone(double h0, double delta_t)
{
	if (delta_t <= ROUNDOFF)
		return h0;

	// old because this happens in presync and needs previously used demand
	const double mdot = water_demand_old * 60 * RHOWATER / GALPCF;		// lbm/hr...
	const double c1 = RHOWATER * Cp * area * (/*Tupper*/ Tw - Tlower); // lb/ft^3 * ft^2 * degF * Btu/lb.degF = lb/lb * ft^2/ft^3 * degF/degF * Btu = Btu/ft

	// check c1 before division
	if (fabs(c1) <= ROUNDOFF)
        return height;      // if /*Tupper*/ Tw and Tlower are real close, then the new height is the same as tank height
//		throw MODEL_NOT_2ZONE;
		
//	#define CWATER		(0.9994)		// BTU/lb/F
	const double cA = -mdot / (RHOWATER * area) + (actual_kW()*BTUPHPKW + tank_UA * (get_Tambient(location) - Tlower)) / c1;
	// lbm/hr / lb/ft + kW * Btu.h/kW + 
	const double cb = (tank_UA / height) * (/*Tupper*/ Tw - Tlower) / c1;

    if (fabs(cb) <= ROUNDOFF)
        return height;

	return ((exp(cb * delta_t) * (cA + cb * h0)) - cA) / cb;	// [ft]
}

double waterheater::get_Tambient(enumeration loc)
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

void waterheater::wrong_model(WRONGMODEL msg)
{
	char *errtxt[] = {"model is not one-zone","model is not two-zone"};
	OBJECT *obj = OBJECTHDR(this);
	gl_warning("%s (waterheater:%d): %s", obj->name?obj->name:"(anonymous object)", obj->id, errtxt[msg]);
	throw msg; // this must be caught by the waterheater code, not by the core
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_waterheater(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(waterheater::oclass);
		if (*obj!=NULL)
		{
			waterheater *my = OBJECTDATA(*obj,waterheater);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else
			return 0;
	}
	CREATE_CATCHALL(waterheater);
}

EXPORT int init_waterheater(OBJECT *obj)
{
	try
	{
		waterheater *my = OBJECTDATA(obj,waterheater);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(waterheater);
}

EXPORT int isa_waterheater(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,waterheater)->isa(classname);
	} else {
		return 0;
	}
}


EXPORT TIMESTAMP sync_waterheater(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	try {
		waterheater *my = OBJECTDATA(obj, waterheater);
		if (obj->clock <= ROUNDOFF)
			obj->clock = t0;  //set the object clock if it has not been set yet
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
		return TS_INVALID;
	}
	SYNC_CATCHALL(waterheater);
}

EXPORT TIMESTAMP commit_waterheater(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	waterheater *my = OBJECTDATA(obj,waterheater);
	return my->commit();
}

EXPORT TIMESTAMP plc_waterheater(OBJECT *obj, TIMESTAMP t0)
{
	// this will be disabled if a PLC object is attached to the waterheater
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the clock if it has not been set yet

	waterheater *my = OBJECTDATA(obj,waterheater);
	my->thermostat(obj->clock, t0);
	
	// no changes to timestamp will be made by the internal water heater thermostat
	/// @todo If external plc codes return a timestamp, it will allow sync sooner but not later than water heater time to transition (ticket #147)
	return TS_NEVER;  
}

/**@}**/
