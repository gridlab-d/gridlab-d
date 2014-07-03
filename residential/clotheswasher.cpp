/** $Id: clotheswasher.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file clotheswasher.cpp
	@addtogroup clotheswasher
	@ingroup residential

	The clotheswasher simulation is based on an hourly demand profile attached to the object.
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

#include "house_a.h"
#include "clotheswasher.h"

//////////////////////////////////////////////////////////////////////////
// clotheswasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* clotheswasher::oclass = NULL;
CLASS* clotheswasher::pclass = NULL;
clotheswasher *clotheswasher::defaults = NULL;

clotheswasher::clotheswasher(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"clotheswasher",sizeof(clotheswasher),PC_PRETOPDOWN|PC_BOTTOMUP|PC_AUTOLOCK);
		pclass = residential_enduse::oclass;
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"motor_power[kW]",PADDR(shape.params.analog.power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"queue[unit]",PADDR(enduse_queue), PT_DESCRIPTION, "the total laundry accumulated",
			PT_double,"demand[unit/day]",PADDR(enduse_demand), PT_DESCRIPTION, "the amount of laundry accumulating daily",			
			PT_complex,"energy_meter[kWh]",PADDR(load.energy),
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_double,"clothesWasherPower",PADDR(clothesWasherPower),		
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),
			PT_double ,"Is_on",PADDR(Is_on),

			PT_double ,"normal_perc",PADDR(normal_perc),
			PT_double ,"perm_press_perc",PADDR(perm_press_perc),

			PT_double,"NORMAL_PREWASH_POWER",PADDR(NORMAL_PREWASH_POWER),
			PT_double,"NORMAL_WASH_POWER",PADDR(NORMAL_WASH_POWER),
			PT_double,"NORMAL_SPIN_LOW_POWER",PADDR(NORMAL_SPIN_LOW_POWER),
			PT_double,"NORMAL_SPIN_MEDIUM_POWER",PADDR(NORMAL_SPIN_MEDIUM_POWER),
			PT_double,"NORMAL_SPIN_HIGH_POWER",PADDR(NORMAL_SPIN_HIGH_POWER),
			PT_double,"NORMAL_SMALLWASH_POWER",PADDR(NORMAL_SMALLWASH_POWER),

			PT_double,"NORMAL_PREWASH_ENERGY",PADDR(NORMAL_PREWASH_ENERGY),
			PT_double,"NORMAL_WASH_ENERGY",PADDR(NORMAL_WASH_ENERGY),
			PT_double,"NORMAL_SPIN_LOW_ENERGY",PADDR(NORMAL_SPIN_LOW_ENERGY),
			PT_double,"NORMAL_SPIN_MEDIUM_ENERGY",PADDR(NORMAL_SPIN_MEDIUM_ENERGY),
			PT_double,"NORMAL_SPIN_HIGH_ENERGY",PADDR(NORMAL_SPIN_HIGH_ENERGY),
			PT_double,"NORMAL_SMALLWASH_ENERGY",PADDR(NORMAL_SMALLWASH_ENERGY),	

			PT_double,"PERMPRESS_PREWASH_POWER",PADDR(PERMPRESS_PREWASH_POWER),
			PT_double,"PERMPRESS_WASH_POWER",PADDR(PERMPRESS_WASH_POWER),
			PT_double,"PERMPRESS_SPIN_LOW_POWER",PADDR(PERMPRESS_SPIN_LOW_POWER),
			PT_double,"PERMPRESS_SPIN_MEDIUM_POWER",PADDR(PERMPRESS_SPIN_MEDIUM_POWER),
			PT_double,"PERMPRESS_SPIN_HIGH_POWER",PADDR(PERMPRESS_SPIN_HIGH_POWER),
			PT_double,"PERMPRESS_SMALLWASH_POWER",PADDR(PERMPRESS_SMALLWASH_POWER),

			PT_double,"PERMPRESS_PREWASH_ENERGY",PADDR(PERMPRESS_PREWASH_ENERGY),
			PT_double,"PERMPRESS_WASH_ENERGY",PADDR(PERMPRESS_WASH_ENERGY),
			PT_double,"PERMPRESS_SPIN_LOW_ENERGY",PADDR(PERMPRESS_SPIN_LOW_ENERGY),
			PT_double,"PERMPRESS_SPIN_MEDIUM_ENERGY",PADDR(PERMPRESS_SPIN_MEDIUM_ENERGY),
			PT_double,"PERMPRESS_SPIN_HIGH_ENERGY",PADDR(PERMPRESS_SPIN_HIGH_ENERGY),
			PT_double,"PERMPRESS_SMALLWASH_ENERGY",PADDR(PERMPRESS_SMALLWASH_ENERGY),

			PT_double,"GENTLE_PREWASH_POWER",PADDR(GENTLE_PREWASH_POWER),
			PT_double,"GENTLE_WASH_POWER",PADDR(GENTLE_WASH_POWER),
			PT_double,"GENTLE_SPIN_LOW_POWER",PADDR(GENTLE_SPIN_LOW_POWER),
			PT_double,"GENTLE_SPIN_MEDIUM_POWER",PADDR(GENTLE_SPIN_MEDIUM_POWER),
			PT_double,"GENTLE_SPIN_HIGH_POWER",PADDR(GENTLE_SPIN_HIGH_POWER),
			PT_double,"GENTLE_SMALLWASH_POWER",PADDR(GENTLE_SMALLWASH_POWER),

			PT_double,"GENTLE_PREWASH_ENERGY",PADDR(GENTLE_PREWASH_ENERGY),
			PT_double,"GENTLE_WASH_ENERGY",PADDR(GENTLE_WASH_ENERGY),
			PT_double,"GENTLE_SPIN_LOW_ENERGY",PADDR(GENTLE_SPIN_LOW_ENERGY),
			PT_double,"GENTLE_SPIN_MEDIUM_ENERGY",PADDR(GENTLE_SPIN_MEDIUM_ENERGY),
			PT_double,"GENTLE_SPIN_HIGH_ENERGY",PADDR(GENTLE_SPIN_HIGH_ENERGY),
			PT_double,"GENTLE_SMALLWASH_ENERGY",PADDR(GENTLE_SMALLWASH_ENERGY),	

			PT_double,"queue_min[unit]",PADDR(queue_min),
			PT_double,"queue_max[unit]",PADDR(queue_max),

			PT_double,"clotheswasher_run_prob",PADDR(clotheswasher_run_prob),
			

			PT_enumeration,"state", PADDR(state),
				PT_KEYWORD,"STOPPED",(enumeration)STOPPED,
				PT_KEYWORD,"PREWASH",(enumeration)PREWASH,
				PT_KEYWORD,"WASH",(enumeration)WASH,
				PT_KEYWORD,"SPIN1",(enumeration)SPIN1,
				PT_KEYWORD,"SPIN2",(enumeration)SPIN2,
				PT_KEYWORD,"SPIN3",(enumeration)SPIN3,
				PT_KEYWORD,"SPIN4",(enumeration)SPIN4,		

			PT_enumeration,"spin_mode", PADDR(spin_mode),
				PT_KEYWORD,"SPIN_LOW",(enumeration)SPIN_LOW,
				PT_KEYWORD,"SPIN_MEDIUM",(enumeration)SPIN_MEDIUM,
				PT_KEYWORD,"SPIN_HIGH",(enumeration)SPIN_HIGH,
				PT_KEYWORD,"SPIN_WASH",(enumeration)SPIN_WASH,
				PT_KEYWORD,"SMALLWASH",(enumeration)SMALLWASH,

			PT_enumeration,"wash_mode", PADDR(wash_mode),
				PT_KEYWORD,"NORMAL",(enumeration)NORMAL,
				PT_KEYWORD,"PERM_PRESS",(enumeration)PERM_PRESS,
				PT_KEYWORD,"GENTLE",(enumeration)GENTLE,		

			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

clotheswasher::~clotheswasher()
{
}

int clotheswasher::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;

	load.power = load.admittance = load.current = load.total = complex(0,0,J);
	load.voltage_factor = 1.0;
	load.power_factor = 0.95;
	load.power_fraction = 1.0;

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);
	/* TROUBLESHOOT
		The clothes washer explicit model has some serious issues and should be considered for complete
		removal.  It is highly suggested that this model NOT be used.
	*/

	return res;
}

