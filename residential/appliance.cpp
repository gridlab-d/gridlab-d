/** $Id: appliance.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2012 Battelle Northwest
 **/

#include "appliance.h"

EXPORT_CREATE(appliance);
EXPORT_INIT(appliance);
EXPORT_PRECOMMIT(appliance);
EXPORT_SYNC(appliance);
EXPORT_NOTIFY(appliance);

CLASS *appliance::oclass = nullptr;
CLASS *appliance::pclass = nullptr;
appliance *appliance::defaults = nullptr;

appliance::appliance(MODULE *module) : residential_enduse(module)
{
	if ( oclass==nullptr )
	{
		pclass = residential_enduse::oclass;
		oclass = gl_register_class(module, "appliance",sizeof(appliance),PC_PRETOPDOWN|PC_AUTOLOCK);
		if ( oclass==nullptr )
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		if ( gl_publish_variable(oclass,
			PT_INHERIT,"residential_enduse",
			PT_complex_array, "powers",PADDR(power),
			PT_complex_array, "impedances",PADDR(impedance),
			PT_complex_array, "currents",PADDR(current),
			PT_double_array, "durations",PADDR(duration),
			PT_double_array, "transitions",PADDR(transition),
			PT_double_array, "heatgains", PADDR(heatgain),
			PT_object, "defaults", PADDR(defaults), PT_ACCESS, PA_HIDDEN, PT_DESCRIPTION, "CHECKPOINT_VAR: internal variable for appliance default object",
			nullptr)<1 )
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}  


int appliance::create()
{
	int res = residential_enduse::create();
	return res;
}

void appliance::shared_init(void)
{
	// These variables need intialized every time regardless of checkpoint load
	// Non-published variables (not loaded from checkpoint) must be initialized here
	transition_probabilities = nullptr;
	n_states = 0;
	state = 0;
	next_t = TS_NEVER;
}

int appliance::checkpoint_init(OBJECT *parent)
{
	// Only initialize variables that aren't published.  If a variable is published, it will be loaded from checkpoint, and we don't want to reinitialize it.
	shared_init();
	return residential_enduse::checkpoint_init(parent);
}

int appliance::init(OBJECT *parent)
{
	// Initialize non-published variables
	shared_init();

	gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());

	// check that duration is a vector
	if ( duration.rows()!=1 )
		exception("duration must have 1 rows (it has %d)", n_states, duration.rows());

	// number of states if length of duration vector
	n_states = (unsigned int)duration.cols();
	if ( state<0 || state>=n_states )
		exception("initial state must be between 0 and %d, inclusive", n_states-1);
	gl_debug("n_states = %d (initial state is %d)", n_states, state);

	// transition must be either 1xN or NxN
	if ( ( transition.rows()!=1 && transition.rows()!=n_states ) || transition.cols()!=n_states )
		exception("transition must have either 1r x %dc or %dr x %dc (it is %dr c %dc)", n_states, n_states, n_states, transition.rows(), transition.cols());

	// default impedance is zero
	if ( impedance.size() == 0)
	{
		impedance.resize(0,n_states-1);
		gld::complex zero(0);
		impedance.setZero(); // = zero;
	}
	if ( impedance.rows()!=1 || impedance.cols()!=n_states )
		exception("impedance must 1r x %dc (it is %dr x %dc)", n_states, impedance.rows(), impedance.cols());

	// default current is zero
	if ( current.isZero() )
	{
		current.resize(0,n_states-1);
		gld::complex zero(0);
		current.setZero();
	}
	if ( current.rows()!=1 || current.cols()!=n_states )
		exception("current must 1r x %dc (it is %dr x %dc)", n_states, current.rows(), current.cols());

	// default power is zero
	if ( power.size()==0 )
	{
		power.resize(0,n_states-1);
		gld::complex zero(0);
		power.setZero();
	}
	if ( power.rows()!=1 || power.cols()!=n_states )
		exception("power must 1r x %dc (it is %dr x %dc)", n_states, power.rows(), power.cols());

	// default heatgain is zero
	if ( heatgain.size() == 0)
	{
		heatgain.resize(0,n_states-1);
		heatgain.setZero();
	}
	if ( heatgain.rows()!=1 || heatgain.cols()!=n_states )
		exception("current must 1r x %dc (it is %dr x %dc)", n_states, heatgain.rows(), heatgain.cols());

	// allocated space of transition matrix row
	if ( transition.rows()>1 )
		transition_probabilities = new double[n_states];

	// ready to start
	update_next_t();
	return residential_enduse::init(parent);
}

void appliance::update_next_t(void)
{
	double transition_probability = transition(0,state);
	if ( !isfinite(transition_probability) )
	{
		// transition occurs exactly at the next scheduled time
		next_t = gl_globalclock + (TIMESTAMP)duration(0,state);
		gl_debug("%s: non-probabilistic transition scheduled at %lld", get_name(), next_t);
	}
	else if ( gl_random_uniform(&my()->rng_state,0,1)<transition_probability )
	{
		// transition is uncertain
		next_t = gl_globalclock + (TIMESTAMP)gl_random_uniform(&my()->rng_state,1,duration(0,state));
		gl_debug("%s: transition scheduled at %lld", get_name(), next_t);
	}
	else
	{
		// transition does not occur so check in again later
		next_t = -(gl_globalclock + (TIMESTAMP)duration(0,state));
		gl_debug("%s: no transition scheduled prior to %lld", get_name(), next_t);
	}
}
void appliance::update_power(void)
{
	gld::complex Z = impedance(0,state);
	load.admittance = Z.Mag()==0  ? gld::complex(0) : gld::complex(1)/Z;
	load.current = current(0,state);
	load.power = power(0,state);
	load.heatgain = heatgain(0,state);
}
void appliance::update_state(void)
{
	if ( transition_probabilities==nullptr )
	{
		// linear transition array
		state = (state+1)%n_states;
		gl_debug("%s: now in state %d", get_name(), state);
	}
	else
	{
		// transition matrix
		auto transition_probabilities = transition.row(state);

		// generate a random uniform number
		double rn = gl_random_uniform(&my()->rng_state,0,1);

		// find the state that correspond to that cumulative probabilities
		int n;
		for ( n=0 ; n<n_states ; n++ )
		{
			rn -= transition_probabilities[n];
			if ( rn<0 )
			{
				state = n;
				gl_debug("%s: now in state %d", get_name(), state);
				break;
			}
		}
	}
	update_power();
}

int appliance::precommit(TIMESTAMP t1)
{
	gld_clock now;

	// transition occurs now
	if ( now==(next_t<0?-next_t:next_t) )
	{
		update_state();
		update_next_t();
		return 1;
	}

	// next transition cannot be scheduled yet--just checking in
	else if ( next_t<0 )
	{
		if ( now>-next_t )
			update_next_t();
		return 1;
	}

	// next transition was missed somehow (this should never occur)
	else if ( now>next_t )
	{	
		gl_error("%s: transition at %lld missed", get_name(), next_t); 
		return 0;
	}

	// transition has yet to occur
	else
	{
		return 1;
	}
}

TIMESTAMP appliance::presync(TIMESTAMP t1)
{
	return next_t;
}

int appliance::postnotify(PROPERTY *prop, char *value)
{
	// TODO reset state when duration or transition changes
	return 1;
}
