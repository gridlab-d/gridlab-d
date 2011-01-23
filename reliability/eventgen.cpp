/** $Id: eventgen.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file eventgen.cpp
	@class eventgen Event generator
	@ingroup reliability

	The event generation object synthesizing reliability events for a target
	group of object in a GridLAB-D model.  The following parameters are used
	to generate events

	- \p group is used to identify the characteristics of the target group.
	  See find_mkpgm for details on \p group syntax.
	- \p targets identifies the target properties that will be changed when
	  an event is generated.
	- \p values identifies the values to be written to the target properties
	  when an event is generated.
	- \p frequency specifies the frequency with which events are generated
	- \p duration specifies the duration of events when generated.

	The current implementation generates events with an exponential distribution.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "eventgen.h"

CLASS *eventgen::oclass = NULL;			/**< a pointer to the CLASS definition in GridLAB-D's core */
eventgen *eventgen::defaults = NULL;	/**< a pointer to the default values used when creating new objects */

static PASSCONFIG clockpass = PC_PRETOPDOWN;

/* Class registration is only called once to register the class with the core */
eventgen::eventgen(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"eventgen",sizeof(eventgen),PC_PRETOPDOWN|PC_POSTTOPDOWN);
		if (oclass==NULL)
			throw "unable to register class eventgen";
		else
			oclass->trl = TRL_DEMONSTRATED;

		if (gl_publish_variable(oclass,
			PT_char1024, "target_group", PADDR(target_group),
			PT_char256, "fault_type", PADDR(fault_type),
			PT_enumeration, "failure_dist", PADDR(failure_dist),
				PT_KEYWORD, "UNIFORM", UNIFORM,
				PT_KEYWORD, "NORMAL", NORMAL,
				PT_KEYWORD, "LOGNORMAL", LOGNORMAL,
				PT_KEYWORD, "BERNOULLI", BERNOULLI,
				PT_KEYWORD, "PARETO", PARETO,
				PT_KEYWORD, "EXPONENTIAL", EXPONENTIAL,
				PT_KEYWORD, "RAYLEIGH", RAYLEIGH,
				PT_KEYWORD, "WEIBULL", WEIBULL,
				PT_KEYWORD, "GAMMA", GAMMA,
				PT_KEYWORD, "BETA", BETA,
				PT_KEYWORD, "TRIANGLE", TRIANGLE,
			PT_enumeration, "restoration_dist", PADDR(restore_dist),
				PT_KEYWORD, "UNIFORM", UNIFORM,
				PT_KEYWORD, "NORMAL", NORMAL,
				PT_KEYWORD, "LOGNORMAL", LOGNORMAL,
				PT_KEYWORD, "BERNOULLI", BERNOULLI,
				PT_KEYWORD, "PARETO", PARETO,
				PT_KEYWORD, "EXPONENTIAL", EXPONENTIAL,
				PT_KEYWORD, "RAYLEIGH", RAYLEIGH,
				PT_KEYWORD, "WEIBULL", WEIBULL,
				PT_KEYWORD, "GAMMA", GAMMA,
				PT_KEYWORD, "BETA", BETA,
				PT_KEYWORD, "TRIANGLE", TRIANGLE,
			PT_double, "failure_dist_param_1", PADDR(fail_dist_params[0]),
			PT_double, "failure_dist_param_2", PADDR(fail_dist_params[1]),
			PT_double, "restoration_dist_param_1", PADDR(rest_dist_params[0]),
			PT_double, "restoration_dist_param_2", PADDR(rest_dist_params[1]),
			PT_double, "max_outage_length[s]", PADDR(max_outage_length_dbl),
			PT_int32, "max_simultaneous_faults", PADDR(max_simult_faults),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
	}
}

