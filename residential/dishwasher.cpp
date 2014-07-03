/** $Id: dishwasher.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dishwasher.cpp
	@addtogroup dishwasher
	@ingroup residential


 @{
 **/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "dishwasher.h"

//////////////////////////////////////////////////////////////////////////
// dishwasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* dishwasher::oclass = NULL;
CLASS* dishwasher::pclass = NULL;
dishwasher *dishwasher::defaults = NULL;

dishwasher::dishwasher(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		
		// register the class definition
		oclass = gl_register_class(module,"dishwasher",sizeof(dishwasher),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",

			PT_double,"control_power[W]",PADDR(coil_power[0]),			
			PT_double,"dishwasher_coil_power_1[W]",PADDR(coil_power[1]),
			PT_double,"dishwasher_coil_power_2[W]",PADDR(coil_power[2]),
			PT_double,"dishwasher_coil_power_3[W]",PADDR(coil_power[3]),
			PT_double,"motor_power[W]",PADDR(motor_power),
			
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"queue[unit]",PADDR(enduse_queue), PT_DESCRIPTION, "number of loads accumulated",
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),
			PT_double,"total_power[W]",PADDR(total_power),

			PT_enumeration,"state", PADDR(state),
				PT_KEYWORD,"STOPPED",(enumeration)dishwasher_STOPPED,
				PT_KEYWORD,"STALLED",(enumeration)dishwasher_STALLED,
				PT_KEYWORD,"TRIPPED",(enumeration)dishwasher_TRIPPED,
				PT_KEYWORD,"MOTOR_ONLY",(enumeration)dishwasher_MOTOR_ONLY,
				PT_KEYWORD,"MOTOR_COIL_ONLY",(enumeration)dishwasher_MOTOR_COIL_ONLY,
				PT_KEYWORD,"COIL_ONLY",(enumeration)dishwasher_COIL_ONLY,
				PT_KEYWORD,"CONTROL_ONLY",(enumeration)dishwasher_CONTROL_ONLY,
				PT_KEYWORD,"HEATEDDRY_ONLY",(enumeration)dishwasher_HEATEDDRY_ONLY,


			
			PT_double,"energy_baseline[kWh]",PADDR(energy_baseline),			
			PT_double,"energy_used[kWh]",PADDR(energy_used),

			PT_bool,"control_check1",PADDR(control_check1),
			PT_bool,"control_check2",PADDR(control_check2),
			PT_bool,"control_check3",PADDR(control_check3),
			PT_bool,"control_check4",PADDR(control_check4),
			PT_bool,"control_check5",PADDR(control_check5),
			PT_bool,"control_check6",PADDR(control_check6),
			PT_bool,"control_check7",PADDR(control_check7),
			PT_bool,"control_check8",PADDR(control_check8),
			PT_bool,"control_check9",PADDR(control_check9),
			PT_bool,"control_check10",PADDR(control_check10),
			PT_bool,"control_check11",PADDR(control_check11),
			PT_bool,"control_check12",PADDR(control_check12),
			PT_bool,"control_check_temp",PADDR(control_check_temp),

			PT_bool,"motor_only_check1",PADDR(motor_only_check1),
			PT_bool,"motor_only_check2",PADDR(motor_only_check2),
			PT_bool,"motor_only_check3",PADDR(motor_only_check3),
			PT_bool,"motor_only_check4",PADDR(motor_only_check4),
			PT_bool,"motor_only_check5",PADDR(motor_only_check5),
			PT_bool,"motor_only_check6",PADDR(motor_only_check6),
			PT_bool,"motor_only_check7",PADDR(motor_only_check7),
			PT_bool,"motor_only_check8",PADDR(motor_only_check8),
			PT_bool,"motor_only_check9",PADDR(motor_only_check9),

			PT_bool,"motor_only_temp1",PADDR(motor_only_temp1),
			PT_bool,"motor_only_temp2",PADDR(motor_only_temp2),

			PT_bool,"motor_coil_only_check1",PADDR(motor_coil_only_check1),
			PT_bool,"motor_coil_only_check2",PADDR(motor_coil_only_check2),

			PT_bool,"heateddry_check1",PADDR(heateddry_check1),
			PT_bool,"heateddry_check2",PADDR(heateddry_check2),

			PT_bool,"coil_only_check1",PADDR(coil_only_check1),
			PT_bool,"coil_only_check2",PADDR(coil_only_check2),
			PT_bool,"coil_only_check3",PADDR(coil_only_check3),
			
			PT_bool,"Heateddry_option_check",PADDR(Heateddry_option_check),

			PT_double,"queue_min[unit]",PADDR(queue_min),
			PT_double,"queue_max[unit]",PADDR(queue_max),
			

			PT_double,"pulse_interval_1[s]", PADDR(pulse_interval[0]),
			PT_double,"pulse_interval_2[s]", PADDR(pulse_interval[1]),
			PT_double,"pulse_interval_3[s]", PADDR(pulse_interval[2]),
			PT_double,"pulse_interval_4[s]", PADDR(pulse_interval[3]),
			PT_double,"pulse_interval_5[s]", PADDR(pulse_interval[4]),
			PT_double,"pulse_interval_6[s]", PADDR(pulse_interval[5]),
			PT_double,"pulse_interval_7[s]", PADDR(pulse_interval[6]),
			PT_double,"pulse_interval_8[s]", PADDR(pulse_interval[7]),
			PT_double,"pulse_interval_9[s]", PADDR(pulse_interval[8]),
			PT_double,"pulse_interval_10[s]", PADDR(pulse_interval[9]),
			PT_double,"pulse_interval_11[s]", PADDR(pulse_interval[10]),
			PT_double,"pulse_interval_12[s]", PADDR(pulse_interval[11]),
			PT_double,"pulse_interval_13[s]", PADDR(pulse_interval[12]),
			PT_double,"pulse_interval_14[s]", PADDR(pulse_interval[13]),
			PT_double,"pulse_interval_15[s]", PADDR(pulse_interval[14]),
			PT_double,"pulse_interval_16[s]", PADDR(pulse_interval[15]),
			PT_double,"pulse_interval_17[s]", PADDR(pulse_interval[16]),
			PT_double,"pulse_interval_18[s]", PADDR(pulse_interval[17]),
			PT_double,"pulse_interval_19[s]", PADDR(pulse_interval[18]),

					
			PT_double,"dishwasher_run_prob", PADDR(dishwasher_run_prob),
			
			PT_double,"energy_needed[kWh]",PADDR(energy_needed),
			PT_double,"dishwasher_demand[kWh]",PADDR(dishwasher_demand),
			PT_double,"daily_dishwasher_demand[kWh]",PADDR(daily_dishwasher_demand),
			PT_double,"actual_dishwasher_demand[kWh]",PADDR(actual_dishwasher_demand),

			PT_double,"motor_on_off",PADDR(motor_on_off),
			PT_double,"motor_coil_on_off",PADDR(motor_coil_on_off),
			

			PT_bool,"is_240",PADDR(is_240), PT_DESCRIPTION, "load is 220/240 V (across both phases)",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

dishwasher::~dishwasher()
{
}

int dishwasher::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 0.95;
	load.power_fraction = 1;
	is_240 = true;

	state = dishwasher_STOPPED;
	
	energy_used = 0;
	
	coil_power[0] = -1;
	motor_on_off = motor_coil_on_off = both_coils_on_off = 0;

	last_t = 0;
	

	gl_warning("explicit %s model is experimental and has not been validated", OBJECTHDR(this)->oclass->name);
	/* TROUBLESHOOT
		The dishwasher explicit model has some serious issues and should be considered for complete
		removal.  It is highly suggested that this model NOT be used.
	*/

	return res;
}