int clotheswasher::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("clotheswasher::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	hdr->flags |= OF_SKIPSAFE;
	
	// default properties
	if (shape.params.analog.power==0) shape.params.analog.power = gl_random_uniform(&hdr->rng_state,0.100,0.750);		// clotheswasher size [W]
	if (load.heatgain_fraction==0) load.heatgain_fraction = 0.5; 
	if (load.power_factor==0) load.power_factor = 0.95;

	if(shape.params.analog.power < 0.1){
		gl_error("clotheswasher motor is undersized, using 500W motor");
		shape.params.analog.power = 0.5;
	}

	int res = residential_enduse::init(parent);

	Is_on = 0;
	
	if(NORMAL_PREWASH_ENERGY == 0) NORMAL_PREWASH_ENERGY = 12*20*60;
	if(NORMAL_WASH_ENERGY == 0) NORMAL_WASH_ENERGY = 4*40*60;
	if(NORMAL_SMALLWASH_ENERGY == 0) NORMAL_SMALLWASH_ENERGY = 2*25*60;
	if(NORMAL_SPIN_LOW_ENERGY == 0) NORMAL_SPIN_LOW_ENERGY = 2*60*60;
	if(NORMAL_SPIN_MEDIUM_ENERGY == 0) NORMAL_SPIN_MEDIUM_ENERGY = 2*150*60;
	if(NORMAL_SPIN_HIGH_ENERGY == 0) NORMAL_SPIN_HIGH_ENERGY = 2*220*60;

	if(NORMAL_PREWASH_POWER == 0) NORMAL_PREWASH_POWER = 20;
	if(NORMAL_WASH_POWER == 0) NORMAL_WASH_POWER = 40;
	if(NORMAL_SMALLWASH_POWER == 0) NORMAL_SMALLWASH_POWER = 25;
	if(NORMAL_SPIN_LOW_POWER == 0) NORMAL_SPIN_LOW_POWER = 60;
	if(NORMAL_SPIN_MEDIUM_POWER == 0) NORMAL_SPIN_MEDIUM_POWER = 150;
	if(NORMAL_SPIN_HIGH_POWER == 0) NORMAL_SPIN_HIGH_POWER = 220;	

	if(PERMPRESS_PREWASH_ENERGY == 0) PERMPRESS_PREWASH_ENERGY = 12*20*60;
	if(PERMPRESS_WASH_ENERGY == 0) PERMPRESS_WASH_ENERGY = 4*40*60;
	if(PERMPRESS_SMALLWASH_ENERGY == 0) PERMPRESS_SMALLWASH_ENERGY = 2*25*60;
	if(PERMPRESS_SPIN_LOW_ENERGY == 0) PERMPRESS_SPIN_LOW_ENERGY = 2*60*60;
	if(PERMPRESS_SPIN_MEDIUM_ENERGY == 0) PERMPRESS_SPIN_MEDIUM_ENERGY = 2*150*60;
	if(PERMPRESS_SPIN_HIGH_ENERGY == 0) PERMPRESS_SPIN_HIGH_ENERGY = 2*220*60;

	if(PERMPRESS_PREWASH_POWER == 0) PERMPRESS_PREWASH_POWER = 20;
	if(PERMPRESS_WASH_POWER == 0) PERMPRESS_WASH_POWER = 40;
	if(PERMPRESS_SMALLWASH_POWER == 0) PERMPRESS_SMALLWASH_POWER = 25;
	if(PERMPRESS_SPIN_LOW_POWER == 0) PERMPRESS_SPIN_LOW_POWER = 60;
	if(PERMPRESS_SPIN_MEDIUM_POWER == 0) PERMPRESS_SPIN_MEDIUM_POWER = 150;
	if(PERMPRESS_SPIN_HIGH_POWER == 0) PERMPRESS_SPIN_HIGH_POWER = 220;	

	if(GENTLE_PREWASH_ENERGY == 0) GENTLE_PREWASH_ENERGY = 12*20*60;
	if(GENTLE_WASH_ENERGY == 0) GENTLE_WASH_ENERGY = 4*40*60;
	if(GENTLE_SMALLWASH_ENERGY == 0) GENTLE_SMALLWASH_ENERGY = 2*25*60;
	if(GENTLE_SPIN_LOW_ENERGY == 0) GENTLE_SPIN_LOW_ENERGY = 2*60*60;
	if(GENTLE_SPIN_MEDIUM_ENERGY == 0) GENTLE_SPIN_MEDIUM_ENERGY = 2*150*60;
	if(GENTLE_SPIN_HIGH_ENERGY == 0) GENTLE_SPIN_HIGH_ENERGY = 2*220*60;

	if(GENTLE_PREWASH_POWER == 0) GENTLE_PREWASH_POWER = 20;
	if(GENTLE_WASH_POWER == 0) GENTLE_WASH_POWER = 40;
	if(GENTLE_SMALLWASH_POWER == 0) GENTLE_SMALLWASH_POWER = 25;
	if(GENTLE_SPIN_LOW_POWER == 0) GENTLE_SPIN_LOW_POWER = 60;
	if(GENTLE_SPIN_MEDIUM_POWER == 0) GENTLE_SPIN_MEDIUM_POWER = 150;
	if(GENTLE_SPIN_HIGH_POWER == 0) GENTLE_SPIN_HIGH_POWER = 220;	
	
	if(normal_perc == 0) normal_perc = 0.5;	
	if(perm_press_perc == 0) perm_press_perc = 0.8;	
	
	return res;
}

