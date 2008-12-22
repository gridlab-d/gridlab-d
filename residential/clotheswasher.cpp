/** $Id: clotheswasher.cpp,v 1.13 2008/02/13 02:22:35 d3j168 Exp $
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

#include "house.h"
#include "clotheswasher.h"

//////////////////////////////////////////////////////////////////////////
// clotheswasher CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* clotheswasher::oclass = NULL;
clotheswasher *clotheswasher::defaults = NULL;

clotheswasher::clotheswasher(MODULE *module) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"clotheswasher",sizeof(clotheswasher),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_double,"motor_power[W]",PADDR(motor_power),
			PT_double,"power_factor[unit]",PADDR(power_factor),
			PT_double,"circuit_split",PADDR(circuit_split),
			PT_double,"heat_fraction[unit]",PADDR(heat_fraction),
			PT_double,"enduse_demand[unit]",PADDR(enduse_demand),
			PT_double,"enduse_queue[unit]",PADDR(enduse_queue),
			PT_complex,"enduse_load[kW]",PADDR(load.total),
			PT_complex,"constant_power[kW]",PADDR(load.power),
			PT_complex,"constant_current[A]",PADDR(load.current),
			PT_complex,"constant_admittance[1/Ohm]",PADDR(load.admittance),
			PT_double,"internal_gains[kW]",PADDR(load.heatgain),
			PT_double,"energy_meter[kWh]",PADDR(load.energy),
			PT_double,"stall_voltage[V]", PADDR(stall_voltage),
			PT_double,"start_voltage[V]", PADDR(start_voltage),
			PT_complex,"stall_impedance[Ohm]", PADDR(stall_impedance),
			PT_double,"trip_delay[s]", PADDR(trip_delay),
			PT_double,"reset_delay[s]", PADDR(reset_delay),
			PT_enumeration,"state", PADDR(state),
				PT_KEYWORD,"STOPPED",STOPPED,
				PT_KEYWORD,"RUNNING",RUNNING,
				PT_KEYWORD,"STALLED",STALLED,
				PT_KEYWORD,"TRIPPED",TRIPPED,
			NULL)<1) 
			GL_THROW("unable to publish properties in %s",__FILE__);

		// setup the default values
		defaults = this;
		memset(this,0,sizeof(clotheswasher));
		load.power = load.admittance = load.current = load.total = complex(0,0,J);
	}
}

clotheswasher::~clotheswasher()
{
}

int clotheswasher::create() 
{
	// copy the defaults
	memcpy(this,defaults,sizeof(clotheswasher));

	return 1;
}

int clotheswasher::init(OBJECT *parent)
{
	// default properties
	if (motor_power==0) motor_power = gl_random_uniform(100,750);		// clotheswasher size [W]
	if (heat_fraction==0) heat_fraction = 0.5; 
	if (power_factor==0) power_factor = 0.95;
	if (stall_voltage==0) stall_voltage  = 0.7*120;
	if (trip_delay==0) trip_delay = 10;
	if (reset_delay==0) reset_delay = 60;

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if (parent==NULL || !gl_object_isa(parent,"house"))
	{
		gl_error("clotheswasher must have a parent house");
		return 0;
	}

	// attach object to house panel
	house *pHouse = OBJECTDATA(parent,house);
	pVoltage = (pHouse->attach(OBJECTHDR(this),20,false))->pV;

	// initial load
	update_state(0);

	return 1;
}

TIMESTAMP clotheswasher::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	// compute the seconds in this time step
	double dt = gl_toseconds(t0>0?t1-t0:0);

	// if advancing from a non-init condition
	if (t0>TS_ZERO && t1>t0)
	{
		// compute the total energy usage in this interval
		load.energy += load.total.Mag() * dt*3600;
	}

	// determine the delta time until the next state change
	dt = update_state(dt);

	return dt>0?(TIMESTAMP)(dt*TS_SECOND):TS_NEVER; 
}

double clotheswasher::update_state(double dt)
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
		else if (pVoltage->Mag()<stall_voltage)
		{
			state = STALLED;
			state_time = 0;
		}
		break;
	case STALLED:
		if (pVoltage->Mag()>start_voltage)
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
			if (pVoltage->Mag()>start_voltage)
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
	enduse_queue += enduse_demand * dt/3600;

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

		// running in constant power mode
		load.power.SetPowerFactor(motor_power/1000, power_factor);
		load.current = load.admittance = complex(0,0,J);

		// remaining time
		dt = cycle_time;

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
		break;
	}

	// compute the total electrical load
	load.total = load.power + ~(load.current + load.admittance**pVoltage)**pVoltage/1000;

	// compute the total heat gain
	load.heatgain = load.total.Mag() * heat_fraction;

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

EXPORT TIMESTAMP sync_clotheswasher(OBJECT *obj, TIMESTAMP t0)
{
	clotheswasher *my = OBJECTDATA(obj, clotheswasher);
	TIMESTAMP t1 = my->sync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}

/**@}**/