int dishwasher::init(OBJECT *parent)
{
	// @todo This class has serious problems and should be deleted and started from scratch. Fuller 9/27/2013.

	OBJECT *hdr = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("dishwasher::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	int rv = 0;
	// default properties
	if (motor_power==0) motor_power = gl_random_uniform(&hdr->rng_state,150,350);
	if (heat_fraction==0) heat_fraction = 0.2;
	if (is_240)
	{
		load.config = EUC_IS220;
		if (stall_voltage==0) stall_voltage  = 0.6*240;
	}
	else
		if (stall_voltage==0) stall_voltage  = 0.6*120;

	if (trip_delay==0) trip_delay = 10;
	if (reset_delay==0) reset_delay = 60;

	count_motor_only = 0;
	count_motor_only1 =0;
	count_motor_only_25 = 0;
	count_coil_only = 0;
	count_control_only =0;
	count_control_only1 =0;
	count_motor_only_68 =0;
	
	pulse_interval[0] = 8;
	pulse_interval[1] = 18;
	pulse_interval[2] = 21;
	pulse_interval[3] = 28;
	pulse_interval[4] = 38;
	pulse_interval[5] = 50;
	pulse_interval[6] = 65;

	pulse_interval[7] = 118;
	pulse_interval[8] = 150;
	pulse_interval[9] = 220;
	pulse_interval[10] = 320;
	pulse_interval[11] = 355;
	pulse_interval[12] = 460;
	pulse_interval[13] = 580;

	pulse_interval[14] = 615;
	pulse_interval[15] = 780;
	pulse_interval[16] = 800;
	pulse_interval[17] = 950;
	pulse_interval[18] = 1150;


	if (coil_power[0]==-1) coil_power[0] = 5800;

	coil_power[0] = 10;
	coil_power[1] = 580;
	coil_power[2] = 695;
	coil_power[3] = 950;
	motor_power = 250;

	enduse_queue = 1;
	queue_min = 0;
	queue_max = 2;

				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;
				//Heateddry_option_check = false;


	hdr->flags |= OF_SKIPSAFE;

	load.power_factor = 0.95;
	load.breaker_amps = 30;
	actual_dishwasher_demand = 0;
	energy_needed = 0;

	rv = residential_enduse::init(parent);

	if (rv==SUCCESS) update_state(0.0);

	/* schedule checks */
	switch(shape.type){
		case MT_UNKNOWN:
			gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("dishwasher does not support fixed energy shaping");

			} else if (shape.params.analog.power == 0){

				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				daily_dishwasher_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dishwasher demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated dishwasher usage is nonsensical for residential dishwashers");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, low-level dishwasher demand, it is thoroughly
					counterintuitive to the normal usage of the dishwasher.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dishwasher demand X kW, limited by C + Q * h */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("dishwasher load shape has an unknown state!");
			break;
	}
	return residential_enduse::init(parent);
//}
	// must run before update_state() so that pCircuit can be set


//	return rv;
}

int dishwasher::isa(char *classname)
{
	return (strcmp(classname,"dishwasher")==0 || residential_enduse::isa(classname));
}


TIMESTAMP dishwasher::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double dt = 0;
	TIMESTAMP t3;

	/* determine loadshape effects */

	TIMESTAMP t2 = residential_enduse::sync(t0,t1);

	if ((last_t != t0) && (last_t != 0))	//Different timestamp
	{
		// compute the seconds in this time step
		dt = gl_toseconds(t0>0?t0-last_t:0);

		// if advancing from a non-init condition
		if (t0>TS_ZERO && dt>0)
		{
			// compute the total energy usage in this interval
			load.energy += load.total * dt/3600.0;

			//Update tracking variable
			last_t = t0;
		}

		dt = update_state(dt); //, t1);
	}
	else if (last_t == 0)
	{
		last_t = t0;
	}
	//Defaulted other else, do nothing

	t3 = (TIMESTAMP)(dt*TS_SECOND+t0);

	if (t3>t2)
		return -t3;
	else
		return t2;
}

