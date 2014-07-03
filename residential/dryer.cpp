/** $Id: dryer.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file dryer.cpp
	@addtogroup dryer
	@ingroup residential

	The dryer simulation is based on an hourly demand profile attached to the object.
	The internal heat gain is calculated using a specified fraction of installed power.
	The queue is used to determine the probability of a load being run during that hour.
	Demand is added to the queue until the queue becomes large enough to trigger an event.
	The motor model is simplified in that it assume the motor all the time during the load run
	(which is not really true).  The duration of the load is a configurable parameter.

	Motor stalling is implemented when the voltage is below about 0.7 puV.  A stalled motor 
	remains in the constant impedance state for about 10 seconds before tripping off.  When
	tripped, the motor will attempt a restart after about 60 seconds, regardless of voltage.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_e.h"
#include "dryer.h"

//////////////////////////////////////////////////////////////////////////
// dryer CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* dryer::oclass = NULL;
CLASS* dryer::pclass = NULL;
dryer *dryer::defaults = NULL;

dryer::dryer(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		pclass = residential_enduse::oclass;
		
		// register the class definition
		oclass = gl_register_class(module,"dryer",sizeof(dryer),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"motor_power[W]",PADDR(motor_power),
			PT_double,"dryer_coil_power[W]",PADDR(coil_power[0]),
			PT_double,"controls_power[W]",PADDR(controls_power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"queue[unit]",PADDR(enduse_queue), PT_DESCRIPTION, "number of loads accumulated",
			PT_double,"queue_min[unit]",PADDR(queue_min),
			PT_double,"queue_max[unit]",PADDR(queue_max),
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),
			PT_double,"total_power[W]",PADDR(total_power),

			PT_enumeration,"state", PADDR(state),
				PT_KEYWORD,"STOPPED",(enumeration)DRYER_STOPPED,
				PT_KEYWORD,"STALLED",(enumeration)DRYER_STALLED,
				PT_KEYWORD,"TRIPPED",(enumeration)DRYER_TRIPPED,
				PT_KEYWORD,"MOTOR_ONLY",(enumeration)DRYER_MOTOR_ONLY,
				PT_KEYWORD,"MOTOR_COIL_ONLY",(enumeration)DRYER_MOTOR_COIL_ONLY,
				PT_KEYWORD,"CONTROL_ONLY",(enumeration)DRYER_CONTROL_ONLY,

			
			PT_double,"energy_baseline[kWh]",PADDR(energy_baseline),			
			PT_double,"energy_used[kWh]",PADDR(energy_used),
			PT_double,"next_t",PADDR(next_t),
			PT_bool,"control_check",PADDR(control_check),

			PT_bool,"motor_only_check1",PADDR(motor_only_check1),
			PT_bool,"motor_only_check2",PADDR(motor_only_check2),
			PT_bool,"motor_only_check3",PADDR(motor_only_check3),
			PT_bool,"motor_only_check4",PADDR(motor_only_check4),
			PT_bool,"motor_only_check5",PADDR(motor_only_check5),
			PT_bool,"motor_only_check6",PADDR(motor_only_check6),

			PT_bool,"dryer_on",PADDR(dryer_on),
			PT_bool,"dryer_ready",PADDR(dryer_ready),

			PT_bool,"dryer_check",PADDR(dryer_check),

			PT_bool,"motor_coil_only_check1",PADDR(motor_coil_only_check1),
			PT_bool,"motor_coil_only_check2",PADDR(motor_coil_only_check2),
			PT_bool,"motor_coil_only_check3",PADDR(motor_coil_only_check3),
			PT_bool,"motor_coil_only_check4",PADDR(motor_coil_only_check4),
			PT_bool,"motor_coil_only_check5",PADDR(motor_coil_only_check5),
			PT_bool,"motor_coil_only_check6",PADDR(motor_coil_only_check6),

			PT_double,"dryer_run_prob", PADDR(dryer_run_prob),			
			PT_double,"dryer_turn_on", PADDR(dryer_turn_on),

			PT_double,"pulse_interval_1[s]", PADDR(pulse_interval[0]),
			PT_double,"pulse_interval_2[s]", PADDR(pulse_interval[1]),
			PT_double,"pulse_interval_3[s]", PADDR(pulse_interval[2]),
			PT_double,"pulse_interval_4[s]", PADDR(pulse_interval[3]),
			PT_double,"pulse_interval_5[s]", PADDR(pulse_interval[4]),
			PT_double,"pulse_interval_6[s]", PADDR(pulse_interval[5]),
			PT_double,"pulse_interval_7[s]", PADDR(pulse_interval[6]),

			
			PT_double,"energy_needed[kWh]",PADDR(energy_needed),
			PT_double,"daily_dryer_demand[kWh]",PADDR(daily_dryer_demand),
			PT_double,"actual_dryer_demand[kWh]",PADDR(actual_dryer_demand),
			PT_double,"motor_on_off",PADDR(motor_on_off),
			PT_double,"motor_coil_on_off",PADDR(motor_coil_on_off),
			

			PT_bool,"is_240",PADDR(is_240), PT_DESCRIPTION, "load is 220/240 V (across both phases)",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

dryer::~dryer()
{
}

int dryer::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 0.95;
	load.power_fraction = 1;
	is_240 = true;

	coil_power[0] = -1;

	state = DRYER_STOPPED;
	
	energy_used = 0;	
	
	last_t = 0;	

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);

	return res;
}

int dryer::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("dryer::init(): deferring initialization on %s", gl_name(parent, objname, 255));
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

	if (coil_power[0]==-1) coil_power[0] = 5800;

	start_time = 0;
	dryer_run_prob = 0;

	control_check = false;
	dryer_check = false;
	next_t = 0;

	pulse_interval[0] = 21.6;
	pulse_interval[1] = 32.4;
	pulse_interval[2] = 1287;
	pulse_interval[3] = 85.8 ;
	pulse_interval[4] = 129;
	pulse_interval[5] = 138;
	pulse_interval[6] = 60;

    controls_power = 10;
	motor_power = 200;
	coil_power[0] = 5800;

	enduse_queue = 0.8;
	queue_min = 0;
	queue_max = 2;

	motor_coil_only_check1 = false;
	motor_coil_only_check2 = false;
	motor_coil_only_check3 = false;
	motor_coil_only_check4 = false;
	motor_coil_only_check5 = false;
	motor_coil_only_check6 = false;

	motor_only_check1 = false;
	motor_only_check2 = false;
	motor_only_check3 = false;
	motor_only_check4 = false;
	motor_only_check5 = false;
	motor_only_check6 = false;


	hdr->flags |= OF_SKIPSAFE;

	load.power_factor = 0.95;
	load.breaker_amps = 30;
	actual_dryer_demand = 0;
	energy_needed = 0;

	rv = residential_enduse::init(parent);

	if (rv==SUCCESS) update_state(0.0);

	/* schedule checks */
	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("dryer does not support fixed energy shaping");

			} else if (shape.params.analog.power == 0){

				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				daily_dryer_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dryer demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated dryer usage is nonsensical for residential dryers");
				/*	TROUBLESHOOT
					Though it is possible to put a constant, low-level dryer demand, it is thoroughly
					counterintuitive to the normal usage of the dryer.
				 */
			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dryer demand X kW, limited by C + Q * h */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("dryer load shape has an unknown state!");
			break;
	}
	return residential_enduse::init(parent);