/* Object creation is called once for each object that is created by the core */
int eventgen::create(void)
{
	target_group[0] = '\0';
	fault_type[0] = '\0';

	//Initial distributions set up for old reliability defaults
	failure_dist = EXPONENTIAL;
	restore_dist = PARETO;

	//Arbitrary initial values
	fail_dist_params[0] = 1.0/2592000.0;//30 days mean time to failure (exponential)
	fail_dist_params[1] = 0.0;			//Not needed for exponential

	rest_dist_params[0] = 1.0;				//Minimum value parameter - Pareto
	rest_dist_params[1] = 1.00027785496;	//1 hour mean time to restore - shape

	UnreliableObjs = NULL;
	UnreliableObjCount = 0;

	metrics_obj = NULL;

	max_outage_length_dbl = 432000.0;	//5 day maximum outage by default

	next_event_time = 0;

	max_simult_faults = -1;	//-1 = no limit to number of simult faults
	faults_in_prog = 0;		//Assume we start with no faults

	curr_time_interrupted = 0;
	diff_count_needed = false;

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int eventgen::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index;
	TIMESTAMP tempTime;
	FINDLIST *ObjListVals;
	OBJECT *temp_obj;

	//See if our max outage length is above the global - if so, truncate it
	if (max_outage_length_dbl > event_max_duration)
	{
		max_outage_length_dbl = event_max_duration;

		gl_warning("Maximum outage for %s was above global max - restricted to global max",hdr->name);
		/*  TROUBLESHOOT
		The max_outage_length value exceeds the module global event length defined in maximum_event_length.  Th
		max_outage_length has been truncated to the module global value.  If this is undesired, specify and increase
		the global maximum_event_length value.
		*/
	}

	//Map the metrics object
	if (hdr->parent == NULL)	//No parent :(
	{
		GL_THROW("event_gen:%s must be parented to a metrics object to properly function.",hdr->name);
		/*  TROUBLESHOOT
		The event_gen object needs to be parented to a metrics object to ensure reliability metrics are being
		properly computed.  Please ensure the parent of this event_gen object is specified, and is a metrics
		object.
		*/
	}

	//See if we are a proper metrics object
	if (gl_object_isa(hdr->parent,"metrics","reliability"))
	{
		//Map it up
		metrics_obj = OBJECTDATA(hdr->parent,metrics);
	}
	else	//It isn't.  Become angry
	{
		GL_THROW("event_gen:%s must be parented to a metrics object to properly function.",hdr->name);
		//Defined above
	}

	ObjListVals = gl_find_objects(FL_GROUP,target_group);
	if (ObjListVals==NULL)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,target_group);
		/*  TROUBLESHOOT
		While attempting to populate the list of devices to test reliability on, the event_gen
		object failed to find any desired objects.  Please make sure the objects exist and try again.
		If the bug persists, please submit your code using the trac website.
		*/
	}

	//Do a zero-find check as well
	if (ObjListVals->hit_count == 0)
	{
		GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,target_group);
		//Defined above
	}

	//Pull the count
	UnreliableObjCount = ObjListVals->hit_count;

	//See how many we found and make a big ol' structural array!
	UnreliableObjs = (OBJEVENTDETAILS*)gl_malloc(UnreliableObjCount * sizeof(OBJEVENTDETAILS));

	//Make sure it worked
	if (UnreliableObjs==NULL)
	{
		GL_THROW("Failed to allocate memory for object list in %s",hdr->name);
		/*  TROUBLESHOOT
		The event_gen object failed to allocate memory for the "unreliable" object list.
		Please try again.  If the error persists, submit your code and a bug report using
		the trac website.
		*/
	}

	//Loop through and init them - can't compute exact time, but can populate array
	temp_obj = NULL;
	for (index=0; index<UnreliableObjCount; index++)
	{
		//Find the object
		temp_obj = gl_find_next(ObjListVals, temp_obj);

		if (temp_obj == NULL)
		{
			GL_THROW("Failed to populate object list in eventgen: %s",hdr->name);
			/*  TROUBLESHOOT
			While populating the reliability object vector, an object failed to be
			located.  Please try again.  If the error persists, please submit your
			code and a bug report to the trac website.
			*/
		}

		UnreliableObjs[index].obj_of_int = temp_obj;

		//Zero time for now - will get updated on next run
		UnreliableObjs[index].fail_time = 0;

		//Restoration gets TS_NEVERed for now
		UnreliableObjs[index].rest_time = TS_NEVER;

		//Populate the initial lengths though - could do later, but meh
		UnreliableObjs[index].fail_length = gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1]);

		//Convert double time to timestamp
		max_outage_length = (TIMESTAMP)(max_outage_length_dbl);

		//Find restoration time
		tempTime = gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1]);

		//If over max outage length, cap it
		if (tempTime > max_outage_length)
			UnreliableObjs[index].rest_length = max_outage_length;
		else
			UnreliableObjs[index].rest_length = tempTime;

		//Assume all start not in the fault state
		UnreliableObjs[index].in_fault = false;

		//Flag as no initial fault condition
		UnreliableObjs[index].implemented_fault = -1;

		//Flag as no customers interrupted
		UnreliableObjs[index].customers_affected = 0;
	}//end population loop

	//Free up list
	gl_free(ObjListVals);

	//Pass in parameters for statistics - for tracking
	curr_fail_dist_params[0]=fail_dist_params[0];
	curr_fail_dist_params[1]=fail_dist_params[1];
	curr_rest_dist_params[0]=rest_dist_params[0];
	curr_rest_dist_params[1]=rest_dist_params[1];
	curr_fail_dist = failure_dist;
	curr_rest_dist = restore_dist;

	//Check simultaneous fault value
	if ((max_simult_faults == -1) || (max_simult_faults > 1))	//infinite or more than 1
	{
		gl_warning("event_gen:%s has the ability to generate more than 1 simultaneous fault - metrics may not be accurate",hdr->name);
		/*  TROUBLESHOOT
		The event_gen object has the ability to induce simultaneous and concurrent fault conditions.  This may affect the accuracy
		of your reliability metrics.
		*/
	}
	else if ((max_simult_faults == 0) || (max_simult_faults < -1))
	{
		GL_THROW("event_gen:%s must have a valid maximum simultaneous fault count number",hdr->name);
		/*  TROUBLESHOOT
		Event_gen objects require the max_simultaneous_faults property to be properly defined to run.
		This must be either -1, or a number greater than or equal to 1.  -1 indicates "infinite" simultaneous
		faults, so it is conceivable that every candidate object can be a fault condition concurrently.
		*/
	}
	//defaulted else - no action needed (basically, it equals 1)

	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP eventgen::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index;
	TIMESTAMP tempTime;
	FUNCTIONADDR funadd = NULL;
	int returnval;

	//All passes start out assuming they don't need a new count update and nothing is interrupted
	curr_time_interrupted = 0;
	diff_count_needed = false;

	//Check if first run, if so, do some additional work
	if (next_event_time==0)
	{
		//Make next_event_time REALLY big
		next_event_time = TS_NEVER;
		
		//Loop through and update timevalues
		for (index=0; index<UnreliableObjCount; index++)
		{
			//Failure time
			UnreliableObjs[index].fail_time = t1 + UnreliableObjs[index].fail_length;

			//See if it is a new minimum - don't need to check fault state - first run assumes all are good
			if (UnreliableObjs[index].fail_time < next_event_time)
			{
				next_event_time = UnreliableObjs[index].fail_time;
			}
		}
	}

	//See if the distribution parameters have changed - if so, regen all faults (items in fault condition will wait until restoration to update)
	if ((curr_fail_dist_params[0]!=fail_dist_params[0]) || (curr_fail_dist_params[1]!=fail_dist_params[1]) || (curr_rest_dist_params[0]!=rest_dist_params[0]) || (curr_rest_dist_params[1]!=rest_dist_params[1]) || (curr_fail_dist!=failure_dist) || (curr_rest_dist!=restore_dist))
	{
		//Update tracking variables - easier this way
		curr_fail_dist_params[0]=fail_dist_params[0];
		curr_fail_dist_params[1]=fail_dist_params[1];
		curr_rest_dist_params[0]=rest_dist_params[0];
		curr_rest_dist_params[1]=rest_dist_params[1];
		curr_fail_dist = failure_dist;
		curr_rest_dist = restore_dist;

		//Dump us a verbose indicating this happened
		gl_verbose("Distribution parameters changed for %s",hdr->name);
		
		//Loop through the objects and handle appropriately
		
		//Reset event timer as well, since it may be invalid now
		next_event_time = TS_NEVER;

		for (index=0; index<UnreliableObjCount; index++)
		{
			if (UnreliableObjs[index].in_fault == false)	//Not faulting, so we don't care if we are now a fault or if we have one upcoming
			{
				//Update the failure and restoration length (do this now)
				UnreliableObjs[index].fail_length = gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1]);

				//Find restoration length
				tempTime = gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1]);

				//If over max outage length, cap it
				if (tempTime > max_outage_length)
					UnreliableObjs[index].rest_length = max_outage_length;
				else
					UnreliableObjs[index].rest_length = tempTime;

				//Update failure time
				UnreliableObjs[index].fail_time = t0 + UnreliableObjs[index].fail_length;

				//Check progression
				if (UnreliableObjs[index].fail_time < next_event_time)
					next_event_time = UnreliableObjs[index].fail_time;

				//Flag restoration time
				UnreliableObjs[index].rest_time = TS_NEVER;

				//De-flag the update
				UnreliableObjs[index].in_fault = false;
			}//End non-faulted object update
			else	//Implies it is in a fault - update next_event_time (if it is done, it will be handled below)
			{
				if (UnreliableObjs[index].rest_time < next_event_time)
					next_event_time = UnreliableObjs[index].rest_time;
			}
		}//End failed objects traversion
	}//end distribution parameter change

	//See if it is time for an event
	if (t0 >= next_event_time)	//It is!
	{
		//Reset next event time - we'll find the new one in here
		next_event_time = TS_NEVER;

		//Loop through and find events that are next
		for (index=0; index<UnreliableObjCount; index++)
		{
			//Check failure time
			if ((UnreliableObjs[index].fail_time <= t0) && (UnreliableObjs[index].in_fault == false))	//Failure!
			{
				//See if we're allowed to fault
				if ((faults_in_prog < max_simult_faults) || (max_simult_faults == -1))	//Room to fault or infinite amount
				{
					//See if something else has already asked for a count update
					if (diff_count_needed == false)
					{
						curr_time_interrupted = metrics_obj->get_interrupted_count();	//Get the count of currently interrupted objects
						diff_count_needed = true;										//Flag us for an update
					}

					//Put a fault on the system
					funadd = (FUNCTIONADDR)(gl_get_function(UnreliableObjs[index].obj_of_int,"create_fault"));
					
					//Make sure it was found
					if (funadd == NULL)
					{
						GL_THROW("Unable to induce event on %s",UnreliableObjs[index].obj_of_int->name);
						/*  TROUBLESHOOT
						While attempting to create or restore a fault, the eventgen failed to find the fault-inducing or fault-fixing
						function on the object of interest.  Ensure this object type supports faulting and try again.  If the problem
						persists, please submit a bug report and your code to the trac website.
						*/
					}

					returnval = ((int (*)(OBJECT *, char *, int *))(*funadd))(UnreliableObjs[index].obj_of_int,fault_type,&UnreliableObjs[index].implemented_fault);

					if (returnval == 0)	//Failed :(
					{
						GL_THROW("Failed to induce event on %s",UnreliableObjs[index].obj_of_int->name);
						/*  TROUBLESHOOT
						Eventgen attempted to induce a fault on the object, but the object failed to induce the fault.
						Check the object's create_fault code.  If necessary, submit your code and a bug report using the
						trac website.
						*/
					}

					//Bring the restoration time to bear
					UnreliableObjs[index].rest_time = t0 + UnreliableObjs[index].rest_length;

					//See if this is the new update
					if (UnreliableObjs[index].rest_time < next_event_time)
						next_event_time = UnreliableObjs[index].rest_time;

					//Flag outage time so it won't trip things
					UnreliableObjs[index].in_fault = true;

					//Flag customer count to know we need to populate this value
					UnreliableObjs[index].customers_affected = -1;

					//Increment the simultaneous fault counter (prevents later objects from faulting if they are ready)
					faults_in_prog++;
				}
				else	//Fault limit has been hit, give us a new time to play
				{
					//Update the failure and restoration length (do this now)
					UnreliableObjs[index].fail_length = gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1]);

					//Update failure time
					UnreliableObjs[index].fail_time = t0 + UnreliableObjs[index].fail_length;

					//Check progression
					if (UnreliableObjs[index].fail_time < next_event_time)
						next_event_time = UnreliableObjs[index].fail_time;
				}
			}
			else if (UnreliableObjs[index].rest_time <= t0)	//Restoration time!
			{
				//Call the object back into service
				funadd = (FUNCTIONADDR)(gl_get_function(UnreliableObjs[index].obj_of_int,"fix_fault"));
				
				//Make sure it was found
				if (funadd == NULL)
				{
					GL_THROW("Unable to induce event on %s",UnreliableObjs[index].obj_of_int->name);
					//Defined above
				}

				returnval = ((int (*)(OBJECT *, int *))(*funadd))(UnreliableObjs[index].obj_of_int,&UnreliableObjs[index].implemented_fault);

				if (returnval == 0)	//Restoration is no go :(
				{
					GL_THROW("Failed to induce repair on %s",UnreliableObjs[index].obj_of_int->name);
					/*  TROUBLESHOOT
					Eventgen attempted to induce a repair on the object, but the object failed to perform the repair.
					Check the object's fix_fault code.  If necessary, submit your code and a bug report using the
					trac website.
					*/
				}

				//Call the event updater
				metrics_obj->event_ended(hdr,UnreliableObjs[index].obj_of_int,UnreliableObjs[index].fail_time,UnreliableObjs[index].rest_time,fault_type,UnreliableObjs[index].customers_affected);

				//Update the failure and restoration length (do this now)
				UnreliableObjs[index].fail_length = gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1]);

				//Find restoration length
				tempTime = gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1]);

				//If over max outage length, cap it
				if (tempTime > max_outage_length)
					UnreliableObjs[index].rest_length = max_outage_length;
				else
					UnreliableObjs[index].rest_length = tempTime;

				//Update failure time
				UnreliableObjs[index].fail_time = t0 + UnreliableObjs[index].fail_length;

				//Check progression
				if (UnreliableObjs[index].fail_time < next_event_time)
					next_event_time = UnreliableObjs[index].fail_time;

				//Flag restoration time
				UnreliableObjs[index].rest_time = TS_NEVER;

				//De-flag the update
				UnreliableObjs[index].in_fault = false;

				//Decrement us out of the simultaneous fault count (allows possibility of later object in list to fault before count updated)
				faults_in_prog--;
			}
			else	//Not either - see where we sit against the max
			{
				if ((UnreliableObjs[index].fail_time < next_event_time) && (UnreliableObjs[index].in_fault == false))		//Failure first
				{
					next_event_time = UnreliableObjs[index].fail_time;
				}
				else if (UnreliableObjs[index].rest_time < next_event_time)	//Restoration
				{
					next_event_time = UnreliableObjs[index].rest_time;
				}
				//Defaulted else - we're bigger than the current min
			}
		}//End object loop traversion

		//Reset and update our current fault counter (just to ensure things are accurate)
		faults_in_prog = 0;
		
		//Loop through objects and increment as necessary
		for (index=0; index<UnreliableObjCount;index++)
		{
			if (UnreliableObjs[index].in_fault == true)
				faults_in_prog++;
		}
	}//end time for another event

	//See where we're going
	if (next_event_time == TS_NEVER)
	{
		return TS_NEVER;
	}
	else	//Must be valid-ish
	{
		return -next_event_time;	//Negative return, we'd like to go there if we're going that way
	}
}