int clotheswasher::isa(char *classname)
{
	return (strcmp(classname,"clotheswasher")==0 || residential_enduse::isa(classname));
}

TIMESTAMP clotheswasher::presync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

TIMESTAMP clotheswasher::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// compute the seconds in this time step
	double dt = 0.0;
	TIMESTAMP t2 = residential_enduse::sync(t0, t1);

	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor
	
	dt = gl_toseconds(t0>0?t1-t0:0);

	if(t0==t1)
	{
		starttime = true;
	}
	else{
		starttime = false;
	}
			// determine the delta time until the next state change
	dt = update_state(dt);

	if(dt >= 7200){
		return TS_NEVER;
	}
	else{

		if(t2 > (TIMESTAMP)(dt*TS_SECOND+t1))
			return dt>0?-(TIMESTAMP)(dt*TS_SECOND+t1):TS_NEVER; 
		else		
			return t2;
	}
	
}

double clotheswasher::update_state(double dt)
{

	OBJECT *hdr = OBJECTHDR(this);

	// accumulating units in the queue no matter what happens
	enduse_queue += enduse_demand * dt/3600/24;

	switch(state) {
	case STOPPED:

		if (enduse_queue>1)
		
			clotheswasher_run_prob = double(gl_random_uniform(&hdr->rng_state,queue_min,queue_max));
		
		if (enduse_queue > 1 && (clotheswasher_run_prob > enduse_queue))
			{		
			int temp_rand =  gl_random_uniform(&hdr->rng_state,0,1); 
			
			if(temp_rand <= normal_perc){
				wash_mode = NORMAL;
			}
			else if(temp_rand <= perm_press_perc){
				wash_mode = PERM_PRESS;
			}
			else{
				wash_mode = GENTLE;
			}
			
			switch(wash_mode){
				case NORMAL:
					cycle_time = ceil(NORMAL_PREWASH_ENERGY/NORMAL_PREWASH_POWER);
					clothesWasherPower = NORMAL_PREWASH_POWER;	
				break;
				case PERM_PRESS:
					cycle_time = ceil(PERMPRESS_PREWASH_ENERGY/PERMPRESS_PREWASH_POWER);
					clothesWasherPower = PERMPRESS_PREWASH_POWER;	
				break;
				case GENTLE:	
					cycle_time = ceil(GENTLE_PREWASH_ENERGY/GENTLE_PREWASH_POWER);
					clothesWasherPower = GENTLE_PREWASH_POWER;	
				break;
			}

			state = PREWASH;
			enduse_queue--;			
			Is_on = 1;
			new_running_state = true;
		}
		break;		
	case PREWASH:
		if (cycle_time<=0)
		{
			state = WASH;
			
			switch(wash_mode){
				case NORMAL:
					cycle_time = ceil(NORMAL_WASH_ENERGY/NORMAL_WASH_POWER);
					clothesWasherPower = NORMAL_WASH_POWER;	
				break;
				case PERM_PRESS:
					cycle_time = ceil(PERMPRESS_WASH_ENERGY/PERMPRESS_WASH_POWER);
					clothesWasherPower = PERMPRESS_WASH_POWER;	
				break;
				case GENTLE:	
					cycle_time = ceil(GENTLE_WASH_ENERGY/GENTLE_WASH_POWER);
					clothesWasherPower = GENTLE_WASH_POWER;	
				break;
			}

			new_running_state = true;
		}
		
		break;		
	case WASH:
		if (cycle_time<=0)
		{
			state = SPIN1;
			spin_mode = SPIN_LOW;
			
			switch(wash_mode){
				case NORMAL:
					cycle_time = ceil(NORMAL_SPIN_LOW_ENERGY/NORMAL_SPIN_LOW_POWER);
					clothesWasherPower = NORMAL_SPIN_LOW_POWER;	
				break;
				case PERM_PRESS:
					cycle_time = ceil(PERMPRESS_SPIN_LOW_ENERGY/PERMPRESS_SPIN_LOW_POWER);
					clothesWasherPower = PERMPRESS_SPIN_LOW_POWER;	
				break;
				case GENTLE:	
					cycle_time = ceil(GENTLE_SPIN_LOW_ENERGY/GENTLE_SPIN_LOW_POWER);
					clothesWasherPower = GENTLE_SPIN_LOW_POWER;	
				break;
			}
			
			new_running_state = true;
			
		}
		
		break;		
	case SPIN1:
		if (cycle_time<=0)
		{
			switch(spin_mode){
				case SPIN_LOW:
					spin_mode = SPIN_MEDIUM;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_MEDIUM_ENERGY/NORMAL_SPIN_MEDIUM_POWER);
							clothesWasherPower = NORMAL_SPIN_MEDIUM_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_MEDIUM_ENERGY/PERMPRESS_SPIN_MEDIUM_POWER);
							clothesWasherPower = PERMPRESS_SPIN_MEDIUM_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_MEDIUM_ENERGY/GENTLE_SPIN_MEDIUM_POWER);
							clothesWasherPower = GENTLE_SPIN_MEDIUM_POWER;	
						break;
					}

				break;
				case SPIN_MEDIUM:
					spin_mode = SPIN_HIGH;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_HIGH_ENERGY/NORMAL_SPIN_HIGH_POWER);
							clothesWasherPower = NORMAL_SPIN_HIGH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_HIGH_ENERGY/PERMPRESS_SPIN_HIGH_POWER);
							clothesWasherPower = PERMPRESS_SPIN_HIGH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_HIGH_ENERGY/GENTLE_SPIN_HIGH_POWER);
							clothesWasherPower = GENTLE_SPIN_HIGH_POWER;	
						break;
					}

				break;
				case SPIN_HIGH:
					
					spin_mode = SPIN_WASH;

					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_WASH_ENERGY/NORMAL_WASH_POWER);
							clothesWasherPower = NORMAL_WASH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_WASH_ENERGY/PERMPRESS_WASH_POWER);
							clothesWasherPower = PERMPRESS_WASH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_WASH_ENERGY/GENTLE_WASH_POWER);
							clothesWasherPower = GENTLE_WASH_POWER;	
						break;
					}
				case SPIN_WASH:
					
					state = SPIN2;
					spin_mode = SPIN_LOW;

					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_LOW_ENERGY/NORMAL_SPIN_LOW_POWER);
							clothesWasherPower = NORMAL_SPIN_LOW_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_LOW_ENERGY/PERMPRESS_SPIN_LOW_POWER);
							clothesWasherPower = PERMPRESS_SPIN_LOW_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_LOW_ENERGY/GENTLE_SPIN_LOW_POWER);
							clothesWasherPower = GENTLE_SPIN_LOW_POWER;	
						break;
					}

				break;
				
			}
			
			new_running_state = true;
		}		
		break;	
	case SPIN2:
		if (cycle_time<=0)
		{
			switch(spin_mode){
				case SPIN_LOW:
					spin_mode = SPIN_MEDIUM;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_MEDIUM_ENERGY/NORMAL_SPIN_MEDIUM_POWER);
							clothesWasherPower = NORMAL_SPIN_MEDIUM_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_MEDIUM_ENERGY/PERMPRESS_SPIN_MEDIUM_POWER);
							clothesWasherPower = PERMPRESS_SPIN_MEDIUM_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_MEDIUM_ENERGY/GENTLE_SPIN_MEDIUM_POWER);
							clothesWasherPower = GENTLE_SPIN_MEDIUM_POWER;	
						break;
					}

				break;
				case SPIN_MEDIUM:
					spin_mode = SPIN_HIGH;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_HIGH_ENERGY/NORMAL_SPIN_HIGH_POWER);
							clothesWasherPower = NORMAL_SPIN_HIGH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_HIGH_ENERGY/PERMPRESS_SPIN_HIGH_POWER);
							clothesWasherPower = PERMPRESS_SPIN_HIGH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_HIGH_ENERGY/GENTLE_SPIN_HIGH_POWER);
							clothesWasherPower = GENTLE_SPIN_HIGH_POWER;	
						break;
					}

				break;
				case SPIN_HIGH:
					
					spin_mode = SPIN_WASH;

					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_WASH_ENERGY/NORMAL_WASH_POWER);
							clothesWasherPower = NORMAL_WASH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_WASH_ENERGY/PERMPRESS_WASH_POWER);
							clothesWasherPower = PERMPRESS_WASH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_WASH_ENERGY/GENTLE_WASH_POWER);
							clothesWasherPower = GENTLE_WASH_POWER;	
						break;
					}
				case SPIN_WASH:
					
					state = SPIN3;
					spin_mode = SPIN_LOW;

					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_LOW_ENERGY/NORMAL_SPIN_LOW_POWER);
							clothesWasherPower = NORMAL_SPIN_LOW_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_LOW_ENERGY/PERMPRESS_SPIN_LOW_POWER);
							clothesWasherPower = PERMPRESS_SPIN_LOW_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_LOW_ENERGY/GENTLE_SPIN_LOW_POWER);
							clothesWasherPower = GENTLE_SPIN_LOW_POWER;	
						break;
					}

				break;
				
			}
			
			new_running_state = true;
		}		
		break;
	case SPIN3:
		if (cycle_time<=0)
		{
			switch(spin_mode){
				case SPIN_LOW:
					spin_mode = SPIN_MEDIUM;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_MEDIUM_ENERGY/NORMAL_SPIN_MEDIUM_POWER);
							clothesWasherPower = NORMAL_SPIN_MEDIUM_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_MEDIUM_ENERGY/PERMPRESS_SPIN_MEDIUM_POWER);
							clothesWasherPower = PERMPRESS_SPIN_MEDIUM_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_MEDIUM_ENERGY/GENTLE_SPIN_MEDIUM_POWER);
							clothesWasherPower = GENTLE_SPIN_MEDIUM_POWER;	
						break;
					}

				break;
				case SPIN_MEDIUM:
					spin_mode = SMALLWASH;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SMALLWASH_ENERGY/NORMAL_SMALLWASH_POWER);
							clothesWasherPower = NORMAL_SMALLWASH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SMALLWASH_ENERGY/PERMPRESS_SMALLWASH_POWER);
							clothesWasherPower = PERMPRESS_SMALLWASH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SMALLWASH_ENERGY/GENTLE_SMALLWASH_POWER);
							clothesWasherPower = GENTLE_SMALLWASH_POWER;	
						break;
					}

				break;
				case SMALLWASH:
					
					state = SPIN4;
					spin_mode = SPIN_LOW;

					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_LOW_ENERGY/NORMAL_SPIN_LOW_POWER);
							clothesWasherPower = NORMAL_SPIN_LOW_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_LOW_ENERGY/PERMPRESS_SPIN_LOW_POWER);
							clothesWasherPower = PERMPRESS_SPIN_LOW_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_LOW_ENERGY/GENTLE_SPIN_LOW_POWER);
							clothesWasherPower = GENTLE_SPIN_LOW_POWER;	
						break;
					}

				break;
				
			}
		}		
		break;
	case SPIN4:
		if (cycle_time<=0)
		{
			switch(spin_mode){
				case SPIN_LOW:
					spin_mode = SPIN_MEDIUM;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SPIN_MEDIUM_ENERGY/NORMAL_SPIN_MEDIUM_POWER);
							clothesWasherPower = NORMAL_SPIN_MEDIUM_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SPIN_MEDIUM_ENERGY/PERMPRESS_SPIN_MEDIUM_POWER);
							clothesWasherPower = PERMPRESS_SPIN_MEDIUM_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SPIN_MEDIUM_ENERGY/GENTLE_SPIN_MEDIUM_POWER);
							clothesWasherPower = GENTLE_SPIN_MEDIUM_POWER;	
						break;
					}

				break;
				case SPIN_MEDIUM:
					spin_mode = SPIN_HIGH;
			
					//Longer high cycle, hence multiplication by 3
					switch(wash_mode){
						case NORMAL:
							
							cycle_time = ceil((NORMAL_SPIN_HIGH_ENERGY*3)/NORMAL_SPIN_HIGH_POWER);
							clothesWasherPower = NORMAL_SPIN_HIGH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil((PERMPRESS_SPIN_HIGH_ENERGY*3)/PERMPRESS_SPIN_HIGH_POWER);
							clothesWasherPower = PERMPRESS_SPIN_HIGH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil((GENTLE_SPIN_HIGH_ENERGY*3)/GENTLE_SPIN_HIGH_POWER);
							clothesWasherPower = GENTLE_SPIN_HIGH_POWER;	
						break;
					}

				break;
				case SPIN_HIGH:
					
					spin_mode = SMALLWASH;
			
					switch(wash_mode){
						case NORMAL:
							cycle_time = ceil(NORMAL_SMALLWASH_ENERGY/NORMAL_SMALLWASH_POWER);
							clothesWasherPower = NORMAL_SMALLWASH_POWER;	
						break;
						case PERM_PRESS:
							cycle_time = ceil(PERMPRESS_SMALLWASH_ENERGY/PERMPRESS_SMALLWASH_POWER);
							clothesWasherPower = PERMPRESS_SMALLWASH_POWER;	
						break;
						case GENTLE:	
							cycle_time = ceil(GENTLE_SMALLWASH_ENERGY/GENTLE_SMALLWASH_POWER);
							clothesWasherPower = GENTLE_SMALLWASH_POWER;	
						break;
					}
				break;
				case SMALLWASH:
					
					state = STOPPED;
					cycle_time = 0;
					clothesWasherPower = 0;
			
					Is_on = 0;

				break;
				
				}

				new_running_state = true;
		}		
		break;
	
	}
	
	if(state==STOPPED){
		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);		
		// time to next expected state change
		//dt = (enduse_demand<=0) ? -1 : 	dt = 3600/enduse_demand; 
		if(0==enduse_demand){
			
			if(true==starttime){
				dt=0;
			}
			else{
				dt = 3600;
			}

		}
		else{
			
			dt = (enduse_queue>=1) ? 0 : ((1-enduse_queue)*3600)/(enduse_demand*24); 	
			
		}

	}
	else{ 	

		if(new_running_state == true){
			new_running_state = false;	
			cycle_time = cycle_time-1;
		}
		else{
			cycle_time -=dt;			
		}		
		
		load.power = complex(clothesWasherPower/1000,0,J);		
		load.current = 0;
		load.admittance =0;
		
		dt = cycle_time; 	


	}

	// compute the total electrical load
	load.total = load.power;

	if (dt > 0 && dt < 1)
		dt = 1;

	return dt;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_clotheswasher(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(clotheswasher::oclass);
	if (*obj!=NULL)
	{
		clotheswasher *my = OBJECTDATA(*obj,clotheswasher);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int init_clotheswasher(OBJECT *obj)
{
	clotheswasher *my = OBJECTDATA(obj,clotheswasher);
	return my->init(obj->parent);
}

EXPORT int isa_clotheswasher(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,clotheswasher)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_clotheswasher(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	clotheswasher *my = OBJECTDATA(obj, clotheswasher);
	if (obj->clock <= ROUNDOFF)
		obj->clock = t0;  //set the object clock if it has not been set yet
	try {
		TIMESTAMP t1 = TS_NEVER;
		switch (pass) {
		case PC_PRETOPDOWN:
			return my->presync(obj->clock, t0);
		case PC_BOTTOMUP:
			t1 = my->sync(obj->clock, t0);
			obj->clock = t0;
			return t1;
		default:
			throw "invalid pass request";
		}
	}
	catch (int m)
	{
		gl_error("%s (clotheswasher:%d) model zone exception (code %d) not caught", obj->name?obj->name:"(anonymous waterheater)", obj->id, m);
	}
	catch (char *msg)
	{
		gl_error("%s (clotheswasher:%d) %s", obj->name?obj->name:"(anonymous clotheswasher)", obj->id, msg);
	}
	return TS_INVALID;}

/**@}**/
