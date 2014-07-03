/** $Id: eventgen.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
#include "gridlabd.h"
#include "eventgen.h"

#define TSNVRDBL 9223372036854775808.0

CLASS *eventgen::oclass = NULL;			/**< a pointer to the CLASS definition in GridLAB-D's core */
eventgen *eventgen::defaults = NULL;	/**< a pointer to the default values used when creating new objects */

/* Class registration is only called once to register the class with the core */
eventgen::eventgen(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"eventgen",sizeof(eventgen),PC_PRETOPDOWN|PC_POSTTOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class eventgen";
		else
			oclass->trl = TRL_DEMONSTRATED;

		if (gl_publish_variable(oclass,
			PT_char1024, "target_group", PADDR(target_group),
			PT_char256, "fault_type", PADDR(fault_type),
			PT_enumeration, "failure_dist", PADDR(failure_dist),
				PT_KEYWORD, "UNIFORM", (enumeration)UNIFORM,
				PT_KEYWORD, "NORMAL", (enumeration)NORMAL,
				PT_KEYWORD, "LOGNORMAL", (enumeration)LOGNORMAL,
				PT_KEYWORD, "BERNOULLI", (enumeration)BERNOULLI,
				PT_KEYWORD, "PARETO", (enumeration)PARETO,
				PT_KEYWORD, "EXPONENTIAL", (enumeration)EXPONENTIAL,
				PT_KEYWORD, "RAYLEIGH", (enumeration)RAYLEIGH,
				PT_KEYWORD, "WEIBULL", (enumeration)WEIBULL,
				PT_KEYWORD, "GAMMA", (enumeration)GAMMA,
				PT_KEYWORD, "BETA", (enumeration)BETA,
				PT_KEYWORD, "TRIANGLE", (enumeration)TRIANGLE,
			PT_enumeration, "restoration_dist", PADDR(restore_dist),
				PT_KEYWORD, "UNIFORM", (enumeration)UNIFORM,
				PT_KEYWORD, "NORMAL", (enumeration)NORMAL,
				PT_KEYWORD, "LOGNORMAL", (enumeration)LOGNORMAL,
				PT_KEYWORD, "BERNOULLI", (enumeration)BERNOULLI,
				PT_KEYWORD, "PARETO", (enumeration)PARETO,
				PT_KEYWORD, "EXPONENTIAL", (enumeration)EXPONENTIAL,
				PT_KEYWORD, "RAYLEIGH", (enumeration)RAYLEIGH,
				PT_KEYWORD, "WEIBULL", (enumeration)WEIBULL,
				PT_KEYWORD, "GAMMA", (enumeration)GAMMA,
				PT_KEYWORD, "BETA", (enumeration)BETA,
				PT_KEYWORD, "TRIANGLE", (enumeration)TRIANGLE,
			PT_double, "failure_dist_param_1", PADDR(fail_dist_params[0]),
			PT_double, "failure_dist_param_2", PADDR(fail_dist_params[1]),
			PT_double, "restoration_dist_param_1", PADDR(rest_dist_params[0]),
			PT_double, "restoration_dist_param_2", PADDR(rest_dist_params[1]),
			PT_char1024, "manual_outages", PADDR(manual_fault_list),
			PT_double, "max_outage_length[s]", PADDR(max_outage_length_dbl),
			PT_int32, "max_simultaneous_faults", PADDR(max_simult_faults),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			if (gl_publish_function(oclass,	"add_event", (FUNCTIONADDR)add_event)==NULL)
				GL_THROW("Unable to publish reliability event adding function");
			if (gl_publish_function(oclass,	"interupdate_event_object", (FUNCTIONADDR)interupdate_eventgen)==NULL)
				GL_THROW("Unable to publish reliability deltamode function");
	}
}

/* Object creation is called once for each object that is created by the core */
int eventgen::create(void)
{
	target_group[0] = '\0';
	fault_type[0] = '\0';
	manual_fault_list[0] = '\0';

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
	metrics_obj_hdr = NULL;

	max_outage_length_dbl = 432000.0;	//5 day maximum outage by default

	next_event_time = 0;
	next_event_time_dbl = 0.0;

	max_simult_faults = -1;	//-1 = no limit to number of simult faults
	faults_in_prog = 0;		//Assume we start with no faults

	fault_implement_mode = false;	//Assume by default that we are in randomized fault mode

	curr_time_interrupted = 0;
	curr_time_interrupted_sec = 0;
	diff_count_needed = false;

	secondary_interruption_cnt = NULL;

	//linked list starts empty
	Unhandled_Events.prev = NULL;
	Unhandled_Events.next = NULL;

	//minimum timestep junk
	glob_min_timestep = 0.0;
	off_nominal_time = false;

	//Delta-related items
	deltamode_inclusive=false;		//Not in deltamode by default

	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int eventgen::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index, comma_count;
	TIMESTAMP tempTime, globStartTimeVal;
	FINDLIST *ObjListVals;
	OBJECT *temp_obj;
	double temp_double, temp_val, temp_time_double;
	char *token_a, *token_a1;
	TIMESTAMP temp_time_A, temp_time_B;
	unsigned int temp_time_A_nano, temp_time_B_nano;
	double temp_time_A_dbl, temp_time_B_dbl;
	char temp_buff[128];

	//Get global_minimum_timestep value and set the appropriate flag
	//Retrieve the global value, only does so as a text string for some reason
	gl_global_getvar("minimum_timestep",temp_buff,sizeof(temp_buff));

	//Initialize our parsing variables
	index = 0;

	//Loop through the buffer
	while ((index < 128) && (temp_buff[index] != '\0'))
	{
		glob_min_timestep *= 10;					//Adjust previous iteration value
		temp_val = (double)(temp_buff[index]-48);	//Decode the ASCII
		glob_min_timestep += temp_val;				//Accumulate it

		index++;									//Increment the index
	}

	if (glob_min_timestep > 1)					//Now check us
	{
		off_nominal_time=true;					//Set flag
		gl_warning("eventgen:%s - minimum_timestep set - all events at least 1 minimum step long",hdr->name);
		/*  TROUBLESHOOT
		A minimum timestep value is set in the file.  Reliability works not-so-well when the events
		are starting and stopping less than this value.  All events are therefore going to be set
		to be at least 1 minimum timestep long.  This may alter your results, so please plan appropriately.
		*/
	}
	//Extract starting time - for comparision
	gl_global_getvar("clock",temp_buff,sizeof(temp_buff));

	//Convert it
	globStartTimeVal = gl_parsetime(temp_buff);

	//If a minimum timestep is present, make sure things are set appropriately
	if (off_nominal_time == true)
	{
		if (max_outage_length_dbl < glob_min_timestep)
			max_outage_length_dbl = glob_min_timestep;	//Set to at least one

		if (event_max_duration < glob_min_timestep)
			event_max_duration = glob_min_timestep;		//Set to at least one
	}

	//See if we're a deltamode-enabled device
	if ((hdr->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		if (enable_subsecond_models == true)
		{
			deltamode_inclusive = true;	//Set the flag for further checks
		}
		else	//Deltamode not enabled for reliability
		{
			gl_warning("Eventgen:%s is flagged for deltamode, but reliability deltamode is not enabled",hdr->name);
			/*  TROUBLESHOOT
			The deltamode flag was set for the particular eventgen object, but the enabled_subsecond_models flag for the
			entire reliability module is not set.  Deltamode support is not enabled.  If this is not desired, be sure to set the
			module-level variable enabled_subsecond_models to true and try again.
			*/
		}
	}

	//Secondary check -- theoretically core would catch and complain about this, but do it here too
	if ((deltamode_inclusive == true) && (off_nominal_time == true))
	{
		GL_THROW("event_gen:%s is flagged for deltamode, but minimum timesteps are enabled!",hdr->name);
		/*  TROUBLESHOOT
		An event_gen object is flagged for deltamode, but the simulation also has minimum timesteps enabled.
		This is not supported at this time.  Please disable one or the other to continue.
		*/
	}

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

		//Map general header too, for locking
		metrics_obj_hdr = hdr->parent;
	}
	else	//It isn't.  Become angry
	{
		GL_THROW("event_gen:%s must be parented to a metrics object to properly function.",hdr->name);
		//Defined above
	}

	//Flag the secondary count variable - find its location
	secondary_interruption_cnt = &metrics_obj->secondary_interruptions_count;

	//Populate the maximum length variable
	//Convert double time to timestamp
	max_outage_length = (TIMESTAMP)(max_outage_length_dbl);

	//And convert it back - maximum outage is being forced to whole seconds, just because
	max_outage_length_dbl = (double)(max_outage_length);

	//Determine how we should proceed - are we group-finding objects, or list-finding objects?
	if (target_group[0] == '\0')	//Empty group list implies that manual list mode is desired (programmer defined. i.e., I say so)
	{
		fault_implement_mode = true;	//Flag as manually populated list mode

		//Count the number of commas in the list - use that as an initial basis for validity
		index=0;
		comma_count=1;
		while ((manual_fault_list[index] != '\0') && (index < 1024))
		{
			if (manual_fault_list[index] == ',')	//Comma
				comma_count++;						//increment the number of commas found

			index++;	//increment the pointer
		}

		//See if one is really there - if ends early
		if ((comma_count == 1) && (manual_fault_list[0] == '\0'))	//Is empty :(
		{
			GL_THROW("Manual event generation specified, but no events listed in event_gen:%s",hdr->name);
			/*  TROUBLESHOOT
			An eventgen object was set to manual mode, but no list of objects and fault times was found.  If
			target_group is left blank, specify objects and appropriate times using the manual_outages property.
			*/
		}

		//Assuming it was more than one, make sure the count is in 3s (object, fault time, rest time) before proceeding
		UnreliableObjCount = (int)(comma_count / 3);
		temp_double = (double)(comma_count)/3.0 - (double)(UnreliableObjCount);
		
		if (temp_double != 0.0)	//odd amount, a comma error has occurred
		{
			GL_THROW("event_gen:%s has an invalid manual list",hdr->name);
			/*  TROUBLESHOOT
			The manual list (manual_outages) is improperly specified.  The list must occur in sets of three
			comma separated values (object name, fault time, restoration time).  A comma is missing, or an extra
			comma is in place somewhere.  Please fix this to proceed.
			*/
		}

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
		
		//Loop through the count and try parsing out what everything is - do individual error checks
		token_a = manual_fault_list;
		for (index=0; index<UnreliableObjCount; index++)
		{
			//First item is the object, grab it
			token_a1 = obj_token(token_a, &temp_obj);
			
			//Make sure it is valid
			if (temp_obj == NULL)
			{
				if (token_a1 != NULL)
				{
					//Remove the comma from the list
					*--token_a1 = '\0';
				}
				//Else is the end of list, so it should already be \0-ed

				GL_THROW("eventgen:%s: fault object %s was not found!",hdr->name,token_a);
				/*  TROUBLESHOOT
				While parsing the manual fault list for the evengen object, a fault point
				was not found.  Please check the name and try again.  If the problem persists, please submit 
				your code and a bug report via the trac website.
				*/
			}
			
			//Update the pointer
			token_a = token_a1;

			//Fault time
			token_a1 = time_token(token_a,&temp_time_A,&temp_time_A_nano,&temp_time_A_dbl);	//Pull next - starts on obj, we want to do a count

			token_a = token_a1;	//Update the pointer

			//Restoration time
			token_a1 = time_token(token_a,&temp_time_B,&temp_time_B_nano,&temp_time_B_dbl);	//Pull next - starts on obj, we want to do a count

			token_a = token_a1;	//Update the pointer

			//Store the object
			UnreliableObjs[index].obj_of_int = temp_obj;

			//Ensure the link to the protective device is NULLed
			UnreliableObjs[index].obj_made_int = NULL;

			//Check to make sure failures start AFTER the simulation has started
			if (temp_time_A > globStartTimeVal)
			{
				//Store failure time
				UnreliableObjs[index].fail_time = temp_time_A;
				UnreliableObjs[index].fail_time_ns = temp_time_A_nano;
				UnreliableObjs[index].fail_time_dbl = temp_time_A_dbl;
			}
			else	//Nope, give an error
			{
				GL_THROW("Manual event time in %s must be AFTER the simulation start time",hdr->name);
				/*  TROUBLESHOOT
				A manual event specified into eventgen has a start time before the simulation has actually started.
				Please fix this and try again.
				*/
			}

			//Store restoration time
			UnreliableObjs[index].rest_time = temp_time_B;
			UnreliableObjs[index].rest_time_ns = temp_time_B_nano;
			UnreliableObjs[index].rest_time_dbl = temp_time_B_dbl;

			//Populate the initial lengths - set fault to zero
			UnreliableObjs[index].fail_length = 0;

			if (deltamode_inclusive == true)
			{
				//Find restoration time
				temp_time_double = temp_time_B_dbl - temp_time_A_dbl;

				//If over max outage length, warn here (it's manual mode, so let user shoot themselves in the foot)
				if (temp_time_double > max_outage_length_dbl)
				{
					gl_warning("outage length for object:%s in eventgen:%s exceeds the defined maximum outage.",temp_obj->name,hdr->name);
					//Defined below
				}

				//Store the value - just for the sake of doing so
				UnreliableObjs[index].rest_length_dbl = temp_time_double;
				UnreliableObjs[index].rest_length = (TIMESTAMP)(temp_time_double);
				UnreliableObjs[index].rest_length_ns = (unsigned int)(1.0e9*(temp_time_double - (double)(UnreliableObjs[index].rest_length))+0.5);
			}
			else	//"Traditional" operation
			{
				//Find restoration time
				tempTime = temp_time_B - temp_time_A;

				//If over max outage length, warn here (it's manual mode, so let user shoot themselves in the foot)
				if (tempTime > max_outage_length)
				{
					gl_warning("outage length for object:%s in eventgen:%s exceeds the defined maximum outage.",temp_obj->name,hdr->name);
					/*  TROUBLESHOOT
					An outage length specified for the manual event generation mode is longer than the value defined in max_outage_length.
					If this is not desired, fix the input.  If it is acceptable, the simulation will run normally.
					*/
				}

				//Make sure it is long enough - if not, force it to be
				if ((off_nominal_time == true) && (((double)(tempTime)) < glob_min_timestep))
				{
					//Force to be at least one
					tempTime = (TIMESTAMP)(glob_min_timestep);

					//Update the restoration time appropriately
					UnreliableObjs[index].rest_time = UnreliableObjs[index].fail_time + tempTime;

					//Toss a warning, for giggles
					gl_warning("Outage length for object:%s is less than the minimum timestep of %.0f seconds, rounded to minimum timestep",temp_obj->name,glob_min_timestep);
					/*  TROUBLESHOOT
					The selected outage length is less than the mininum timestep set.  It has been forced to be at least one
					minimum timestep long.
					*/
				}

				//Store the value - just for the sake of doing so
				UnreliableObjs[index].rest_length = tempTime;
			}

			//Assume all start not in the fault state
			UnreliableObjs[index].in_fault = false;

			//Flag as no initial fault condition
			UnreliableObjs[index].implemented_fault = -1;

			//Flag as no customers interrupted
			UnreliableObjs[index].customers_affected = 0;

			//Flag as no customers secondarily interrupted
			UnreliableObjs[index].customers_affected_sec = 0;
		}//end population loop

		//Pass in parameters for statistics - for tracking - put as 0's and "none" to flag out
		curr_fail_dist_params[0]=0;
		curr_fail_dist_params[1]=0;
		curr_rest_dist_params[0]=0;
		curr_rest_dist_params[1]=0;
		curr_fail_dist = NONE;
		curr_rest_dist = NONE;

	}
	else	//Random mode, proceed with that assumption
	{

		ObjListVals = gl_find_objects(FL_GROUP,target_group.get_string());
		if (ObjListVals==NULL)
		{
			GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,target_group.get_string());
			/*  TROUBLESHOOT
			While attempting to populate the list of devices to test reliability on, the event_gen
			object failed to find any desired objects.  Please make sure the objects exist and try again.
			If the bug persists, please submit your code using the trac website.
			*/
		}

		//Do a zero-find check as well
		if (ObjListVals->hit_count == 0)
		{
			GL_THROW("Failure to find devices for %s specified as: %s",hdr->name,target_group.get_string());
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
			//Defined above
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

			//Just set object causing an action to NULL for now
			UnreliableObjs[index].obj_made_int = NULL;

			//Zero time for now - will get updated on next run
			UnreliableObjs[index].fail_time = 0;

			//Restoration gets TS_NEVERed for now
			UnreliableObjs[index].rest_time = TS_NEVER;
			UnreliableObjs[index].rest_time_ns = 0;
			UnreliableObjs[index].rest_time_dbl = TSNVRDBL;

			//Populate the initial lengths though - could do later, but meh
			gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1],&UnreliableObjs[index].fail_length,&UnreliableObjs[index].fail_length_ns,&UnreliableObjs[index].fail_length_dbl);

			//Find restoration time
			gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1],&temp_time_A,&temp_time_A_nano,&temp_time_A_dbl);

			//If over max outage length, cap it - side note - minimum timestep issues handled inside gen_random_time
			if (temp_time_A_dbl > max_outage_length_dbl)
			{
				UnreliableObjs[index].rest_length = max_outage_length;
				UnreliableObjs[index].rest_length_ns = 0;
				UnreliableObjs[index].rest_length_dbl = max_outage_length_dbl;
			}
			else
			{
				UnreliableObjs[index].rest_length = temp_time_A;
				UnreliableObjs[index].rest_length_ns = temp_time_A_nano;
				UnreliableObjs[index].rest_length_dbl = temp_time_A_dbl;
			}

			//Assume all start not in the fault state
			UnreliableObjs[index].in_fault = false;

			//Flag as no initial fault condition
			UnreliableObjs[index].implemented_fault = -1;

			//Flag as no customers interrupted
			UnreliableObjs[index].customers_affected = 0;

			//Flag as no customers secondarily interrupted
			UnreliableObjs[index].customers_affected_sec = 0;
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
	}	//End randomized fault mode

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

	//See if we desire a deltamode update (module-level)
	if (deltamode_inclusive)
	{
		//Check global, for giggles
		if (enable_subsecond_models!=true)
		{
			gl_warning("eventgen:%s indicates it wants to run deltamode, but the module-level flag is not set!",hdr->name?hdr->name:"unnamed");
			/*  TROUBLESHOOT
			The eventgen object has the deltamode_inclusive flag set, but not the module-level enable_subsecond_models flag.  The event
			will not simulate any dynamics this way.
			*/
		}
		else
		{
			eventgen_object_count++;	//Increment the counter
		}
	}

	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP eventgen::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *hdr = OBJECTHDR(this);
	int index;
	double t1_dbl;

	//Cast time for any "deltamode-needed" calculations
	t1_dbl = (double)t1;

	//All passes start out assuming they don't need a new count update and nothing is interrupted
	curr_time_interrupted = 0;
	curr_time_interrupted_sec = 0;
	diff_count_needed = false;

	//Check if first run, if so, do some additional work
	if (next_event_time==0)
	{
		//Make next_event_time REALLY big
		next_event_time = TS_NEVER;
		next_event_time_dbl = TSNVRDBL;
		
		//Loop through and update timevalues
		for (index=0; index<UnreliableObjCount; index++)
		{
			//Failure time - only needs to be computed if "random" mode
			if (fault_implement_mode == false)
			{
				//Deltamode check - handle times "traditionally" or not
				if (deltamode_inclusive == true)
				{
					UnreliableObjs[index].fail_time_dbl = t1_dbl + UnreliableObjs[index].fail_length_dbl;
					UnreliableObjs[index].fail_time = (TIMESTAMP)(floor(UnreliableObjs[index].fail_time_dbl));
					UnreliableObjs[index].fail_time_ns = (unsigned int)((UnreliableObjs[index].fail_time_dbl - (double)(UnreliableObjs[index].fail_time))*1.0e9 + 0.5);
				}
				else	//Nope
				{
					UnreliableObjs[index].fail_time = t1 + UnreliableObjs[index].fail_length;
				}
			}

			//See if it is a new minimum - don't need to check fault state - first run assumes all are good
			if (UnreliableObjs[index].fail_time < next_event_time)
			{
				next_event_time = UnreliableObjs[index].fail_time;
				if (deltamode_inclusive == true)
					next_event_time_dbl = UnreliableObjs[index].fail_time_dbl;
			}
		}

		//Linked list is ignored on this first run - it will get caught as part of the normal routine
		if (deltamode_inclusive && enable_subsecond_models)	//We want deltamode - see if it's populated yet
		{
			if ((eventgen_object_current == -1) || (delta_objects==NULL))
			{
				//Allocate the deltamode object array
				delta_objects = (OBJECT**)gl_malloc(eventgen_object_count*sizeof(OBJECT*));

				//Make sure it worked
				if (delta_objects == NULL)
				{
					GL_THROW("Failed to allocate deltamode objects array for reliability module!");
					/*  TROUBLESHOOT
					While attempting to create a reference array for reliability module deltamode-enabled
					objects, an error was encountered.  Please try again.  If the error persists, please
					submit your code and a bug report via the trac website.
					*/
				}

				//Allocate the function reference list as well
				delta_functions = (FUNCTIONADDR*)gl_malloc(eventgen_object_count*sizeof(FUNCTIONADDR));

				//Make sure it worked
				if (delta_functions == NULL)
				{
					GL_THROW("Failed to allocate deltamode objects function array for reliability module!");
					/*  TROUBLESHOOT
					While attempting to create a reference array for reliability module deltamode-enabled
					objects, an error was encountered.  Please try again.  If the error persists, please
					submit your code and a bug report via the trac website.
					*/
				}

				//Initialize index
				eventgen_object_current = 0;
			}

			//Check limits of the array
			if (eventgen_object_current>=eventgen_object_count)
			{
				GL_THROW("Too many objects tried to populate deltamode objects array in the reliability module!");
				/*  TROUBLESHOOT
				While attempting to populate a reference array of deltamode-enabled objects for the reliability
				module, an attempt was made to write beyond the allocated array space.  Please try again.  If the
				error persists, please submit a bug report and your code via the trac website.
				*/
			}

			//Add us into the list
			delta_objects[eventgen_object_current] = hdr;

			//Map up the function for interupdate
			delta_functions[eventgen_object_current] = (FUNCTIONADDR)(gl_get_function(hdr,"interupdate_event_object"));

			//Make sure it worked
			if (delta_functions[eventgen_object_current] == NULL)
			{
				GL_THROW("Failure to map deltamode function for device:%s",hdr->name);
				/*  TROUBLESHOOT
				Attempts to map up the interupdate function of a specific device failed.  Please try again and ensure
				the object supports deltamode.  If the error persists, please submit your code and a bug report via the
				trac website.
				*/
			}

			//Update pointer
			eventgen_object_current++;

		}
	}

	//If the next time point is the whole second right before the time for a deltamode event, we need to schedule to enter deltamode.
	if ((next_event_time == t1) && deltamode_inclusive) 
	{
		schedule_deltamode_start(next_event_time);
		return -next_event_time; //Return here and we will do all the work in deltamode
	}

	//See if the event times need to be updated
	regen_events(t1,t1_dbl);

	if(t1 >= next_event_time) //Time for an event in supersecond
	{
		//Parse event list and execute any that are occurring
		do_event(t1,t1_dbl,false);
	}

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
	int after_count, after_count_sec, differential_count, differential_count_sec, index;
	RELEVANTSTRUCT *temp_struct;

	//See if we need a "post-fault" count - assumes all customers will determine their outage state by either presync or sync (or before this in postsync)
	if (diff_count_needed == true)
	{
		if (*secondary_interruption_cnt == true)
		{
			//Get current number of customers out of service
			metrics_obj->get_interrupted_count_secondary(&after_count,&after_count_sec);

			//Figure out the differential
			differential_count = after_count - curr_time_interrupted;
			differential_count_sec = after_count_sec - curr_time_interrupted_sec;

			//See how it sits - if it is negative, toss a warning and just use the after_count value
			if (differential_count < 0)	//Must be positive or zero, otherwise things were restored
			{
				gl_warning("Afflicted object count went negative - using after fault count");
				/*  TROUBLESHOOT
				While determining the number of objects faulted by an event, a negative count was encountered.
				This indicates some objects magically restored themselves before the final count could be obtained.  This
				may be a logic fault in the code, or the event generation logic.  Pleas submit your code and a bug report
				using the trac website.
				*/

				//Just use the after count - some unexpected restorations occurred
				differential_count = after_count;
			}

			//Do the same for the other
			//See how it sits - if it is negative, toss a warning and just use the after_count value
			if (differential_count_sec < 0)	//Must be positive or zero, otherwise things were restored
			{
				gl_warning("Afflicted secondary object count went negative - using after fault count");
				/*  TROUBLESHOOT
				While determining the number of objects secondarily faulted by an event, a negative count was encountered.
				This indicates some objects magically restored themselves before the final count could be obtained.  This
				may be a logic fault in the code, or the event generation logic.  Pleas submit your code and a bug report
				using the trac website.
				*/

				//Just use the after count - some unexpected restorations occurred
				differential_count_sec = after_count_sec;
			}

			//Apply the update to objects needing it
			for (index=0; index<UnreliableObjCount; index++)
			{
				if (UnreliableObjs[index].customers_affected == -1)	//We need it
					UnreliableObjs[index].customers_affected = differential_count;

				if (UnreliableObjs[index].customers_affected_sec == -1)	//We need it
					UnreliableObjs[index].customers_affected_sec = differential_count_sec;
			}

			//Check the linked list as well
			if (Unhandled_Events.next != NULL)	//Something is in there!
			{
				//Copy the pointer
				temp_struct = &Unhandled_Events;

				//Inward we go
				while (temp_struct->next != NULL)
				{
					//Inward
					temp_struct = temp_struct->next;

					if (temp_struct->objdetails.customers_affected == -1)	//We need it
						temp_struct->objdetails.customers_affected = differential_count;

					if (temp_struct->objdetails.customers_affected_sec == -1)	//We need it
						temp_struct->objdetails.customers_affected_sec = differential_count_sec;
				}//End LL while
			}//end LL if
		}
		else	//No secondaries
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

			//Check the linked list as well
			if (Unhandled_Events.next != NULL)	//Something is in there!
			{
				//Copy the pointer
				temp_struct = &Unhandled_Events;

				//Inward we go
				while (temp_struct->next != NULL)
				{
					//Inward
					temp_struct = temp_struct->next;

					if (temp_struct->objdetails.customers_affected == -1)	//We need it
						temp_struct->objdetails.customers_affected = differential_count;
				}//End LL while
			}//end LL if
		}
	}//If differntial count needed

	return TS_NEVER;	//We always want to go forever
}

