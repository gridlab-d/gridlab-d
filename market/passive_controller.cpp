/** $Id: passive_controller.h 1182 2009-09-09 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file passive_controller.cpp
	@addtogroup passive_controller
	@ingroup market

 **/

#include "passive_controller.h"

CLASS *passive_controller::oclass = NULL;

// ref: http://www.digitalmars.com/pnews/read.php?server=news.digitalmars.com&group=c++&artnum=3659
/***************************
*   erf.cpp
*   author:  Steve Strand
*   written: 29-Jan-04
***************************/

#include <math.h>


static const double rel_error= 1E-14;        //calculate 12^N^N 14 significant figures
//you can adjust rel_error to trade off between accuracy and speed
//but don't ask for > 15 figures (assuming usual 52 bit mantissa in a double)

static double tc_erfc(double x)
//erfc(x) = 2/sqrt(pi)*integral(exp(-t^2),t,x,inf)
//        = exp(-x^2)/sqrt(pi) * [1/x+ (1/2)/x+ (2/2)/x+ (3/2)/x+ (4/2)/x+ ...]
//        = 1-erf(x)
//expression inside [] is a continued fraction so '+' means add to denominator only
{
    static const double one_sqrtpi=  0.564189583547756287;        // 1/sqrt(pi)
//    if (fabs(x) < 2.2) {
//        return 1.0 - erf(x);        //use series when fabs(x) < 2.2
//    }
    if (x < 0.0) {               //continued fraction only valid for x>0
        return 2.0 - tc_erfc(-x);
    }
    double a=1, b=x;                //last two convergent numerators
    double c=x, d=x*x+0.5;          //last two convergent denominators
    double q1, q2= b/d;             //last two convergents (a/c and b/d)
    double n= 1.0, t;
    do {
        t= a*n+b*x;
        a= b;
        b= t;
        t= c*n+d*x;
        c= d;
        d= t;
        n+= 0.5;
        q1= q2;
        q2= b/d;
      } while (fabs(q1-q2)/q2 > rel_error);
    return one_sqrtpi*exp(-x*x)*q2;
} 

static double tc_erf(double x)
//erf(x) = 2/sqrt(pi)*integral(exp(-t^2),t,0,x)
//       = 2/sqrt(pi)*[x - x^3/3 + x^5/5*2! - x^7/7*3! + ...]
//       = 1-erfc(x)
{
    static const double two_sqrtpi=  1.128379167095512574;        // 2/sqrt(pi)
    if (fabs(x) > 2.2) {
        return 1.0 - tc_erfc(x);        //use continued fraction when fabs(x) > 2.2
    }
    double sum= x, term= x, xsqr= x*x;
    int j= 1;
    do {
        term*= xsqr/j;
        sum-= term/(2*j+1);
        ++j;
        term*= xsqr/j;
        sum+= term/(2*j+1);
        ++j;
    } while (fabs(term)/sum > rel_error);
    return two_sqrtpi*sum;
}

