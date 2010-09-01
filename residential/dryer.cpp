/** $Id: dryer.cpp,v 1.13 2008/02/13 02:22:35 d3j168 Exp $
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

#include "house_a.h"
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
		// register the class definition
		oclass = gl_register_class(module,"dryer",sizeof(dryer),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_INHERIT, "residential_enduse",
			PT_double,"motor_power[W]",PADDR(motor_power),
			PT_double,"coil_power[W]",PADDR(coil_power),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"demand[unit/day]",PADDR(enduse_demand), PT_DESCRIPTION, "number of loads accumulating daily",
			PT_double,"queue[unit]",PADDR(enduse_queue), PT_DESCRIPTION, "number of loads accumulated",
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),
			PT_complex,"actual_power[kVA]",PADDR(dryer_actual_power),
			PT_enumeration,"state", PADDR(state),
				PT_KEYWORD,"STOPPED",STOPPED,
				PT_KEYWORD,"RUNNING",RUNNING,
				PT_KEYWORD,"STALLED",STALLED,
				PT_KEYWORD,"TRIPPED",TRIPPED,
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
	load.power_fraction = 1.0;
	coil_power = -1;

	load.config = EUC_IS220;

	gl_warning("explicit %s model is experimental", OBJECTHDR(this)->oclass->name);

	return res;
}

int dryer::init(OBJECT *parent)
{
	int rv = 0;
	// default properties
	if (motor_power==0) motor_power = gl_random_uniform(150,350);
	if (heat_fraction==0) heat_fraction = 0.2; 
	if (stall_voltage==0) stall_voltage  = 0.6*120;
	if (trip_delay==0) trip_delay = 10;
	if (reset_delay==0) reset_delay = 60;
	if (coil_power==-1) coil_power = gl_random_uniform(3500,5000);

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	load.power_factor = 0.95;
	load.breaker_amps = 30;

	// must run before update_state() so that pCircuit can be set
	rv = residential_enduse::init(parent);

	// initial load
	if (rv==SUCCESS) update_state();

	return rv;
}

int dryer::isa(char *classname)
{
	return (strcmp(classname,"dryer")==0 || residential_enduse::isa(classname));
}


TIMESTAMP dryer::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// compute the seconds in this time step
	double dt = gl_toseconds(t0>0?t1-t0:0);

	// if advancing from a non-init condition
	if (t0>TS_ZERO && t1>t0)
	{
		// compute the total energy usage in this interval
		load.energy += load.total * dt/3600.0;
	}

	// determine the delta time until the next state change
	dt = update_state(dt);

	return dt>0?-(TIMESTAMP)(dt*TS_SECOND+t1):TS_NEVER; 
}

double dryer::update_state(double dt)
{
	// implement any state change now
	switch(state) {
	case STOPPED:
		if (enduse_queue>1)
		{
			state = RUNNING;
			enduse_queue--;
			cycle_time = cycle_duration>0 ? cycle_duration : gl_random_uniform(20,40)*60;
			state_time = 0;
		}
		break;
	case RUNNING:
		if (cycle_time<=0)
		{
			state = STOPPED;
			cycle_time = state_time = 0;
		}
		else if (pCircuit->pV->Mag()<stall_voltage)
		{
			state = STALLED;
			state_time = 0;
		}
		break;
	case STALLED:
		if (pCircuit->pV->Mag()>start_voltage)
		{
			state = RUNNING;
			state_time = cycle_time;
		}
		else if (state_time>trip_delay)
		{
			state = TRIPPED;
			state_time = 0;
		}
		break;
	case TRIPPED:
		if (state_time>reset_delay)
		{
			if (pCircuit->pV->Mag()>start_voltage)
				state = RUNNING;
			else
				state = STALLED;
			state_time = 0;
		}
		break;
	}

	// advance the state_time
	state_time += dt;

	// accumulating units in the queue no matter what happens
	enduse_queue += enduse_demand * dt/3600/24;

	// now implement current state
	switch(state) {
	case STOPPED: 
		
		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);
		
		// time to next expected state change
		dt = (enduse_demand<=0) ? -1 : 	dt = 3600/enduse_demand; 

		break;

	case RUNNING:

		// update cycle time
		cycle_time -= dt;

		// running in constant power mode with intermittent coil
		load.power.SetPowerFactor(motor_power/1000, load.power_factor);
		load.admittance = complex(coil_power/1000,0,J); // no separate coil load for now
		load.current = complex(0,0,J);

		// remaining time
		dt = cycle_time>300?300:cycle_time; // no more than 5 minutes to prevent ramp for getting too erroneous @todo eliminate 5 minute limit when pulsed load is used

		break;
	case STALLED:

		// running in constant impedance mode
		load.power = load.current = complex(0,0,J);
		load.admittance = complex(1)/stall_impedance;

		// time to trip
		dt = trip_delay;

		break;
	case TRIPPED:

		// nothing running
		load.power = load.current = load.admittance = complex(0,0,J);
		
		// time to next expected state change
		dt = reset_delay; 

		break;
	default:
		throw "unexpected motor state";
		/*	TROUBLESHOOT
			This is an error.  Please submit a bug report along with at the dryer
			object & class sections from the relevant GLM file, and from the dump file.
		*/
		break;
	}

	// compute the total electrical load
	load.total = load.power + load.current + load.admittance;
	dryer_actual_power = load.power + (load.current + load.admittance * load.voltage_factor )* load.voltage_factor;

	// compute the total heat gain
	load.heatgain = load.total.Mag() * heat_fraction;

	// Simple error catch to stop infinite loops because of rounding errors
	if (dt > 0 && dt < 1)
		dt = 1;

	return dt;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

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

EXPORT TIMESTAMP sync_dryer(OBJECT *obj, TIMESTAMP t0)
{
	dryer *my = OBJECTDATA(obj, dryer);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
