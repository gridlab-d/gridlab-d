/** $Id: microwave.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file microwave.cpp
	@addtogroup microwave
	@ingroup residential

	The microwave simulation is based on a demand profile attached to the object.
	The internal heat gain is calculated using a specified fraction of installed power.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "house_a.h"
#include "microwave.h"

//////////////////////////////////////////////////////////////////////////
// microwave CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* microwave::oclass = NULL;
CLASS* microwave::pclass = NULL;

microwave::microwave(MODULE *module) : residential_enduse(module)
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"microwave",sizeof(microwave),PC_BOTTOMUP|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class microwave";
		else
			oclass->trl = TRL_DEMONSTRATED;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"installed_power[kW]",PADDR(shape.params.analog.power),PT_DESCRIPTION,"rated microwave power level",
			PT_double,"standby_power[kW]",PADDR(standby_power),PT_DESCRIPTION,"standby microwave power draw (unshaped only)",
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_enumeration,"state",PADDR(state),PT_DESCRIPTION,"on/off state of the microwave",
				PT_KEYWORD,"OFF",OFF,
				PT_KEYWORD,"ON",ON,
			
			PT_double,"cycle_length[s]",PADDR(cycle_time),PT_DESCRIPTION,"length of the combined on/off cycle between uses",
			PT_double,"runtime[s]",PADDR(runtime),PT_DESCRIPTION,"",
			PT_double,"state_time[s]",PADDR(state_time),PT_DESCRIPTION,"",
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

microwave::~microwave()
{
}

int microwave::create() 
{
	int res = residential_enduse::create();

	// name of enduse
	load.name = oclass->name;
	load.power = load.admittance = load.current = load.total = complex(0,0,J);

	load.heatgain_fraction = 0.25;
	load.power_factor = 0.95;

	standby_power = 0.01;
	shape.load = gl_random_uniform(RNGSTATE,0, 0.1);  // assuming a default maximum 10% of the sync time 

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);

	return res;
}

/* basic checks on unshaped microwaves.  on failure, don't play games, just throw exceptions. */
void microwave::init_noshape(){
	if(shape.params.analog.power < 0){
		GL_THROW("microwave power must be positive (read as %f)", shape.params.analog.power);
	} else if (shape.params.analog.power > 4.000){
		GL_THROW("microwave power can not exceed 4 kW (and most don't exceed 2 kW)");
	}
	if(shape.params.analog.power < 0.700){
		gl_warning("microwave installed power is smaller than traditional microwave ovens");
	} else if(shape.params.analog.power > 1.800){
		gl_warning("microwave installed power is greater than traditional microwave ovens");
	}
	if(standby_power < 0){
		gl_warning("negative standby power, resetting to 1%% of installed power");
		standby_power = shape.params.analog.power * 0.01;
	} else if(standby_power > shape.params.analog.power){
		gl_warning("standby power exceeds installed power, resetting to 1%% of installed power");
		standby_power = shape.params.analog.power * 0.01;
	}
	if(cycle_time < 0){
		GL_THROW("negative cycle_length is an invalid value");
	}
	if(cycle_time > 14400){
		gl_warning("cycle_length is abnormally long and may give unusual results");
	}
}

int microwave::init(OBJECT *parent)
{
	if(parent != NULL){
		if((parent->flags & OF_INIT) != OF_INIT){
			char objname[256];
			gl_verbose("microwave::init(): deferring initialization on %s", gl_name(parent, objname, 255));
			return 2; // defer
		}
	}
	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (load.voltage_factor==0) load.voltage_factor = 1.0;

	if(shape.type == MT_UNKNOWN){
		init_noshape();
		gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());
		// initial demand
		update_state(0.0);
	} else if(shape.type == MT_ANALOG){
		if(1){
			;
		}
	} else if(shape.type == MT_PULSED){
		if(1){
			;
		}
	} else if(shape.type == MT_MODULATED){
		if(1){
			;
		}
	} else if(shape.type == MT_QUEUED){
		gl_error("queued loadshapes not supported ~ will attempt to run as an unshaped load");
		shape.type = MT_UNKNOWN;
		init_noshape();
		// initial demand
		update_state(0.0);
	} else {
		gl_error("unrecognized loadshape");
		return 0;
	}
	load.total = load.power = standby_power;
	
	// waiting this long to initialize the parent class is normal
	return residential_enduse::init(parent);
}

int microwave::isa(char *classname)
{
	return (strcmp(classname,"microwave")==0 || residential_enduse::isa(classname));
}