//}
	// must run before update_state() so that pCircuit can be set


//	return rv;
}

int dryer::isa(char *classname)
{
	return (strcmp(classname,"dryer")==0 || residential_enduse::isa(classname));
}


TIMESTAMP dryer::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	double dt = 0;
	TIMESTAMP t3;

	if(((t1-start_time)%3600)==0 && start_time>0)	
		dryer_on = true;
	else
		dryer_on = false;	


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

TIMESTAMP dryer::presync(TIMESTAMP t0, TIMESTAMP t1){

	if(start_time==0)
	{
		start_time = int32(t0);
		
	}	


	switch(shape.type){
		case MT_UNKNOWN:
			/* normal, undriven behavior. */
			break;
		case MT_ANALOG:
			if(shape.params.analog.energy == 0.0){
				GL_THROW("dryer does not support fixed energy shaping");

			} else if (shape.params.analog.power == 0){

				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			} else {
				daily_dryer_demand = gl_get_loadshape_value(&shape); /* unitless ~ drive gpm */
			}
			break;
		case MT_PULSED:
			/* pulsed loadshapes "emit one or more pulses at random times s. t. the total energy is accumulated over the period of the loadshape".
			 * pulsed loadshapes can either user time or kW values per pulse. */
			if(shape.params.pulsed.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons to drive heater for Y hours ~ but what's Vdot, what's t? */
			} else if(shape.params.pulsed.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dryer demand X kW, limited by C + Q * h ~ Vdot proportional to power/time */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_MODULATED:
			if(shape.params.modulated.pulsetype == MPT_TIME){
				GL_THROW("Amplitude modulated dryer usage is nonsensical for residential dryers");

			} else if(shape.params.modulated.pulsetype == MPT_POWER){
				/* frequency modulated */
				/* fixed-amplitude, varying length pulses at regular intervals. */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		case MT_QUEUED:
			if(shape.params.queued.pulsetype == MPT_TIME){
				; /* constant time pulse ~ consumes X gallons/minute to consume Y thermal energy */
			} else if(shape.params.queued.pulsetype == MPT_POWER){
				; /* constant power pulse ~ dryer demand X kW, limited by C + Q * h */
				daily_dryer_demand = gl_get_loadshape_value(&shape) / 2.4449;
			}
			break;
		default:
			GL_THROW("dryer load shape has an unknown state!");
			break;
	}
	return TS_NEVER;
}

double dryer::update_state(double dt) //,TIMESTAMP t1)
{	
	OBJECT *hdr = OBJECTHDR(this);
	// accumulate the energy
	energy_used += total_power/1000 * dt/3600;


switch(state) {

	case DRYER_STOPPED:

		if (enduse_queue>1)// && dryer_on == true)
		
			dryer_run_prob = double(gl_random_uniform(&hdr->rng_state,queue_min,queue_max));
		
		if (enduse_queue > 1 && (dryer_run_prob > enduse_queue))
			{
				state = DRYER_CONTROL_ONLY;
				energy_needed = energy_baseline;
				cycle_duration = cycle_time = 1000 * (energy_needed - energy_used) / controls_power * 60 * 60;
				cycle_time = pulse_interval[0];
				//cycle_duration_dryer = pulse_interval[0];
				enduse_queue--;

				new_running_state = true;
				
			}
		else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state = DRYER_STALLED;
				state_time = 0;
			}
		break;

	case DRYER_CONTROL_ONLY:

		if (energy_used >= energy_needed && cycle_time <= 0)
			{  // The clothes are dry
				state = DRYER_STOPPED;
				cycle_time = 0;
				energy_used = 0;

				control_check = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;
				motor_coil_only_check3 = false;
				motor_coil_only_check4 = false;
				motor_coil_only_check5 = false;
				motor_coil_only_check6 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;
				motor_only_check5 = false;
				motor_only_check6 = false;

				new_running_state = true;			


			}
		else if (cycle_time<=0 && control_check == false)//one-over
			{
					state = DRYER_MOTOR_COIL_ONLY;;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[1];
					control_check = true;
					motor_coil_only_check1 = true;

						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;	

						new_running_state = true;
			}		

		
		else if (pCircuit->pV->Mag()<stall_voltage)
			{
				state = DRYER_STALLED;
				state_time = 0;
			}
		break;

		
case DRYER_MOTOR_COIL_ONLY:

	if (energy_used >= energy_needed && cycle_time <= 0)
		{  // The clothes are dry
				state = DRYER_STOPPED;
				cycle_time = 0;
				energy_used = 0;

				control_check = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;
				motor_coil_only_check3 = false;
				motor_coil_only_check4 = false;
				motor_coil_only_check5 = false;
				motor_coil_only_check6 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;
				motor_only_check5 = false;
				motor_only_check6 = false;

				new_running_state = true;

		}
	else if (cycle_time<=0 && motor_coil_only_check1 == true)//one-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				double interval = pulse_interval[0];
				motor_coil_only_check1 = false;
				motor_coil_only_check2 = true;
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;	

					new_running_state = true;
		}		

	else if (cycle_time<=0 && motor_coil_only_check2 == true)//two-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				double interval = pulse_interval[3];
				motor_coil_only_check2 = false;
				motor_coil_only_check3 = true;
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;
				new_running_state = true;
		}	

	else if (cycle_time<=0 && motor_coil_only_check3 == true)//three-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				double interval = pulse_interval[3];
				motor_coil_only_check3 = false;
				motor_coil_only_check4 = true;
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;

					new_running_state = true;
		}	

	else if (cycle_time<=0 && motor_coil_only_check4 == true)//four-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				double interval = pulse_interval[3];
				motor_coil_only_check4 = false;
				motor_coil_only_check5 = true;
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;	
				new_running_state = true;
		}	
	else if (cycle_time<=0 && motor_coil_only_check5 == true)//five-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				double interval = pulse_interval[5];
				motor_coil_only_check5 = false;
				motor_coil_only_check6 = true;
					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;		

				new_running_state = true;
		}

	else if (cycle_time<=0 && motor_coil_only_check6 == true)//five-over
		{
				state = DRYER_MOTOR_ONLY;
				double cycle_t = 1000 * (energy_needed - energy_used) / motor_power * 60 * 60;
				motor_only_check6 = false;
				new_running_state = true;
/*					if (cycle_t > interval)
						cycle_time = interval;
					else
						cycle_time = cycle_t;	*/		
		}
	else if (pCircuit->pV->Mag()<stall_voltage)
		{
			state = DRYER_STALLED;
			state_time = 0;
		}
	break;

	case DRYER_MOTOR_ONLY:

		if (energy_used >= energy_needed)
		{  // The clothes are dry
			state = DRYER_STOPPED;
			cycle_time = 0;

				energy_used = 0;
				control_check = false;

				motor_coil_only_check1 = false;
				motor_coil_only_check2 = false;
				motor_coil_only_check3 = false;
				motor_coil_only_check4 = false;
				motor_coil_only_check5 = false;
				motor_coil_only_check6 = false;

				motor_only_check1 = false;
				motor_only_check2 = false;
				motor_only_check3 = false;
				motor_only_check4 = false;
				motor_only_check5 = false;
				motor_only_check6 = false;

				new_running_state = true;
		}		

		else if (cycle_time<=0 && motor_only_check1 == false)//one-over
			{
					state = DRYER_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[2];

					motor_only_check1 = true;
					motor_only_check2 = true;

						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;
						new_running_state = true;
			}		


		else if (cycle_time<=0 && motor_only_check2 == true)//two-over
			{
					state = DRYER_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[4];

					motor_only_check2 = false;
					motor_only_check3 = true;

						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;

						new_running_state = true;
			}	

		else if (cycle_time<=0 && motor_only_check3 == true)//three-over
			{
					state = DRYER_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[4];

					motor_only_check3 = false;
					motor_only_check4 = true;

						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;
						new_running_state = true;
			}	

		else if (cycle_time<=0 && motor_only_check4 == true)//four-over
			{
					state = DRYER_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[4];

					motor_only_check4 = false;
					motor_only_check5 = true;
					
						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;
						new_running_state = true;
			}	

		else if (cycle_time<=0 && motor_only_check5 == true)//five-over
			{
					state = DRYER_MOTOR_COIL_ONLY;
					double cycle_t = 1000 * (energy_needed - energy_used) / (motor_power + coil_power[0]) * 60 * 60;
					double interval = pulse_interval[6];

					motor_only_check5 = false;
					
						if (cycle_t > interval)
							cycle_time = interval;
						else
							cycle_time = cycle_t;	
						new_running_state = true;
			}	

		else if (pCircuit->pV->Mag()<stall_voltage)
			{
					state = DRYER_STALLED;
					state_time = 0;
			}
			
	break;

	case DRYER_STALLED:

		if (pCircuit->pV->Mag()>start_voltage)
		{
			state = DRYER_MOTOR_ONLY;
			state_time = cycle_time;
		}
		else if (state_time>trip_delay)
		{
			state = DRYER_TRIPPED;
			state_time = 0;
		}

	break;

	case DRYER_TRIPPED:

		if (state_time>reset_delay)
		{
			if (pCircuit->pV->Mag()>start_voltage)
				state = DRYER_MOTOR_ONLY;
			else
				state = DRYER_STALLED;
				state_time = 0;
		}

		break;
	}
	

	// accumulating units in the queue no matter what happens
	if (dryer_on == true)
	{
	enduse_queue += daily_dryer_demand * dt/24;
	}

	actual_dryer_demand = actual_dryer_demand + daily_dryer_demand;

	
	// now implement current state
	switch(state) {
	case DRYER_STOPPED: 
		
		motor_on_off = motor_coil_on_off = 0;

		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);

		dt = ((enduse_queue>=1) || (enduse_queue==0)) ? 0 : ((1-enduse_queue)*3600)/(enduse_queue*24); 				

		break;

	case DRYER_MOTOR_COIL_ONLY:

		motor_on_off = motor_coil_on_off = 1;
		cycle_time -= dt;
		// running in constant power mode with intermittent coil
		load.power.SetPowerFactor(motor_power/1000, load.power_factor);
		load.admittance = complex((coil_power[0])/1000,0,J); //assume pure resistance
		load.current = complex(0,0,J);

		dt = cycle_time;
		break;

	case DRYER_CONTROL_ONLY:

		if(true==new_running_state){
			
			new_running_state = false;

		}
		else{

			cycle_time -= dt;
		}

		// running in constant power mode with intermittent coil
		load.power = load.current = complex(0,0,J);
		load.admittance = complex(controls_power/1000,0,J);

		dt = cycle_time;
		break;

	case DRYER_STALLED:

		// running in constant impedance mode
		load.power = load.current = complex(0,0,J);
		load.admittance = complex(1)/stall_impedance;

		// time to trip
		dt = trip_delay;

		break;

	case DRYER_TRIPPED:

		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);
		
		// time to next expected state change
		dt = reset_delay; 

		break;


	case DRYER_MOTOR_ONLY:
		motor_on_off = 1;
		cycle_time -= dt;

		// running in constant power mode with intermittent coil
		load.power.SetPowerFactor(motor_power/1000, load.power_factor);
		load.admittance = complex(0,0,J); //assume pure resistance
		load.current = complex(0,0,J);

		dt = cycle_time;
		break;

	default:

		throw "unexpected motor state";
		/*	TROUBLESHOOT
			This is an error.  Please submit a bug report along with at the dryer
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


EXPORT TIMESTAMP sync_dryer(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	TIMESTAMP tret;
	dryer *my = OBJECTDATA(obj, dryer);
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
		gl_error("%s (dryer:%d) model zone exception (code %d) not caught", obj->name?obj->name:"(anonymous dryer)", obj->id, m);
	}
	catch (char *msg)
	{
		gl_error("%s (dryer:%d) %s", obj->name?obj->name:"(anonymous dryer)", obj->id, msg);
	}
	return TS_INVALID;
}

EXPORT int create_dryer(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(dryer::oclass);
	if (*obj!=NULL)
	{
		dryer *my = OBJECTDATA(*obj,dryer);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_dryer(OBJECT *obj)
{
	dryer *my = OBJECTDATA(obj,dryer);
	return my->init(obj->parent);
}

EXPORT int isa_dryer(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,dryer)->isa(classname);
	} else {
		return 0;
	}
}

//EXPORT TIMESTAMP sync_dryer(OBJECT *obj, TIMESTAMP t0)
//{
//	dryer *my = OBJECTDATA(obj, dryer);
//	TIMESTAMP t1 = my->sync(obj->clock, t0);
//	obj->clock = t0;
//	return t1;
//}

/**@}**/