TIMESTAMP dishwasher::presync(TIMESTAMP t0, TIMESTAMP t1){

	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("dishwasher does not support fixed energy shaping");

			} else if (shape.params.analog.power == 0){

				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				daily_dishwasher_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dishwasher demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated dishwasher usage is nonsensical for residential dishwashers");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, low-level dishwasher demand, it is thoroughly
					counterintuitive to the normal usage of the dishwasher.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dishwasher demand X kW, limited by C + Q * h */
				daily_dishwasher_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("dishwasher load shape has an unknown state!");
			break;
	}
	return TS_NEVER;
}

double dishwasher::update_state(double dt) //,TIMESTAMP t1)
{	
	// accumulate the energy

	OBJECT *hdr = OBJECTHDR(this);

	energy_used += total_power/1000 * dt/3600;


switch(state) {

	case dishwasher_STOPPED:

		if (enduse_queue>1)// && dryer_on == true)
		
			dishwasher_run_prob = double(gl_random_uniform(&hdr->rng_state,queue_min,queue_max));
		
		if (enduse_queue > 1 && (dishwasher_run_prob > enduse_queue))
			{
				state = dishwasher_CONTROL_ONLY;
				if (Heateddry_option_check == true)
				energy_needed = energy_baseline;
				else
				energy_needed = 0.66*energy_baseline;

				cycle_duration = cycle_time = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
				cycle_time = pulse_interval[5];
				enduse_queue--;

				new_running_state = true;
				
			}
		else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state = dishwasher_STALLED;
				state_time = 0;
			}
		break;

	case dishwasher_CONTROL_ONLY:

		if (energy_used >= energy_needed || cycle_time <= 0)
		{  
				if (energy_used >= energy_needed)
				{
					state = dishwasher_STOPPED;
					cycle_time = 0;
					energy_used = 0;

				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;


				new_running_state = true;			

			}
		else if (cycle_time<=0)
			{			

				if (cycle_time<=0 && (control_check1 == false || control_check_temp == true))
				{
					state = dishwasher_MOTOR_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
					double interval = pulse_interval[0];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					control_check1 = true;
					control_check2 = true;
				}

				if (cycle_time<=0 && control_check2 == true)
				{
					state = dishwasher_MOTOR_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
					double interval = pulse_interval[7];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					control_check2 = false;
					control_check6 = true;
						
				}
				
				if (cycle_time<=0 && control_check3 == true)
				{
					state = dishwasher_MOTOR_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
					double interval = pulse_interval[6];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					control_check3 = false;
					control_check7 = true;
				}
				
				if (cycle_time<=0 && control_check4 == true)
				{
					state = dishwasher_MOTOR_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;								
					double interval = pulse_interval[9];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					control_check4 = false;
					control_check5 = true;
				}

				if (cycle_time<=0 && control_check5 == true)
				{
					state = dishwasher_MOTOR_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;								
					double interval = pulse_interval[4];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
									
					control_check5 = false;
					control_check8 = true;
				}

				
				
				if (cycle_time<=0 && control_check6 == true)
				{
					state = dishwasher_COIL_ONLY;

					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[1] * 60 * 60;

					double interval = pulse_interval[0];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					control_check6 = false;
					control_check3 = true;
									
				}


				if(cycle_time<=0 && (control_check7 == true))
				{

					state = dishwasher_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[3]) * 60 * 60;
					double interval = pulse_interval[10];
						
						if (cycle_t > interval)
						cycle_time = interval;
						else
						cycle_time = cycle_t;	

						control_check7 = false;	
						control_check4 = true;	
				}


				if (cycle_time<=0 && control_check8 == true && count_control_only<5)
				{
					state = dishwasher_COIL_ONLY;

					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[1] * 60 * 60;

					double interval = pulse_interval[0];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					
					count_control_only += 1;

					if (count_control_only == 4)
					{
					
					control_check8 = false;
					control_check9 = true;	
					}
				}


				if (cycle_time<=0 && control_check9 == true && count_control_only1 < 7)
				{
					state = dishwasher_MOTOR_ONLY;

					double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;

					double interval = pulse_interval[0];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	

					count_control_only1 +=1;
					if (count_control_only1 ==6)
					{
					control_check9 = false;
					control_check10 = true;	
					}
				}

				if(cycle_time<=0 && (control_check10 == true))
				{

					state = dishwasher_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[3]) * 60 * 60;
					double interval = pulse_interval[17];
						
						if (cycle_t > interval)
						cycle_time = interval;
						else
						cycle_time = cycle_t;	

						control_check10 = false;	
						control_check11 = true;	
				}


				if(cycle_time<=0 && control_check11 == true && Heateddry_option_check == true)
				{
					    state = dishwasher_HEATEDDRY_ONLY;

						double cycle_t = 1000 * (energy_needed - energy_used) / (coil_power[2]) * 60 * 60;
						double interval = pulse_interval[16];

						if (cycle_t > interval)
						cycle_time = interval;
						else
						cycle_time = cycle_t;	
						control_check11 = false;
						control_check12 = true;
				}

				if(cycle_time<=0 && control_check12 == true && Heateddry_option_check == true)
				{
					    state = dishwasher_HEATEDDRY_ONLY;

						double cycle_t = 1000 * (energy_needed - energy_used) / (coil_power[2]) * 60 * 60;
						double interval = pulse_interval[15];

						if (cycle_t > interval)
						cycle_time = interval;
						else
						cycle_time = cycle_t;	
						control_check12 = false;	
				}
				


						new_running_state = true;
			}		
		
		
		else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state = dishwasher_STALLED;
				state_time = 0;
			}
	}
		break;

