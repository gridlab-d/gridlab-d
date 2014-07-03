/** $Id: appliance.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2012 Battelle Northwest
 **/

#include "appliance.h"

EXPORT_CREATE(appliance);
EXPORT_INIT(appliance);
EXPORT_PRECOMMIT(appliance);
EXPORT_SYNC(appliance);
EXPORT_NOTIFY(appliance);

CLASS *appliance::oclass = NULL;
CLASS *appliance::pclass = NULL;
appliance *appliance::defaults = NULL;

appliance::appliance(MODULE *module) : residential_enduse(module)
{
	if ( oclass==NULL )
	{
		pclass = residential_enduse::oclass;
		oclass = gl_register_class(module,"appliance",sizeof(appliance),PC_PRETOPDOWN|PC_AUTOLOCK);
		if ( oclass==NULL )
			GL_THROW("unable to register object class implemented by %s",__FILE__);
		if ( gl_publish_variable(oclass,
			PT_INHERIT,"residential_enduse",
			PT_complex_array, "powers",get_power_offset(),
			PT_complex_array, "impedances",get_impedance_offset(),
			PT_complex_array, "currents",get_current_offset(),
			PT_double_array, "durations",get_duration_offset(),
			PT_double_array, "transitions",get_transition_offset(),
			PT_double_array, "heatgains", get_heatgain_offset(),
			NULL)<1 )
			GL_THROW("unable to publish properties in %s",__FILE__);
	}
}  

appliance::~appliance()
{
}

int appliance::create()
{
	int res = residential_enduse::create();
	return res;
}

int appliance::init(OBJECT *parent)
{

	gl_warning("This device, %s, is considered very experimental and has not been validated.", get_name());

	// check that duration is a vector
	if ( duration.get_rows()!=1 )
		exception("duration must have 1 rows (it has %d)", n_states, duration.get_rows());

	// number of states if length of duration vector
	n_states = (unsigned int)duration.get_cols();
	if ( state<0 || state>=n_states )
		exception("initial state must be between 0 and %d, inclusive", n_states-1);
	gl_debug("n_states = %d (initial state is %d)", n_states, state);

	// transition must be either 1xN or NxN
	if ( ( transition.get_rows()!=1 && transition.get_rows()!=n_states ) || transition.get_cols()!=n_states )
		exception("transition must have either 1r x %dc or %dr x %dc (it is %dr c %dc)", n_states, n_states, n_states, transition.get_rows(), transition.get_cols());

	// default impedance is zero
	if ( impedance.is_empty() )
	{
		impedance.grow_to(0,n_states-1);
		complex zero(0);
		impedance = zero;
	}
	if ( impedance.get_rows()!=1 || impedance.get_cols()!=n_states )
		exception("impedance must 1r x %dc (it is %dr x %dc)", n_states, impedance.get_rows(), impedance.get_cols());

	// default current is zero
	if ( current.is_empty() )
	{
		current.grow_to(0,n_states-1);
		complex zero(0);
		current = zero;
	}
	if ( current.get_rows()!=1 || current.get_cols()!=n_states )
		exception("current must 1r x %dc (it is %dr x %dc)", n_states, current.get_rows(), current.get_cols());

	// default power is zero
	if ( power.is_empty() )
	{
		power.grow_to(0,n_states-1);
		complex zero(0);
		power = zero;
	}
	if ( power.get_rows()!=1 || power.get_cols()!=n_states )
		exception("power must 1r x %dc (it is %dr x %dc)", n_states, power.get_rows(), power.get_cols());

	// default heatgain is zero
	if ( heatgain.is_empty() )
	{
		heatgain.grow_to(0,n_states-1);
		heatgain = 0;
	}
	if ( heatgain.get_rows()!=1 || heatgain.get_cols()!=n_states )
		exception("current must 1r x %dc (it is %dr x %dc)", n_states, heatgain.get_rows(), heatgain.get_cols());

	// allocated space of transition matrix row
	if ( transition.get_rows()>1 )
		transition_probabilities = new double[n_states];

	// ready to start
	update_next_t();
	return residential_enduse::init(parent);
}

void appliance::update_next_t(void)
{
	double transition_probability = transition.get_at(0,state);
	if ( !isfinite(transition_probability) )
	{
		// transition occurs exactly at the next scheduled time
		next_t = gl_globalclock + (TIMESTAMP)duration.get_at(0,state);
		gl_debug("%s: non-probabilistic transition scheduled at %lld", get_name(), next_t);
	}
	else if ( gl_random_uniform(&my()->rng_state,0,1)<transition_probability )
	{
		// transition is uncertain
		next_t = gl_globalclock + (TIMESTAMP)gl_random_uniform(&my()->rng_state,1,duration.get_at(0,state));
		gl_debug("%s: transition scheduled at %lld", get_name(), next_t);
	}
	else
	{
		// transition does not occur so check in again later
		next_t = -(gl_globalclock + (TIMESTAMP)duration.get_at(0,state));
		gl_debug("%s: no transition scheduled prior to %lld", get_name(), next_t);
	}
}
void appliance::update_power(void)
{
	complex Z = impedance.get_at(0,state);
	load.admittance = Z.Mag()==0  ? complex(0) : complex(1)/Z;
	load.current = current.get_at(0,state);
	load.power = power.get_at(0,state);
	load.heatgain = heatgain.get_at(0,state);
}
void appliance::update_state(void)
{
	if ( transition_probabilities==NULL )
	{
		// linear transition array
		state = (state+1)%n_states;
		gl_debug("%s: now in state %d", get_name(), state);
	}
	else
	{
		// transition matrix
		transition.extract_row(transition_probabilities,state);

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