//Function to do random time generation - functionalized for ease
void eventgen::gen_random_time(enumeration rand_dist_type, double param_1, double param_2, TIMESTAMP *event_time, unsigned int *event_nanoseconds, double *event_double)
{
	TIMESTAMP random_time = 0;
	double dbl_random_time = 0.0;
	unsigned int ns_random_time = 0;
	OBJECT *obj = OBJECTHDR(this);

	switch(rand_dist_type)
	{
		case UNIFORM:
			{
				dbl_random_time = gl_random_uniform(RNGSTATE,param_1,param_2);
				break;
			}
		case NORMAL:
			{
				dbl_random_time = gl_random_normal(RNGSTATE,param_1,param_2);
				break;
			}
		case LOGNORMAL:
			{
				dbl_random_time = gl_random_lognormal(RNGSTATE,param_1,param_2);
				break;
			}
		case BERNOULLI:
			{
				dbl_random_time = gl_random_bernoulli(RNGSTATE,param_1);
				break;
			}
		case PARETO:
			{
				dbl_random_time = gl_random_pareto(RNGSTATE,param_1,param_2);
				break;
			}
		case EXPONENTIAL:
			{
				dbl_random_time = gl_random_exponential(RNGSTATE,param_1);
				break;
			}
		case RAYLEIGH:
			{
				dbl_random_time = gl_random_rayleigh(RNGSTATE,param_1);
				break;
			}
		case WEIBULL:
			{
				dbl_random_time = gl_random_weibull(RNGSTATE,param_1,param_2);
				break;
			}
		case GAMMA:
			{
				dbl_random_time = gl_random_gamma(RNGSTATE,param_1,param_2);
				break;
			}
		case BETA:
			{
				dbl_random_time = gl_random_beta(RNGSTATE,param_1, param_2);
				break;
			}
		case TRIANGLE:
			{
				dbl_random_time = gl_random_triangle(RNGSTATE,param_1, param_2);
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

	//Round it - if necessary
	if ((off_nominal_time == true) && (dbl_random_time < glob_min_timestep)) 
	{
		dbl_random_time = glob_min_timestep;	//Make it at least 1

		//Toss a warning, for giggles
		gl_warning("Outage length is less than the minimum timestep of %.0f seconds, rounded to minimum timestep",glob_min_timestep);
		//Defined above
	}

	//Cast the whole seconds - probably needs the half, but it's random, so meh
	random_time = (int64)(dbl_random_time);

	//Assign the different sections
	*event_time = random_time;

	//See if we support delta mode
	if (deltamode_inclusive == true)
	{
		//Strip off the nanosecond portion
		ns_random_time = (unsigned int)((dbl_random_time - (double)(random_time))*1e9+0.5);

		//Assign the double value
		*event_double = dbl_random_time;

		//Assign the nanoseconds portion
		*event_nanoseconds = ns_random_time;
	}
	else	//"Classic" mode
	{
		//double value is just the in
		*event_double = (double)(random_time);

		//Nanoseconds is empty
		*event_nanoseconds = 0;
	}
}

//Function to parse a comma-separated list to get the next timestamp (or the last timestamp)
// start_token - pointer to character field
// time_val - pointer to TIMESTAMP variable of "normal" time
// micro_time_val - pointer to microsecond portion of TIMESTAMP variable, if available
// dbl_time_val - pointer to double-precision (combined) time variable
char *eventgen::time_token(char *start_token, TIMESTAMP *time_val, unsigned int *micro_time_val, double *dbl_time_val)
{
	char workArray[64];	//If we ever need over 64, this will need changing
	char *outIndex, *workIndex, *end_token;
	char index;

	//Initialize work variable
	for (index=0; index<64; index++)
		workArray[index] = 0;

	//Link the output
	workIndex = workArray;

	//Look for a comma in the input value
	outIndex = strchr(start_token,',');	//Look for commas, or none

	if (outIndex == NULL)	//No commas found
	{
		while (*start_token != '\0')
		{
			*workIndex++ = *start_token++;
		}

		end_token = NULL;
	}
	else	//Comma found, but we only want to go just before it
	{
		while (start_token < outIndex)
		{
			*workIndex++ = *start_token++;
		}

		end_token = start_token+1;
	}

	//Point back to the beginning
	workIndex = workArray;

	//Convert it
	*time_val = gl_delta_parsetime(workIndex,micro_time_val,dbl_time_val);

	//Return the end pointer
	return end_token;
}

//Function to parse a comma-separated list to get the next object (or the last object)
char *eventgen::obj_token(char *start_token, OBJECT **obj_val)
{
	char workArray[128];	//Hopefully, names will never be over 128 characters - seems to get upset if we do more
	char *outIndex, *workIndex, *end_token;
	unsigned char index;

	//Initialize work variable
	for (index=0; index<128; index++)
		workArray[index] = 0;

	//Link the output
	workIndex = workArray;

	//Look for a comma in the input value
	outIndex = strchr(start_token,',');	//Look for commas, or none

	if (outIndex == NULL)	//No commas found
	{
		while (*start_token != '\0')
		{
			*workIndex++ = *start_token++;
		}

		end_token = NULL;
	}
	else	//Comma found, but we only want to go just before it
	{
		while (start_token < outIndex)
		{
			*workIndex++ = *start_token++;
		}

		end_token = start_token+1;
	}

	//Point back to the beginning
	workIndex = workArray;

	//Get the object
	*obj_val = gl_get_object(workIndex);

	//Return the end pointer
	return end_token;
}

//Function to add new event into structure - since externally called, presumes calling object will handle mean_repair_time calls
int eventgen::add_unhandled_event(OBJECT *obj_to_fault, char *event_type, TIMESTAMP fail_time, TIMESTAMP rest_length, int implemented_fault, bool fault_state)
{
	OBJECT *obj = OBJECTHDR(this);
	RELEVANTSTRUCT *new_struct;
	TIMESTAMP rest_time;
	double fail_time_dbl, rest_time_dbl;

	//Create a new structure object
	new_struct = (RELEVANTSTRUCT*)gl_malloc(sizeof(RELEVANTSTRUCT));

	//Make sure it worked
	if (new_struct == NULL)
	{
		GL_THROW("Eventgen:%s - Failed to allocate memory for new reliability event!",obj->name);
		/*  TROUBLESHOOT
		While attempting to allocate memory for a new "unhandled" reliability event, the memory allocation
		failed.  Please try again.  If the error persists, plesa submit your code and a bug report via the trac
		website.
		*/
	}

	//Force rest_length to at least one timestep, if it isn't
	if ((off_nominal_time == true) && (((double)(rest_length)) < glob_min_timestep))
	{
		rest_length = (TIMESTAMP)(glob_min_timestep);	//Force to one

		//Warning
		gl_warning("Outage length for object:%s is less than the minimum timestep of %.0f seconds, rounded to minimum timestep",obj_to_fault->name,glob_min_timestep);
		//Defined above
	}

	//Create a casted fail time variable
	fail_time_dbl = (double)(fail_time);

	//Update restoration time appropriately
	if ((fault_state == false) && (rest_length == TS_NEVER))
	{
		rest_time = TS_NEVER;
		rest_time_dbl = TSNVRDBL;
	}
	else
	{
		rest_time = fail_time + rest_length;
		rest_time_dbl = (double)(fail_time) + (double)(rest_length);
	}

	//Check faulting condition - if false means will fault (leave default)
	if (fault_state == false)
		new_struct->objdetails.implemented_fault = -1;
	else	//Restore - we need to know what we were
		new_struct->objdetails.implemented_fault = implemented_fault;

	//Populate the details - no real supersecond support now, just populate
	memcpy(new_struct->event_type,event_type,33*sizeof(char));
	new_struct->objdetails.obj_of_int = obj_to_fault;
	new_struct->objdetails.obj_made_int = NULL;
	new_struct->objdetails.fail_time = fail_time;
	new_struct->objdetails.fail_time_dbl = fail_time_dbl;
	new_struct->objdetails.fail_length_ns = 0;
	new_struct->objdetails.rest_time = rest_time;
	new_struct->objdetails.rest_time_dbl = rest_time_dbl;
	new_struct->objdetails.rest_time_ns = 0;
	new_struct->objdetails.fail_length = TS_NEVER;
	new_struct->objdetails.fail_length_dbl = TSNVRDBL;
	new_struct->objdetails.fail_length_ns = 0;
	new_struct->objdetails.rest_length = rest_length;
	new_struct->objdetails.rest_length_dbl = (double)(rest_length);
	new_struct->objdetails.rest_length_ns = 0;

	//Set other details
	new_struct->objdetails.in_fault = fault_state;
	new_struct->objdetails.customers_affected = 0;
	new_struct->objdetails.customers_affected_sec = 0;

	//Put into the structure - just add it at the beginning
	new_struct->next = Unhandled_Events.next;
	new_struct->prev = &Unhandled_Events;
	Unhandled_Events.next = new_struct;

	//See how to prioritize
	if (fault_state == false)	//Not faulted yet
	{
		//Super-second
		if (fail_time < next_event_time)
			next_event_time = fail_time;

		//Update sub-second as well
		if (deltamode_inclusive == true)
		{
			if (fail_time_dbl < next_event_time_dbl)
			{
				next_event_time_dbl = fail_time_dbl;
			}
		}
	}
	else	//Starting faulted - check against restoration time
	{
		//Super second
		if (rest_time < next_event_time)
			next_event_time = rest_time;

		//sub-second
		if (deltamode_inclusive == true)
		{
			if (rest_time_dbl < next_event_time_dbl)
			{
				next_event_time_dbl = rest_time_dbl;
			}
		}
	}

	return 1;	//Always successful here
}

//Retrieve the address of a double
double *eventgen::get_double(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_double)
		return NULL;
	return (double*)GETADDR(obj,p);
}

//Function to update any event times after a successful state change
void eventgen::regen_events(TIMESTAMP t1_ts, double t1_dbl){
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP temp_time_A;
	double temp_time_A_dbl;
	unsigned int temp_time_A_nano;
	int index;

	//See if the distribution parameters have changed - if so, regen all faults (items in fault condition will wait until restoration to update)
	//Only do this if in random mode
	if ((fault_implement_mode==false) && ((curr_fail_dist_params[0]!=fail_dist_params[0])
		|| (curr_fail_dist_params[1]!=fail_dist_params[1]) || (curr_rest_dist_params[0]!=rest_dist_params[0])
		|| (curr_rest_dist_params[1]!=rest_dist_params[1]) || (curr_fail_dist!=failure_dist) || (curr_rest_dist!=restore_dist)))
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
		next_event_time_dbl = TSNVRDBL;

		for (index=0; index<UnreliableObjCount; index++)
		{
			if (UnreliableObjs[index].in_fault == false)	//Not faulting, so we don't care if we are now a fault or if we have one upcoming
			{
				//Update the failure and restoration length (do this now) - mininmum timestep issues are handled inside gen_random_time
				gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1],&UnreliableObjs[index].fail_length,&UnreliableObjs[index].fail_length_ns,&UnreliableObjs[index].fail_length_dbl);

				//Find restoration length
				gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1],&temp_time_A,&temp_time_A_nano,&temp_time_A_dbl);

				//If over max outage length, cap it - side note - minimum timestep issues handled inside gen_random_time
				if (temp_time_A_dbl > max_outage_length_dbl)
				{
					UnreliableObjs[index].rest_length = max_outage_length;
					UnreliableObjs[index].rest_length_ns = 0;
					UnreliableObjs[index].rest_length_dbl = max_outage_length_dbl;
				}
				else
				{
					UnreliableObjs[index].rest_length = temp_time_A;
					UnreliableObjs[index].rest_length_ns = temp_time_A_nano;
					UnreliableObjs[index].rest_length_dbl = temp_time_A_dbl;
				}

				if (deltamode_inclusive == true)	//Check for deltamode
				{
					//Update failure time
					UnreliableObjs[index].fail_time_dbl = t1_dbl + UnreliableObjs[index].fail_length_dbl;
					UnreliableObjs[index].fail_time = (TIMESTAMP)(floor(UnreliableObjs[index].fail_time_dbl));
					UnreliableObjs[index].fail_time_ns = (unsigned int)((UnreliableObjs[index].fail_time_dbl - (double)(UnreliableObjs[index].fail_time))*1.0e9 + 0.5);
				}
				else	//Standard operation
				{
					//Update failure time
					UnreliableObjs[index].fail_time = t1_ts + UnreliableObjs[index].fail_length;
				}

				//Check progression - supersecond
				if (UnreliableObjs[index].fail_time < next_event_time)
				{
					next_event_time = UnreliableObjs[index].fail_time;
				}

				//Check progression - subsecond
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].fail_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].fail_time_dbl;
					}
				}

				//Flag restoration time
				UnreliableObjs[index].rest_time = TS_NEVER;
				UnreliableObjs[index].rest_time_ns = 0;
				UnreliableObjs[index].rest_time_dbl = TSNVRDBL;

				//De-flag the update
				UnreliableObjs[index].in_fault = false;

			}//End non-faulted object update
			else	//Implies it is in a fault - update next_event_time (if it is done, it will be handled below)
			{
				//Check supersecond
				if (UnreliableObjs[index].rest_time < next_event_time)
				{
					next_event_time = UnreliableObjs[index].rest_time;
				}

				//Check subsecond
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].rest_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].rest_time_dbl;
					}
				}
			}
		}//End failed objects traversion
	}//end distribution parameter change
}