case dishwasher_MOTOR_ONLY:

		if (energy_used >= energy_needed || cycle_time<=0)
		{ 
			if (energy_used >= energy_needed)
			{
			state = dishwasher_STOPPED;
			cycle_time = 0;

				energy_used = 0;

				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;


				new_running_state = true;		
			}		

		else if (cycle_time<=0)
			{
				state = dishwasher_CONTROL_ONLY;	
					
				if(cycle_time<=0 && (motor_only_check1 == false))
				
				{

					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[5];
					

					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					motor_only_check1 = true;	
					motor_only_check2 = true;
				}
											

				if(cycle_time<=0 && motor_only_check2 == true)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;

					double interval = pulse_interval[3];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;
					
					motor_only_check2 = false;	

					motor_only_check3 = true;
				}
				
				if(cycle_time<=0 && motor_only_check3 == true && count_motor_only_68 < 3)

				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[5];

					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					count_motor_only_68 +=1;

					if (count_motor_only_68 ==2)
					{
					motor_only_check3 = false;	
					motor_only_check4 = true;	
					
					}
				}


				if(cycle_time<=0 && motor_only_check4 == true)
				{
					
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[2];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;
					motor_only_check4 = false;	
					motor_only_check5 = true;	
				}

				if(cycle_time<=0 && motor_only_check5 == true && count_motor_only_25 <6)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[1];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					count_motor_only_25 +=1;
					if (count_motor_only_25 == 5)
					{
					motor_only_check5 = false;	
					motor_only_check6 = true;	
					}
				}

				if(cycle_time<=0 && motor_only_check6 == true)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[7];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;
					motor_only_check6 = false;	
					motor_only_check7 = true;	
					
				}

				if(cycle_time<=0 && motor_only_check7 == true && count_motor_only <6)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[4];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					count_motor_only +=1;
					if (count_motor_only ==5)
					{
					motor_only_check7 = false;	
					motor_only_check8 = true;	
					}
					
				}

				if(cycle_time<=0 && motor_only_check8 == true)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[8];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					motor_only_check8 = false;	
					motor_only_check9 = true;	
					
				}

				if(cycle_time<=0 && motor_only_check9 == true)
				{
					
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[12];
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					count_motor_only1 +=1;

					motor_only_check9 = false;	

				}		

					new_running_state = true;
			}		


		else if (pCircuit->pV->Mag()<stall_voltage)
			{
					state = dishwasher_STALLED;
					state_time = 0;
			}
		}
			
	break;
		