passive_controller::passive_controller(MODULE *mod){
	if(oclass == NULL){
		oclass = gl_register_class(mod,"passive_controller",sizeof(passive_controller),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if(oclass == NULL){
			GL_THROW("unable to register object class implemented by %s", __FILE__);
		}
		if(gl_publish_variable(oclass,
			// series inputs
/**/		PT_int32,"input_state",PADDR(input_state),
/**/		PT_double,"input_setpoint",PADDR(input_setpoint),
/**/		PT_bool,"input_chained",PADDR(input_chained),
			// outputs
			PT_double,"observation",PADDR(observation),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed value",
			PT_double,"mean_observation",PADDR(obs_mean),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed mean value",
			PT_double,"stdev_observation",PADDR(obs_stdev),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed standard deviation value",
			PT_double,"expectation",PADDR(expectation),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the observed expected value",
			// inputs
/**/		PT_double,"sensitivity",PADDR(sensitivity),PT_DEPRECATED,PT_DESCRIPTION,"the sensitivity of the control actuator to observation deviations",
			PT_double,"period[s]",PADDR(dPeriod),PT_DESCRIPTION,"the cycle period for the controller logic",
/**/		PT_char32,"expectation_prop",PADDR(expectation_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the property to observe for the expected value",
/**/		PT_object,"expectation_obj",PADDR(expectation_object),PT_DEPRECATED,PT_DESCRIPTION,"the object to watch for the expectation property",
			PT_char32,"expectation_property",PADDR(expectation_propname),PT_DESCRIPTION,"the name of the property to observe for the expected value",
			PT_object,"expectation_object",PADDR(expectation_object),PT_DESCRIPTION,"the object to watch for the expectation property",
/**/		PT_char32,"setpoint_prop",PADDR(output_setpoint_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the setpoint property in the parent object",
			PT_char32,"setpoint",PADDR(output_setpoint_propname),PT_DESCRIPTION,"the name of the setpoint property in the parent object",
/**/		PT_char32,"state_prop",PADDR(output_state_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the actuator property in the parent object",
			PT_char32,"state_property",PADDR(output_state_propname),PT_DESCRIPTION,"the name of the actuator property in the parent object",
/**/		PT_object,"observation_obj",PADDR(observation_object),PT_DEPRECATED,PT_DESCRIPTION,"the object to observe",
/**/		PT_char32,"observation_prop",PADDR(observation_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the observation property",
			PT_object,"observation_object",PADDR(observation_object),PT_DESCRIPTION,"the object to observe",
			PT_char32,"observation_property",PADDR(observation_propname),PT_DESCRIPTION,"the name of the observation property",
/**/		PT_char32,"mean_observation_prop",PADDR(observation_mean_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the mean observation property",
			PT_char32,"stdev_observation_prop",PADDR(observation_stdev_propname),PT_DEPRECATED,PT_DESCRIPTION,"the name of the standard deviation observation property",
			PT_char32,"stdev_observation_property",PADDR(observation_stdev_propname),PT_DESCRIPTION,"the name of the standard deviation observation property",
/**/		PT_int32,"cycle_length",PADDR(cycle_length),PT_DEPRECATED,PT_DESCRIPTION,"length of time between processing cycles",
			PT_double,"base_setpoint",PADDR(base_setpoint),PT_DESCRIPTION,"the base setpoint to base control off of",
			// probabilistic control inputs
			PT_enumeration, "distribution_type",PADDR(distribution_type),
				PT_KEYWORD, "NORMAL", PDT_NORMAL,
				PT_KEYWORD, "EXPONENTIAL", PDT_EXPONENTIAL,
				PT_KEYWORD, "UNIFORM", PDT_UNIFORM,
			PT_double, "comfort_level",PADDR(comfort_level),
			// specific inputs
			PT_double,"range_high",PADDR(range_high),
			PT_double,"range_low",PADDR(range_low),
			PT_double,"ramp_high",PADDR(ramp_high),
			PT_double,"ramp_low",PADDR(ramp_low),
			// specific outputs
/**/		PT_double,"prob_off",PADDR(prob_off),
/**/		PT_int32,"output_state",PADDR(output_state),PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"the target setpoint given the input observations",
/**/		PT_double,"output_setpoint",PADDR(output_setpoint),
			// enums
			PT_enumeration,"control_mode",PADDR(control_mode),PT_DESCRIPTION,"the control mode to use for determining controller action",
				PT_KEYWORD,"NONE",CM_NONE,
				PT_KEYWORD,"RAMP",CM_RAMP,
				PT_KEYWORD,"DUTYCYCLE",CM_DUTYCYCLE,
				PT_KEYWORD,"PROBABILITY_OFF",CM_PROBOFF,
			NULL) < 1)
		{
				GL_THROW("unable to publish properties in %s",__FILE__);
		}
		memset(this,0,sizeof(passive_controller));
	}
}

int passive_controller::create(){
	memset(this, 0, sizeof(passive_controller));
	sensitivity = 1.0;
	comfort_level = 1.0;
	return 1;
}

int passive_controller::init(OBJECT *parent){
	OBJECT *hdr = OBJECTHDR(this);
	gl_set_dependent(hdr, expectation_object);
	gl_set_dependent(hdr, observation_object);
	if(parent == NULL){
		gl_error("passive_controller has no parent and will be operating in 'dummy' mode");
	} else {
		if(output_state_propname[0] == 0 && output_setpoint_propname[0] == 0){
			GL_THROW("passive_controller has no output properties");
		}
		// expectation_addr
		if(expectation_object != 0){
			expectation_property = gl_get_property(expectation_object, expectation_propname);
			if(expectation_property == 0){
				GL_THROW("passive_controller cannot find its expectation property");
			}
			expectation_addr = (void *)((unsigned int64)expectation_object + sizeof(OBJECT) + (unsigned int64)expectation_property->addr);
		}

		if(observation_object != 0){
			// observation_addr
			observation_prop = gl_get_property(observation_object, observation_propname);
			if(observation_prop != 0){
				observation_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)observation_prop->addr);
			}
			// observation_mean_addr
			observation_mean_prop = gl_get_property(observation_object, observation_mean_propname);
			if(observation_mean_prop != 0){
				observation_mean_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)observation_mean_prop->addr);
			}
			// observation_stdev_addr
			stdev_observation_property = gl_get_property(observation_object, observation_stdev_propname);
			if(stdev_observation_property != 0){
				observation_stdev_addr = (void *)((unsigned int64)observation_object + sizeof(OBJECT) + (unsigned int64)stdev_observation_property->addr);
			}

		}
		// output_state
		if(output_state_propname[0] != 0){
			output_state_prop = gl_get_property(parent, output_state_propname);
			if(output_state_prop == NULL){
				GL_THROW("passive_controller parent \"%s\" does not contain property \"%s\"", 
					(parent->name ? parent->name : "anon"), output_state_propname);
			}
			output_setpoint_addr = (void *)((unsigned int64)parent + sizeof(OBJECT) + (unsigned int64)output_state_prop->addr);
		}
		
		// output_setpoint
		if(output_setpoint_propname[0] == 0 && output_setpoint_propname[0] == 0){
			GL_THROW("passive_controller has no output properties");
		}
		if(output_setpoint_propname[0] != 0){
			output_setpoint_property = gl_get_property(parent, output_setpoint_propname);
			if(output_setpoint_property == NULL){
				GL_THROW("passive_controller parent \"%s\" does not contain property \"%s\"", 
					(parent->name ? parent->name : "anon"), output_setpoint_propname);
			}
			output_setpoint_addr = (void *)((unsigned int64)parent + sizeof(OBJECT) + (unsigned int64)output_setpoint_property->addr);
		}
	}

	if(observation_object == NULL){
		GL_THROW("passive_controller observation_object object is undefined, and can not function");
	}
	
	//
	// make sure that the observation_object and expectable are ranked above the controller
	//

	if(base_setpoint != 0.0){
		orig_setpoint = 1;
	}

	if(dPeriod == 0.0){
		dPeriod = 300.0;
		period = 300; // five minutes
	} else {
		period = (TIMESTAMP)floor(dPeriod + 0.5);
	}

	return 1;
}


int passive_controller::isa(char *classname)
{
	return strcmp(classname,"passive_controller")==0;
}


TIMESTAMP passive_controller::presync(TIMESTAMP t0, TIMESTAMP t1){
	// determine output based on control mode
	if(last_cycle == 0 || t1 >= last_cycle + period || period == 0){
		last_cycle = t1; // advance cycle time
			// get observations
		if(observation_addr != 0){
			observation = *(double *)observation_addr;
		} else {
			observation = 0;
		}
		if(observation_mean_addr != 0){
			obs_mean = *(double *)observation_mean_addr;
		} else {
			obs_mean = 0;
		}
		if(observation_stdev_addr != 0){
			obs_stdev = *(double *)observation_stdev_addr;
		} else {
			obs_stdev = 0;
		}

		// get expectation
		if(expectation_addr != 0){
			expectation = *(double *)expectation_addr;
		} else {
			expectation = 0;
		}

		switch(control_mode){
			case CM_NONE:
				// no control ~ let it slide
				break;
			case CM_RAMP:
				if(calc_ramp(t0, t1) != 0){
					GL_THROW("error occured when handling the ramp control mode");
				}
				break;
			case CM_PROBOFF:
				if(calc_proboff(t0, t1) != 0){
					GL_THROW("error occured when handling the probabilistic cutoff control mode");
				}
				break;
			case CM_DUTYCYCLE:
				if(calc_dutycycle(t0, t1) != 0){
					GL_THROW("error occured when handling the duty cycle control mode");
				}
				break;
			default:
				GL_THROW("passive_controller has entered an invalid control mode");
				break;
		}

		// determine if input is chained first
		if(output_setpoint_addr != 0){
			*(double *)output_setpoint_addr = output_setpoint;
		}
		if(output_state_addr != 0){
			*(int *)output_state_addr = output_state;
		}
	}
	return (period > 0 ? last_cycle+period : TS_NEVER);
}

TIMESTAMP passive_controller::sync(TIMESTAMP t0, TIMESTAMP t1){
#if 0
	// determine output based on control mode
	if(t1 <= last_cycle + period || period == 0){
		last_cycle = t1; // advance cycle time
		switch(control_mode){
			case CM_NONE:
				// no control ~ let it slide
				break;
			case CM_RAMP:
				if(calc_ramp(t0, t1) != 0){
					GL_THROW("error occured when handling the ramp control mode");
				}
				break;
			case CM_PROBOFF:
				if(calc_proboff(t0, t1) != 0){
					GL_THROW("error occured when handling the probabilistic cutoff control mode");
				}
				break;
			case CM_DUTYCYCLE:
				if(calc_dutycycle(t0, t1) != 0){
					GL_THROW("error occured when handling the duty cycle control mode");
				}
				break;
			default:
				GL_THROW("passive_controller has entered an invalid control mode");
				break;
		}

		// determine if input is chained first
		if(output_setpoint_addr != 0){
			*(double *)output_setpoint_addr = output_setpoint;
		}
		if(output_state_addr != 0){
			*(int *)output_state_addr = output_state;
		}
	}
	return (period > 0 ? last_cycle+period : TS_NEVER);
#endif
	return TS_NEVER;
}

TIMESTAMP passive_controller::postsync(TIMESTAMP t0, TIMESTAMP t1){
	return TS_NEVER;
}

int passive_controller::calc_ramp(TIMESTAMP t0, TIMESTAMP t1){
	double T_limit;
	double T_set;
	double ramp;
	double set_change;
	extern double bid_offset;

	if(!orig_setpoint){
		base_setpoint = *(double *)output_setpoint_addr;
		orig_setpoint = 1;
	}

	// "olypen style" ramp with k_high, k_low, rng_high, rng_low
	if(expectation_addr == 0 || observation_addr == 0 || observation_stdev_addr == 0){
		gl_error("insufficient input for ramp control mode");
		return -1;
	}
	if(ramp_high * ramp_low < 0 || range_high * range_low > 0){ // zero is legit
		gl_warning("invalid ramp parameters");
	}

	/*
	if(observation > expectation){ // net ramp direction
		ramp = ramp_high;
	} else {
		ramp = ramp_low;
	}
	*/

	//T_limit = (observation > expectation && ramp > 0.0 ? range_high : range_low);
	if(observation > expectation){
		if(ramp_low > 0.0){
			T_limit = range_high;
			ramp = ramp_high;
		} else {
			T_limit = range_low;
			ramp = ramp_low;
		}
		//T_limit = (ramp_low > 0.0 ? range_high : range_low);
	} else if (observation < expectation){
		if(ramp_low < 0.0){
			T_limit = range_high;
			ramp = ramp_high;
		} else {
			T_limit = range_low;
			ramp = ramp_low;
		}
		//T_limit = (ramp_low 0.0 ? range_low : range_high);
	} else {
		output_setpoint = base_setpoint;
		output_state = 0;
		return 0;
	}

	if(T_limit == 0){
		// zero range short-circuit
		output_setpoint = base_setpoint;
		output_state = 0;
		return 0;
	}

	T_set = base_setpoint;

	// is legit to set expectation to the mean
	if(obs_stdev < bid_offset){
		set_change = 0.0;
	} else {
		set_change = (observation - expectation) * fabs(T_limit) / (ramp * obs_stdev);
	}
	if(set_change > range_high){
		set_change = range_high;
	}
	if(set_change < range_low){
		set_change = range_low;
	}
	output_setpoint = T_set + set_change;
	output_state = 0;
	return 0;
}


int passive_controller::calc_dutycycle(TIMESTAMP t0, TIMESTAMP t1){
	// not sure.
	return 0;
}


int passive_controller::calc_proboff(TIMESTAMP t0, TIMESTAMP t1){
	if(observation_addr == 0 || expectation_addr == 0 || observation_stdev_addr == 0){
		gl_error("insufficient input for probabilistic shutoff control mode");
		return -1;
	}
	double erf_in, erf_out;
	double cdf_out;
	double r;
	extern double bid_offset;

	
	switch(distribution_type){
		case PDT_NORMAL:
			// N is the cumulative normal distribution
			// k_w is a comfort setting
			// r is compared to a uniformly random number on [0,1.0)
			// erf_in = (x-mean) / (var^2 * sqrt(2))
			if(obs_stdev < bid_offset){ // short circuit
				if(observation > expectation){
					output_state = -1;
					prob_off = 1.0;
				} else {
					output_state = 0;
					prob_off = 0.0;
				}
				return 0;
			}
			erf_in = (observation - expectation) / (obs_stdev*obs_stdev * sqrt(2.0));
			erf_out = tc_erf(erf_in);
			cdf_out = 0.5 * (1 + erf_out);
			prob_off = sensitivity * (cdf_out-0.5);			
			break;
		case PDT_EXPONENTIAL:
			prob_off = comfort_level * (1 - exp(-observation/expectation));
			break;
		case PDT_UNIFORM:
			prob_off = comfort_level * (observation - expectation) / expectation;
			break;
		default:
			gl_error("");
	}
	if(prob_off < 0 || !isfinite(prob_off)){
		prob_off = 0.0;
	}
	r = gl_random_uniform(0.0,1.0);
	if(r < prob_off){
		output_state = -1; // off?
	} else {
		output_state = 0;
	}

	return 0;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_passive_controller(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(passive_controller::oclass);
		if (*obj!=NULL)
		{
			passive_controller *my = OBJECTDATA(*obj,passive_controller);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_passive_controller: %s", msg);
	}
	return 1;
}

EXPORT int init_passive_controller(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL){
			return OBJECTDATA(obj,passive_controller)->init(parent);
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("init_passive_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return 1;
}

EXPORT int isa_passive_controller(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,passive_controller)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_passive_controller(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	passive_controller *my = OBJECTDATA(obj,passive_controller);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
			t2 = my->sync(obj->clock, t1);
			break;
		case PC_POSTTOPDOWN:
			t2 = my->postsync(obj->clock,t1);
			obj->clock = t1;
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
	}
	catch (char *msg)
	{
		char name[64];
		gl_error("sync_passive_controller(obj=%s): %s", gl_name(obj,name,sizeof(name)), msg);
	}
	return t2;
}

// EOF

