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
		oclass = gl_register_class(module,"clotheswasher",sizeof(clotheswasher),PC_PRETOPDOWN|PC_BOTTOMUP);
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

	return res;
}

int clotheswasher::init(OBJECT *parent)
{
	// default properties
	if (shape.params.analog.power==0) shape.params.analog.power = gl_random_uniform(0.100,0.750);		// clotheswasher size [W]
	if (load.heatgain_fraction==0) load.heatgain_fraction = 0.5; 
	if (load.power_factor==0) load.power_factor = 0.95;
	if (stall_voltage==0) stall_voltage  = 0.7*120;
	if (trip_delay==0) trip_delay = 10;
	if (reset_delay==0) reset_delay = 60;

	OBJECT *hdr = OBJECTHDR(this);
	hdr->flags |= OF_SKIPSAFE;

	if(shape.params.analog.power < 0.1){
		gl_error("clotheswasher motor is undersized, using 500W motor");
		shape.params.analog.power = 0.5;
	}

	int res = residential_enduse::init(parent);

	if (res==SUCCESS && shape.type==MT_UNKNOWN)
		update_state(0);

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

	switch(shape.type){
		case MT_UNKNOWN:
			dt = gl_toseconds(t0>0?t1-t0:0);
			// determine the delta time until the next state change
			dt = update_state(dt);
			break;
		case MT_ANALOG:
			break;
		case MT_PULSED:
			break;
		case MT_MODULATED:
			break;
		default:
			GL_THROW("clotheswasher load shape has an unknown state!");
			break;
	}

	gl_enduse_sync(&(residential_enduse::load),t1);

	if(dt>0){
		if(t2 > (TIMESTAMP)(dt*TS_SECOND + t0)){
			t2 = (TIMESTAMP)(dt*TS_SECOND + t0);
		}
	}
	return -t2;
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

		// running in constant power mode
		load.power.SetPowerFactor(shape.params.analog.power, load.power_factor);
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
		/*	TROUBLESHOOT
			This is an error.  Please submit a bug report along with at the clotheswasher
			object & class sections from the relevant GLM file, and from the dump file.
		*/
		break;
	}

	// compute the total electrical load
	load.total = load.power + ~(load.current + load.admittance**pCircuit->pV)**pCircuit->pV/1000;

	// compute the total heat gain
	load.heatgain = load.total.Mag() * load.heatgain_fraction * BTUPHPKW;

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