case dishwasher_MOTOR_COIL_ONLY:

	if (energy_used >= energy_needed || cycle_time <= 0)
		{  				
				if (energy_used >= energy_needed)
				{
				state = dishwasher_STOPPED;
				cycle_time = 0;
				energy_used = 0;

				
				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;


				new_running_state = true;		

		}
	else if (cycle_time<=0)
		{
				state = dishwasher_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				

				if (cycle_time<=0 && motor_coil_only_check1 == false)
				{
					double interval = pulse_interval[11];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	

					motor_coil_only_check1 = true;
					motor_coil_only_check2 = true;
				}

				if(cycle_time<=0 && motor_coil_only_check2 == true)
				{
					double interval = pulse_interval[13];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					
					motor_coil_only_check2 = false;					
				}

					new_running_state = true;
		}		


	else if (pCircuit->pV->Mag()<stall_voltage)
		{
			state = dishwasher_STALLED;
			state_time = 0;
		}
	}
	break;

	

case dishwasher_HEATEDDRY_ONLY:
	
	if (energy_used >= energy_needed || cycle_time <= 0 )
		{  
			if (energy_used >= energy_needed)
			{
			state = dishwasher_STOPPED;
			cycle_time = 0;

				energy_used = 0;
				
				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;


				new_running_state = true;		
		}		

	else if (cycle_time<=0 && heateddry_check1 == false)
			{
					state = dishwasher_CONTROL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / coil_power[0] * 60 * 60;
					double interval = pulse_interval[7];

					heateddry_check1 = true;
					heateddry_check2 = true;

					if (cycle_t > interval)
						cycle_time = interval;
					else
					cycle_time = cycle_t;


						new_running_state = true;
			}		


	else if (cycle_time<=0 && heateddry_check2 == true)
			{
					state = dishwasher_CONTROL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (coil_power[0]) * 60 * 60;
					double interval = pulse_interval[18];
					

					if (cycle_t > interval)
						cycle_time = interval;
					else
					cycle_time = cycle_t;
					motor_only_check2 = false;

					new_running_state = true;
			}	
			


	else if (pCircuit->pV->Mag()<stall_voltage)
			{
					state = dishwasher_STALLED;
					state_time = 0;
			}
	}
			
	break;