// periodically activates for the tail demand % of a cycle_time period.  Has a random offset to prevent
//	lock-step behavior across uniform devices
// start ....... on .. off
TIMESTAMP microwave::update_state_cycle(TIMESTAMP t0, TIMESTAMP t1){
	double ti0 = (double)t0, ti1 = (double)t1;

	if(shape.load == 0){
		state = OFF;
		cycle_start = 0;
		return TS_NEVER;
	}

	if(shape.load == 1){
		state = ON;
		cycle_start = 0;
		return TS_NEVER;
	}

	if(cycle_start == 0){
		double off = gl_random_uniform(RNGSTATE,0, this->cycle_time);
		cycle_start = (TIMESTAMP)(ti1 + off);
		cycle_on = (TIMESTAMP)((1 - shape.load) * cycle_time) + cycle_start;
		cycle_off = (TIMESTAMP)cycle_time + cycle_start;
		state = OFF;
		return (TIMESTAMP)cycle_on;
	}
	
	if(t0 == cycle_on){
		state = ON;
	}
	if(t0 == cycle_off){
		state = OFF;
		cycle_start = cycle_off;
	}
	if(t0 == cycle_start){
		cycle_on = (TIMESTAMP)((1 - shape.load) * cycle_time) + cycle_start;
		state = OFF;
		cycle_off = (TIMESTAMP)cycle_time + cycle_start;
	}

	if(state == ON)
		return (TIMESTAMP)cycle_off;
	if(state == OFF)
		return (TIMESTAMP)cycle_on;
	return TS_NEVER; // from ambiguous state
}

double microwave::update_state(double dt)
{
	// run times (used for gl_random_sample()) - DPC: this is an educated guess, the true PDF needs to be researched
	static double rt[] = {30,30,30,30,30,30,30,30,30,30,60,60,60,60,90,90,120,150,180,450,600};
	static double sumrt = 2520; // sum(pdf) -- you do the math
	static double avgrt = sumrt/sizeof(rt);

	if(shape.load < 0.0){
		gl_error("microwave demand less than 0, reseting to zero");
		shape.load = 0.0;
	}
	if(shape.load > 1.0){
		gl_error("microwave demand greater than 1, reseting to one");
		shape.load = 1.0;
	}
	switch (state) {
	case OFF:
		// new OFF state or demand changed
		if (state_time == 0 || prev_demand != shape.load) 
		{
			if(shape.load != 0.0){
				runtime = avgrt * (1 - shape.load) / shape.load;
			} 
			else {
				runtime = 0.0;
				return 0; /* don't run the microwave */
			}
			prev_demand = shape.load;
			state_time = 0; // important that state time be reset to prevent increase in demand from causing immediate state change
		}

		// time for state change
		if (state_time>runtime)
		{
			state = ON;
			runtime = gl_random_sampled(RNGSTATE,sizeof(rt)/sizeof(rt[0]),rt);
			state_time = 0;
		}
		else
			state_time += dt;
		break;
	case ON:
		// power outage or runtime expired
		runtime = floor(runtime);
		if (pCircuit->pV->Mag() < 0.25 || state_time>runtime)
		{
			state = OFF;
			state_time = 0;
		}
		else
			state_time += dt;
		break;
	default:
		throw "unknown microwave state";
		/*	TROUBLESHOOT
			The microwave is neither on nor off.  Please initialize the "state" variable
			to either "ON" or "OFF".
		*/
	}

	return runtime;
}

TIMESTAMP microwave::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	TIMESTAMP ct = 0;
	double dt = 0;
	double val = 0.0;
	TIMESTAMP t2 = TS_NEVER;

	if (t0 <= 0)
		return TS_NEVER;
	
	if (pCircuit!=NULL)
		load.voltage_factor = pCircuit->pV->Mag() / 120; // update voltage factor

	t2 = residential_enduse::sync(t0,t1);

	if(shape.type == MT_UNKNOWN){
		if(cycle_time > 0){
			ct = update_state_cycle(t0, t1);
		} else {
			dt = update_state(gl_toseconds(t1-t0));
		}
		load.power.SetPowerFactor( (state==ON ? shape.params.analog.power : standby_power), load.power_factor);
	}

	gl_enduse_sync(&(residential_enduse::load),t1);

	if(shape.type == MT_UNKNOWN){
		if(cycle_time == 0)
			return dt>0?-(TIMESTAMP)(t1 + dt*TS_SECOND) : TS_NEVER; // negative time means soft transition
		else
			return ct == TS_NEVER ? TS_NEVER : -ct;
	} else {
		return t2;
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_microwave(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(microwave::oclass);
		if (*obj!=NULL)
		{
			microwave *my = OBJECTDATA(*obj,microwave);;
			gl_set_parent(*obj,parent);
			my->create();
			return 1;
		}
		else 
			return 0;
	}
	CREATE_CATCHALL(microwave);

}

EXPORT int init_microwave(OBJECT *obj)
{
	try {
		microwave *my = OBJECTDATA(obj,microwave);
		return my->init(obj->parent);
	}
	INIT_CATCHALL(microwave);
}

EXPORT int isa_microwave(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,microwave)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_microwave(OBJECT *obj, TIMESTAMP t0)
{
	try {
		microwave *my = OBJECTDATA(obj, microwave);
		TIMESTAMP t2 = my->sync(obj->clock, t0);
		obj->clock = t0;
		return t2;
	}
	SYNC_CATCHALL(microwave);
}

/**@}**/