TIMESTAMP eventgen::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	int after_count, differential_count, index;

	//See if we need a "post-fault" count - assumes all customers will determine their outage state by either presync or sync (or before this in postsync)
	if (diff_count_needed == true)
	{
		//Get current number of customers out of service
		after_count = metrics_obj->get_interrupted_count();

		//Figure out the differential
		differential_count = after_count - curr_time_interrupted;

		//See how it sits - if it is negative, toss a warning and just use the after_count value
		if (differential_count < 0)	//Must be positive or zero, otherwise things were restored
		{
			//Just use the after count - some unexpected restorations occurred
			differential_count = after_count;
		}

		//Apply the update to objects needing it
		for (index=0; index<UnreliableObjCount; index++)
		{
			if (UnreliableObjs[index].customers_affected == -1)	//We need it
				UnreliableObjs[index].customers_affected = differential_count;
		}
	}//If differntial count needed

	return TS_NEVER;	//We always want to go forever
}

//Function to do random time generation - functionalized for ease
TIMESTAMP eventgen::gen_random_time(DISTTYPE rand_dist_type, double param_1, double param_2)
{
	TIMESTAMP random_time = 0;
	double dbl_random_time = 0.0;
	OBJECT *obj = OBJECTHDR(this);

	switch(rand_dist_type)
	{
		case UNIFORM:
			{
				dbl_random_time = gl_random_uniform(param_1,param_2);
				break;
			}
		case NORMAL:
			{
				dbl_random_time = gl_random_normal(param_1,param_2);
				break;
			}
		case LOGNORMAL:
			{
				dbl_random_time = gl_random_lognormal(param_1,param_2);
				break;
			}
		case BERNOULLI:
			{
				dbl_random_time = gl_random_bernoulli(param_1);
				break;
			}
		case PARETO:
			{
				dbl_random_time = gl_random_pareto(param_1,param_2);
				break;
			}
		case EXPONENTIAL:
			{
				dbl_random_time = gl_random_exponential(param_1);
				break;
			}
		case RAYLEIGH:
			{
				dbl_random_time = gl_random_rayleigh(param_1);
				break;
			}
		case WEIBULL:
			{
				dbl_random_time = gl_random_weibull(param_1,param_2);
				break;
			}
		case GAMMA:
			{
				dbl_random_time = gl_random_gamma(param_1,param_2);
				break;
			}
		case BETA:
			{
				dbl_random_time = gl_random_beta(param_1, param_2);
				break;
			}
		case TRIANGLE:
			{
				dbl_random_time = gl_random_triangle(param_1, param_2);
				break;
			}
		default:
			{
				GL_THROW("Undefined distribution in eventgen %s",obj->name);
				/*  TROUBLESHOOT
				While creating a random failure or restoration time, the eventgen object
				encountered an unspecified random distribution.  Please check the values of
				failure_dist and restoration_dist and try again.
				*/
			}
	}

	//Cast it
	random_time = (int64)(dbl_random_time);

	return random_time;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_eventgen(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(eventgen::oclass);
		if (*obj!=NULL)
		{
			eventgen *my = OBJECTDATA(*obj,eventgen);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_eventgen: %s", msg);

	}
	return 1;
}

EXPORT int init_eventgen(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,eventgen)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_eventgen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_eventgen(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	eventgen *my = OBJECTDATA(obj,eventgen);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			return my->presync(obj->clock,t1);
			break;
		case PC_BOTTOMUP:
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
		gl_error("sync_eventgen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}
/**@}*/