case dishwasher_COIL_ONLY:

	if (energy_used >= energy_needed || cycle_time <= 0)
		{  
			if (energy_used >= energy_needed)
			{
				state = dishwasher_STOPPED;
				cycle_time = 0;
				energy_used = 0;

				
				control_check1 = false;
				control_check2 = false;
				control_check3 = false;

				control_check4 = false;
				control_check5 = false;
				control_check6 = false;

				control_check7 = false;
				control_check8 = false;
				control_check9 = false;
				control_check10 = false;
				control_check11 = false;
				control_check12 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;

				motor_only_check5 = false;
				motor_only_check6 = false;
				motor_only_check7 = false;
				motor_only_check8 = false;
				motor_only_check9 = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;

				heateddry_check1 = false;
				heateddry_check2 = false;

				coil_only_check1 = false;
				coil_only_check2 = false;
				coil_only_check3 = false;


				new_running_state = true;		

		}
	else if (cycle_time<=0)
		{
				state = dishwasher_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				
				if (cycle_time<=0 && coil_only_check1 == false)
				{
					double interval = pulse_interval[8];
					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;

					coil_only_check1 = true;
					coil_only_check2 = true;
				}
				if(cycle_time<=0 && coil_only_check2 == true && count_coil_only <4)
				{
					double interval = pulse_interval[4];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	

					count_coil_only +=1;
					if (count_coil_only == 3)
					{
					coil_only_check2 = false;	
					coil_only_check3 = true;
					}
				}

				if(cycle_time<=0 && coil_only_check3 == true)
				{
					double interval = pulse_interval[14];

					if (cycle_t > interval)
					cycle_time = interval;
					else
					cycle_time = cycle_t;	
					coil_only_check3 = false;	
					
				}
				new_running_state = true;
		}		

	
	else if (pCircuit->pV->Mag()<stall_voltage)
		{
			state = dishwasher_STALLED;
			state_time = 0;
		}
	}
	break;

case dishwasher_STALLED:

		if (pCircuit->pV->Mag()>start_voltage)
		{
			state = dishwasher_MOTOR_ONLY;
			state_time = cycle_time;
		}
		else if (state_time>trip_delay)
		{
			state = dishwasher_TRIPPED;
			state_time = 0;
		}

	break;