//Functionalized version of old presync code
//Performs actual event status changes on the system
void eventgen::do_event(TIMESTAMP t1_ts, double t1_dbl, bool entry_type)
{
	OBJECT *hdr = OBJECTHDR(this);
	TIMESTAMP temp_time_A;
	double temp_time_A_dbl;
	unsigned int temp_time_A_nano;
	TIMESTAMP mean_repair_time;
	FUNCTIONADDR funadd = NULL;
	int returnval, index;
	char impl_fault[257];
	RELEVANTSTRUCT *temp_struct, *temp_struct_b;
	
	//Reset next event time - we'll find the new one in here
	next_event_time = TS_NEVER;
	next_event_time_dbl = TSNVRDBL;

	//Loop through and find events that are next
	for (index=0; index<UnreliableObjCount; index++)
	{
		//Check failure time
		if ((((UnreliableObjs[index].fail_time <= t1_ts) && (entry_type == false)) || ((UnreliableObjs[index].fail_time_dbl <= t1_dbl) && (entry_type == true))) && (UnreliableObjs[index].in_fault == false))	//Failure!
		{
			//See if we're allowed to fault
			if ((faults_in_prog < max_simult_faults) || (max_simult_faults == -1))	//Room to fault or infinite amount
			{
				//See if something else has already asked for a count update
				if (diff_count_needed == false)
				{
					if (*secondary_interruption_cnt == true)
					{
						metrics_obj->get_interrupted_count_secondary(&curr_time_interrupted,&curr_time_interrupted_sec);	//Get the count of currently interrupted objects
					}
					else	//No secondaries
					{
						curr_time_interrupted = metrics_obj->get_interrupted_count();	//Get the count of currently interrupted objects
					}
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
				
				returnval = ((int (*)(OBJECT *, OBJECT **, char *, int *, TIMESTAMP *, void *))(*funadd))(UnreliableObjs[index].obj_of_int,&UnreliableObjs[index].obj_made_int,fault_type.get_string(),&UnreliableObjs[index].implemented_fault,&mean_repair_time,metrics_obj->Extra_Data);

				if (returnval == 0)	//Failed :(
				{
					GL_THROW("Failed to induce event on %s",UnreliableObjs[index].obj_of_int->name);
					/*  TROUBLESHOOT
					Eventgen attempted to induce a fault on the object, but the object failed to induce the fault.
					Check the object's create_fault code.  If necessary, submit your code and a bug report using the
					trac website.
					*/
				}

				//Update restoration time
				if (fault_implement_mode == false)	//If random mode
				{
					//Bring the restoration time to bear
					//Deltamode check - handle times "traditionally" or not
					if (deltamode_inclusive == true)
					{
						UnreliableObjs[index].rest_time_dbl = t1_dbl + UnreliableObjs[index].rest_length_dbl;
						UnreliableObjs[index].rest_time = (TIMESTAMP)(floor(UnreliableObjs[index].rest_time_dbl));
						UnreliableObjs[index].rest_time_ns = (unsigned int)((UnreliableObjs[index].rest_time_dbl - (double)(UnreliableObjs[index].rest_time))*1.0e9 + 0.5);
					}
					else	//Nope
					{
						UnreliableObjs[index].rest_time = t1_ts + UnreliableObjs[index].rest_length;
					}
				}

				//Regardless, add the repair time, if it exists
				if (mean_repair_time != 0)
				{
					UnreliableObjs[index].rest_time += mean_repair_time;
					UnreliableObjs[index].rest_time_dbl += (double)mean_repair_time;
				}

				//See if this is the new update -- supersecond
				if (UnreliableObjs[index].rest_time < next_event_time)
				{
					next_event_time = UnreliableObjs[index].rest_time;
				}

				//Subsecond update
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].rest_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].rest_time_dbl;
					}
				}				

				//Flag outage time so it won't trip things
				UnreliableObjs[index].in_fault = true;

				//Flag customer count to know we need to populate this value
				UnreliableObjs[index].customers_affected = -1;

				//Do the same for secondary - regardless of if we want it or not
				UnreliableObjs[index].customers_affected_sec = -1;

				//Increment the simultaneous fault counter (prevents later objects from faulting if they are ready)
				faults_in_prog++;
			}
			else	//Fault limit has been hit, give us a new time to play
			{
				if (fault_implement_mode == false)	//Random mode - OK to do this
				{
					//Update the failure and restoration length (do this now) - minimum timestep issues handled inside gen_random_time
					gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1],&UnreliableObjs[index].fail_length,&UnreliableObjs[index].fail_length_ns,&UnreliableObjs[index].fail_length_dbl);

					//Update failure time
					//Deltamode check - handle times "traditionally" or not
					if (deltamode_inclusive == true)
					{
						UnreliableObjs[index].fail_time_dbl = t1_dbl + UnreliableObjs[index].fail_length_dbl;
						UnreliableObjs[index].fail_time = (TIMESTAMP)(floor(UnreliableObjs[index].fail_time_dbl));
						UnreliableObjs[index].fail_time_ns = (unsigned int)((UnreliableObjs[index].fail_time_dbl - (double)(UnreliableObjs[index].fail_time))*1.0e9 + 0.5);
					}
					else	//Nope
					{
						UnreliableObjs[index].fail_time = t1_ts + UnreliableObjs[index].fail_length;
					}

					//Check progression -- super second
					if (UnreliableObjs[index].fail_time < next_event_time)
					{
						next_event_time = UnreliableObjs[index].fail_time;
					}

					//Check progression -- subsecond
					if (deltamode_inclusive == true)
					{
						if (UnreliableObjs[index].fail_time_dbl < next_event_time_dbl)
						{
							next_event_time_dbl = UnreliableObjs[index].fail_time_dbl;
						}
					}
				}
				else	//Deterministic mode - if this happens, a parameter was set wrong.  Flag this as already occurred and move on
				{
					//Toss the warning
					gl_warning("Manual event %d of eventgen:%s exceeded the simultaneous fault limit and was ignored",index,hdr->name);
					/*  TROUBLESHOOT
					While processing manual fault conditions, a fault attempted to exceed the maximum number of faults limit set for this
					eventgen object.  This fault is now ignored.  Increase the maximum simultaneous fault limit and try again.
					*/

					//Set values to TS_NEVER so they never go off
					UnreliableObjs[index].rest_time = TS_NEVER;
					UnreliableObjs[index].rest_time_dbl = TSNVRDBL;
					UnreliableObjs[index].rest_length_ns = 0;
					
					UnreliableObjs[index].fail_time = TS_NEVER;
					UnreliableObjs[index].fail_time_dbl = TSNVRDBL;
					UnreliableObjs[index].fail_time_ns = 0;
				}
			}
		}
		else if (((UnreliableObjs[index].rest_time <= t1_ts) && (entry_type == false)) || ((UnreliableObjs[index].rest_time_dbl <= t1_dbl) && (entry_type == true)))	//Restoration time!
		{
			//Call the object back into service
			funadd = (FUNCTIONADDR)(gl_get_function(UnreliableObjs[index].obj_of_int,"fix_fault"));
			
			//Make sure it was found
			if (funadd == NULL)
			{
				GL_THROW("Unable to induce event on %s",UnreliableObjs[index].obj_of_int->name);
				//Defined above
			}

			returnval = ((int (*)(OBJECT *, int *, char *, void *))(*funadd))(UnreliableObjs[index].obj_of_int,&UnreliableObjs[index].implemented_fault,impl_fault,metrics_obj->Extra_Data);

			if (returnval == 0)	//Restoration is no go :(
			{
				GL_THROW("Failed to induce repair on %s",UnreliableObjs[index].obj_of_int->name);
				/*  TROUBLESHOOT
				Eventgen attempted to induce a repair on the object, but the object failed to perform the repair.
				Check the object's fix_fault code.  If necessary, submit your code and a bug report using the
				trac website.
				*/
			}

			//Lock metrics event
			wlock(metrics_obj_hdr);

			//Call the event updater - call relevant version
			if (*secondary_interruption_cnt == true)
			{
				metrics_obj->event_ended_sec(hdr,UnreliableObjs[index].obj_of_int,UnreliableObjs[index].obj_made_int,UnreliableObjs[index].fail_time,UnreliableObjs[index].rest_time,fault_type.get_string(),impl_fault,UnreliableObjs[index].customers_affected,UnreliableObjs[index].customers_affected_sec);
			}
			else	//no secondaries
			{
				metrics_obj->event_ended(hdr,UnreliableObjs[index].obj_of_int,UnreliableObjs[index].obj_made_int,UnreliableObjs[index].fail_time,UnreliableObjs[index].rest_time,fault_type.get_string(),impl_fault,UnreliableObjs[index].customers_affected);
			}

			//All done, unlock it
			wunlock(metrics_obj_hdr);

			//If in random mode, update the failure values
			if (fault_implement_mode == false)
			{
				//Update the failure and restoration length (do this now) - minimum timestep issues handled inside gen_random_time
				gen_random_time(failure_dist,fail_dist_params[0],fail_dist_params[1],&UnreliableObjs[index].fail_length,&UnreliableObjs[index].fail_length_ns,&UnreliableObjs[index].fail_length_dbl);

				//Find restoration length
				gen_random_time(restore_dist,rest_dist_params[0],rest_dist_params[1],&temp_time_A,&temp_time_A_nano,&temp_time_A_dbl);

				//If over max outage length, cap it - side note - minimum timestep issues handled inside gen_random_time
				if (temp_time_A_dbl > max_outage_length_dbl)
				{
					UnreliableObjs[index].rest_length = max_outage_length;
					UnreliableObjs[index].rest_length_ns = 0;
					UnreliableObjs[index].rest_length_dbl = max_outage_length_dbl;
				}
				else
				{
					UnreliableObjs[index].rest_length = temp_time_A;
					UnreliableObjs[index].rest_length_ns = temp_time_A_nano;
					UnreliableObjs[index].rest_length_dbl = temp_time_A_dbl;
				}

				//Update failure time
				//Deltamode check - handle times "traditionally" or not
				if (deltamode_inclusive == true)
				{
					UnreliableObjs[index].fail_time_dbl = t1_dbl + UnreliableObjs[index].fail_length_dbl;
					UnreliableObjs[index].fail_time = (TIMESTAMP)(floor(UnreliableObjs[index].fail_time_dbl));
					UnreliableObjs[index].fail_time_ns = (unsigned int)((UnreliableObjs[index].fail_time_dbl - (double)(UnreliableObjs[index].fail_time))*1.0e9 + 0.5);
				}
				else	//Nope
				{
					UnreliableObjs[index].fail_time = t1_ts + UnreliableObjs[index].fail_length;
				}

				//Check progression -- super-second
				if (UnreliableObjs[index].fail_time < next_event_time)
					next_event_time = UnreliableObjs[index].fail_time;

				//Check progression -- subsecond
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].fail_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].fail_time_dbl;
					}
				}

				//Flag restoration time
				UnreliableObjs[index].rest_time = TS_NEVER;
				UnreliableObjs[index].rest_time_ns = 0;
				UnreliableObjs[index].rest_time_dbl = TSNVRDBL;

			}
			else	//Manual mode - put both to TS_NEVER so we never come back in
			{
				UnreliableObjs[index].fail_time = TS_NEVER;
				UnreliableObjs[index].fail_time_dbl = TSNVRDBL;
				UnreliableObjs[index].fail_time_ns = 0;

				UnreliableObjs[index].rest_time = TS_NEVER;
				UnreliableObjs[index].rest_time_dbl = TSNVRDBL;
				UnreliableObjs[index].rest_time_ns = 0;
			}

			//De-flag the update
			UnreliableObjs[index].in_fault = false;

			//Decrement us out of the simultaneous fault count (allows possibility of later object in list to fault before count updated)
			faults_in_prog--;
		}
		else	//Not either - see where we sit against the max
		{
			//Failure mode?
			if (UnreliableObjs[index].in_fault == false)
			{
				//Super second check
				if (UnreliableObjs[index].fail_time < next_event_time)
				{
					next_event_time = UnreliableObjs[index].fail_time;
				}

				//Subsecond check
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].fail_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].fail_time_dbl;
					}
				}
			}
			else //Restoration
			{
				//Super-second
				if (UnreliableObjs[index].rest_time < next_event_time)
				{
					next_event_time = UnreliableObjs[index].rest_time;
				}

				//Sub-second
				if (deltamode_inclusive == true)
				{
					if (UnreliableObjs[index].rest_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = UnreliableObjs[index].rest_time_dbl;
					}
				}
			}
			//Defaulted else - we're bigger than the current min
		}
	}//End object loop traversion

	//Traverse the linked list - if anything is in it
	if (Unhandled_Events.next != NULL)	//Something is in there!
	{
		//Copy the pointer
		temp_struct = &Unhandled_Events;

		//Inward we go
		while (temp_struct->next != NULL)
		{
			//Proceed in
			temp_struct = temp_struct->next;

			//Check details (replication of above FOR loop)

			//Check failure time
			if ((((temp_struct->objdetails.fail_time <= t1_ts) && (entry_type == false)) || ((temp_struct->objdetails.fail_time_dbl <= t1_dbl) && (entry_type == true))) && (temp_struct->objdetails.in_fault == false))	//Failure!
			{
				//"Random" events are always allowed to happen
				//See if something else has already asked for a count update
				if (diff_count_needed == false)
				{
					if (*secondary_interruption_cnt == true)
					{
						metrics_obj->get_interrupted_count_secondary(&curr_time_interrupted,&curr_time_interrupted_sec);	//Get the count of currently interrupted objects
					}
					else	//No secondaries
					{
						curr_time_interrupted = metrics_obj->get_interrupted_count();	//Get the count of currently interrupted objects
					}
					diff_count_needed = true;										//Flag us for an update
				}

				//Put a fault on the system
				funadd = (FUNCTIONADDR)(gl_get_function(temp_struct->objdetails.obj_of_int,"create_fault"));
				
				//Make sure it was found
				if (funadd == NULL)
				{
					GL_THROW("Unable to induce event on %s",temp_struct->objdetails.obj_of_int->name);
					//Defined above
				}
				
				returnval = ((int (*)(OBJECT *, OBJECT **, char *, int *, TIMESTAMP *, void *))(*funadd))(temp_struct->objdetails.obj_of_int,&temp_struct->objdetails.obj_made_int,temp_struct->event_type,&temp_struct->objdetails.implemented_fault,&mean_repair_time,metrics_obj->Extra_Data);

				if (returnval == 0)	//Failed :(
				{
					GL_THROW("Failed to induce event on %s",temp_struct->objdetails.obj_of_int->name);
					//Defined above
				}

				//See if a repair time was passed - if so, implement it
				if ((mean_repair_time != 0) && (temp_struct->objdetails.rest_time != TS_NEVER))
				{
					temp_struct->objdetails.rest_time += mean_repair_time;

					//See if a supersecond update is needed too
					if (deltamode_inclusive == true)
					{
						temp_struct->objdetails.rest_time_dbl += (double)(mean_repair_time);
					}
				}

				//See if this is the new update -- super second
				if (temp_struct->objdetails.rest_time < next_event_time)
				{
					next_event_time = temp_struct->objdetails.rest_time;
				}

				//See if this is a new update -- sub-second
				if (deltamode_inclusive == true)
				{
					if (temp_struct->objdetails.rest_time_dbl < next_event_time_dbl)
					{
						next_event_time_dbl = temp_struct->objdetails.rest_time_dbl;
					}
				}

				//Flag outage time so it won't trip things
				temp_struct->objdetails.in_fault = true;

				//Flag customer count to know we need to populate this value
				temp_struct->objdetails.customers_affected = -1;

				//Do the same for secondary - regardless of if we want it or not
				temp_struct->objdetails.customers_affected_sec = -1;

				//Increment the simultaneous fault counter (prevents later objects from faulting if they are ready)
				faults_in_prog++;

				//See if the restoration time is TS_NEVER - if so, we are an induced fault that will be undone separately - delete us
				if (temp_struct->objdetails.rest_time == TS_NEVER)
				{
					//Go to our previous
					temp_struct_b = temp_struct->prev;

					//Update it
					temp_struct_b->next = temp_struct->next;

					//Remove us
					gl_free(temp_struct);

					//Now assign back - if there is more in the list
					if (temp_struct_b->next != NULL)
						temp_struct = temp_struct_b->next;
					else	//Just put us back in
						temp_struct = temp_struct_b;
				}
			}
			else if (((temp_struct->objdetails.rest_time <= t1_ts) && (entry_type == false)) || ((temp_struct->objdetails.rest_time_dbl <= t1_dbl) && (entry_type == true)))	//Restoration time!
			{
				//Call the object back into service
				funadd = (FUNCTIONADDR)(gl_get_function(temp_struct->objdetails.obj_of_int,"fix_fault"));
				
				//Make sure it was found
				if (funadd == NULL)
				{
					GL_THROW("Unable to induce event on %s",temp_struct->objdetails.obj_of_int->name);
					//Defined above
				}

				returnval = ((int (*)(OBJECT *, int *, char *, void *))(*funadd))(temp_struct->objdetails.obj_of_int,&temp_struct->objdetails.implemented_fault,impl_fault,metrics_obj->Extra_Data);

				if (returnval == 0)	//Restoration is no go :(
				{
					GL_THROW("Failed to induce repair on %s",temp_struct->objdetails.obj_of_int->name);
					//Defined above
				}

				//Lock metrics event
				wlock(metrics_obj_hdr);

				//Call the event updater - call relevant version
				if (*secondary_interruption_cnt == true)
				{
					metrics_obj->event_ended_sec(hdr,temp_struct->objdetails.obj_of_int,temp_struct->objdetails.obj_made_int,temp_struct->objdetails.fail_time,temp_struct->objdetails.rest_time,temp_struct->event_type,impl_fault,temp_struct->objdetails.customers_affected,temp_struct->objdetails.customers_affected_sec);
				}
				else	//no secondaries
				{
					metrics_obj->event_ended(hdr,temp_struct->objdetails.obj_of_int,temp_struct->objdetails.obj_made_int,temp_struct->objdetails.fail_time,temp_struct->objdetails.rest_time,temp_struct->event_type,impl_fault,temp_struct->objdetails.customers_affected);
				}

				//All done, unlock it
				wunlock(metrics_obj_hdr);

				//Event is done, remove it from the structure
				//Go to our previous
				temp_struct_b = temp_struct->prev;

				//Update it
				temp_struct_b->next = temp_struct->next;

				//Remove us
				gl_free(temp_struct);

				//Decrement us out of the simultaneous fault count (allows possibility of later object in list to fault before count updated)
				faults_in_prog--;

				//Now assign back - if there is more in the list
				if (temp_struct_b->next != NULL)
					temp_struct = temp_struct_b->next;
				else	//Just put us back in
					temp_struct = temp_struct_b;

			}
			else	//Not either - see where we sit against the max
			{
				//See if we're in a fault first
				if (temp_struct->objdetails.in_fault == false)
				{
					//Super second
					if (temp_struct->objdetails.fail_time < next_event_time)
					{
						next_event_time = temp_struct->objdetails.fail_time;
					}

					//Sub-second
					if (deltamode_inclusive == true)
					{
						if (temp_struct->objdetails.fail_time_dbl < next_event_time_dbl)
						{
							next_event_time_dbl = temp_struct->objdetails.fail_time_dbl;
						}
					}
				}
				else	//Restoration
				{
					//Super-second
					if (temp_struct->objdetails.rest_time < next_event_time)
					{
						next_event_time = temp_struct->objdetails.rest_time;
					}

					//Sub-second
					if (deltamode_inclusive == true)
					{
						if (temp_struct->objdetails.rest_time_dbl < next_event_time_dbl)
						{
							next_event_time_dbl = temp_struct->objdetails.rest_time_dbl;
						}
					}
				}
				//Defaulted else - we're bigger than the current min
			}
		}//End while
	}//end unhandled events linked list

	//Reset and update our current fault counter (just to ensure things are accurate)
	faults_in_prog = 0;
	
	//Loop through objects and increment as necessary
	for (index=0; index<UnreliableObjCount;index++)
	{
		if (UnreliableObjs[index].in_fault == true)
			faults_in_prog++;
	}

	//Loop through the linked-list and do the same
	if (Unhandled_Events.next != NULL)	//Something is in there!
	{
		//Copy the pointer
		temp_struct = &Unhandled_Events;

		//Inward we go
		while (temp_struct->next != NULL)
		{
			//Inward
			temp_struct = temp_struct->next;

			//Check for faults
			if (temp_struct->objdetails.in_fault == true)
				faults_in_prog++;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF DELTA MODE
//////////////////////////////////////////////////////////////////////////
//Module-level call
EXPORT SIMULATIONMODE interupdate_eventgen(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	eventgen *my = OBJECTDATA(obj,eventgen);
	SIMULATIONMODE status = SM_ERROR;
	status = my->inter_deltaupdate(delta_time, dt, iteration_count_val);
	return status;
}

SIMULATIONMODE eventgen::inter_deltaupdate(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	double deltat = (double)delta_time/(double)DT_SECOND; 
	double t1_dbl = (double)gl_globalclock + deltat;
	TIMESTAMP t1_ts;
	double delta_epsilon = (double)dt/(double)DT_SECOND/2; // A small increase to t1 to make sure we stay in deltamode if an event's time lands on the whole second and the next event's time is within a second

	t1_ts = (TIMESTAMP)(t1_dbl + 1.0/(2.0*((double)(DT_SECOND))));

	if(next_event_time_dbl < ceil(t1_dbl + delta_epsilon)){//Is there an event to do between now and the end of this second
		if (t1_dbl < next_event_time_dbl){
			return SM_DELTA;
		}

		//Perform delta regeneratin of any event times
		regen_events(t1_ts,t1_dbl);

		//Perform the execution of any events
		do_event(t1_ts,t1_dbl,true);
		
		// If the next event is within the same second, stay in deltamode
		if(next_event_time_dbl < ceil(t1_dbl + delta_epsilon)){
			return SM_DELTA;
		}else{ return SM_EVENT; }
	}

	return SM_EVENT;
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
		else
			return 0;
	}
	CREATE_CATCHALL(eventgen);
}

EXPORT int init_eventgen(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,eventgen)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(eventgen);
}

EXPORT TIMESTAMP sync_eventgen(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	try
	{
		TIMESTAMP t2 = TS_NEVER;
		eventgen *my = OBJECTDATA(obj,eventgen);
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
		return t2;
	}
	SYNC_CATCHALL(eventgen);
}

//Exposed function to add items
EXPORT int add_event(OBJECT *event_obj, OBJECT *obj_to_fault, char *event_type, TIMESTAMP fail_time, TIMESTAMP rest_length, int implemented_fault, bool fault_state)
{
	int ret_value;
	eventgen *eventgenobj = OBJECTDATA(event_obj,eventgen);

	//Call the function
	ret_value = eventgenobj->add_unhandled_event(obj_to_fault, event_type, fail_time, rest_length, implemented_fault, fault_state);

	//Return the result
	return ret_value;
}
/**@}*/