case dishwasher_TRIPPED:

		if (state_time>reset_delay)
		{
			if (pCircuit->pV->Mag()>start_voltage)
				state = dishwasher_MOTOR_ONLY;
			else
				state = dishwasher_STALLED;
				state_time = 0;
		}

		break;
	}

	// accumulating units in the queue no matter what happens
	enduse_queue += daily_dishwasher_demand * dt/3600/24;
	//actual_dishwasher_demand = actual_dishwasher_demand + enduse_queue;
	
	// now implement current state
	switch(state) {
	case dishwasher_STOPPED: 
		
		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);

		dt = ((enduse_queue>=1) || (enduse_queue==0)) ? 0 : ((1-enduse_queue)*3600)/(enduse_queue*24); 				

		break;

	case dishwasher_MOTOR_COIL_ONLY:

		cycle_time -= dt;		
		load.power.SetPowerFactor(motor_power/1000, load.power_factor);
		load.admittance = complex((coil_power[3])/1000,0,J); //assume pure resistance
		load.current = complex(0,0,J);

		dt = cycle_time;
	break;

	case dishwasher_COIL_ONLY:

		cycle_time -= dt;	 
		load.power = load.current = complex(0,0,J);
		load.admittance = complex((coil_power[1])/1000,0,J); //assume pure resistance
	
	dt = cycle_time;
	break;


	case dishwasher_HEATEDDRY_ONLY:
	
	cycle_time -= dt;
	load.power = load.current = complex(0,0,J);
	load.admittance = complex((coil_power[2])/1000,0,J); //assume pure resistance
	
	dt = cycle_time;
	break;

	case dishwasher_CONTROL_ONLY:

		if(true==new_running_state){
			
			new_running_state = false;

		}
		else{

			cycle_time -= dt;
		}
		load.power = load.current = complex(0,0,J);
		load.admittance = complex(coil_power[0]/1000,0,J);

		dt = cycle_time;
		break;

	case dishwasher_STALLED:

		load.power = load.current = complex(0,0,J);
		load.admittance = complex(1)/stall_impedance;

		dt = trip_delay;

		break;

	case dishwasher_TRIPPED:

		load.power = load.current = load.admittance = complex(0,0,J);
		
		dt = reset_delay; 

		break;


	case dishwasher_MOTOR_ONLY:
		motor_on_off = 1;
		motor_coil_on_off = both_coils_on_off = 0;
		cycle_time -= dt;

		load.power.SetPowerFactor(motor_power/1000, load.power_factor);
		load.admittance = complex(0,0,J); 
		load.current = complex(0,0,J);

		dt = cycle_time;
		break;

	default:

		throw "unexpected motor state";
		/*	TROUBLESHOOT
			This is an error.  Please submit a bug report along with at the dishwasher
			object & class sections from the relevant GLM file, and from the dump file.
		*/
		break;
	}

	// compute the total electrical load - first for the enduse structure and second for an internal variable
	load.total = load.power + load.current + load.admittance;
	total_power = (load.power.Re() + (load.current.Re() + load.admittance.Re()*load.voltage_factor)*load.voltage_factor) * 1000;

	// compute the total heat gain
	load.heatgain = load.total.Mag() * heat_fraction;


	if (dt > 0 && dt < 1)
		dt = 1;

	return dt;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////


EXPORT TIMESTAMP sync_dishwasher(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP tret;
	dishwasher *my = OBJECTDATA(obj, dishwasher);
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the object clock if it has not been set yet
	try {
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return my->presync(obj->clock, t0);
		case PC_BOTTOMUP:
			tret = my->sync(obj->clock, t0);
			obj->clock = t0;
			return tret;
		default:
			throw "invalid pass request";
		}
	}
	catch (int m)
	{
		gl_error("%s (dishwasher:%d) model zone exception (code %d) not caught", obj->name?obj->name:"(anonymous dishwasher)", obj->id, m);
	}
	catch (char *msg)
	{
		gl_error("%s (dishwasher:%d) %s", obj->name?obj->name:"(anonymous dishwasher)", obj->id, msg);
	}
	return TS_INVALID;
}

EXPORT int create_dishwasher(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(dishwasher::oclass);
	if (*obj!=NULL)
	{
		dishwasher *my = OBJECTDATA(*obj,dishwasher);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_dishwasher(OBJECT *obj)
{
	dishwasher *my = OBJECTDATA(obj,dishwasher);
	return my->init(obj->parent);
}

EXPORT int isa_dishwasher(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,dishwasher)->isa(classname);
	} else {
		return 0;
	}
}

//EXPORT TIMESTAMP sync_dishwasher(OBJECT *obj, TIMESTAMP t0)
//{
//	dishwasher *my = OBJECTDATA(obj, dishwasher);
//	TIMESTAMP t1 = my->sync(obj->clock, t0);
//	obj->clock = t0;
//	return t1;
//}

/**@}**/