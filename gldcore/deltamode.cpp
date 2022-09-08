/** $Id: deltamode.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file deltamode.cpp
	@addtogroup modules Deltamode Operation

 @{
 **/

#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include "globals.h"
#include "module.h"
#include "class.h"
#include "object.h"
#include "deltamode.h"
#include "output.h"
#include "realtime.h"

static OBJECT **delta_objectlist = NULL; /* qualified object list */
static int delta_objectcount = 0; /* qualified object count */
static MODULE **delta_modulelist = NULL; /* qualified module list */
static int delta_modulecount = 0; /* qualified module count */

/* profile data structure */
static DELTAPROFILE profile;
DELTAPROFILE *delta_getprofile(void)
{
	return &profile;
}

/** Initialize the delta mode code

	This call must be completed before the first call to any delta mode code.
	If the call fails, no delta mode code can be executed.  Failure does not
	affect whether event mode code can run.

	@return SUCCESS or FAILED
 **/
STATUS delta_init(void)
{
	OBJECT *obj, **pObj;
	char temp_name_buff[64];
	unsigned int n, toprank = 0;
	OBJECT ***ranklist;
	int *rankcount;
	MODULE *module;
	typedef struct {
		MODULE *tape_mod;
		MODULE *connection_mod;
		MODULE *reliability_mod;
		MODULE *residential_mod;
		MODULE *powerflow_mod;
	} MODFOUNDSTRUCT;
	MODFOUNDSTRUCT modules_found;
	bool *ordered_module;
	clock_t t = clock();
	size_t mod_count = module_getcount(); 

	/* Ordered module initialization - just because */
	modules_found.tape_mod = NULL;
	modules_found.connection_mod = NULL;
	modules_found.reliability_mod = NULL;
	modules_found.residential_mod = NULL;
	modules_found.powerflow_mod = NULL;
	ordered_module = NULL;

	/* Preallocate the tracking array, if in the "preferred sort" mode */
	if (global_deltamode_force_preferred_order == true)
	{
		ordered_module = (bool *)malloc(mod_count*sizeof(bool));

		// Make sure it worked
		if (ordered_module == NULL)
		{
			output_error("deltamode: unable to allocate memory for module order tracking");
			/*  TROUBLESHOOT
			While attempting to allocate memory for a deltamode tracking array, an error occurred.
			Please try again.  If the error persists, please submit an issue via the ticketing system.
			*/
			return FAILED;
		}

		//Ensure it is false, for giggles
		for (n=0; n<mod_count; n++)
		{
			ordered_module[n] = false;
		}
	}
	//Default else - array not needed

	//Reset loop counter - for next item
	n = 0;

	/* count qualified modules - populate "special" ones, if necessary */
	for ( module=module_get_first() ; module!=NULL ; module=module_get_next(module) )
	{
		if ( 0 != module->deltadesired ){
			// this could probably be counted in module_init()...
			delta_modulecount++;
			if (profile.module_list[0]!=0)
				strcat(profile.module_list,",");
			strcat(profile.module_list,module->name);

			//See if we are trying to do the "managed order"
			if (global_deltamode_force_preferred_order == true)
			{
				if (strcmp(module->name,"tape") == 0)
				{
					//Store
					modules_found.tape_mod = module;

					//Flag
					ordered_module[n] = true;
				}
				else if (strcmp(module->name,"connection") == 0)
				{
					//Store
					modules_found.connection_mod = module;

					//Flag
					ordered_module[n] = true;
				}
				else if (strcmp(module->name,"reliability") == 0)
				{
					//Store
					modules_found.reliability_mod = module;

					//Flag
					ordered_module[n] = true;
				}
				else if (strcmp(module->name,"residential") == 0)
				{
					//Store
					modules_found.residential_mod = module;

					//Flag
					ordered_module[n] = true;
				}
				else if (strcmp(module->name,"powerflow") == 0)
				{
					//Store
					modules_found.powerflow_mod = module;

					//Flag
					ordered_module[n] = true;
				}
				//Default else - other module, don't care - already set false in tracker
			}
			//Default else - no, just leave as is
		}
		//Default else - no deltamode update function

		//Increment the counter - regardless of if used or not
		n++;
	}

	/* if none, stop here */
	if ( delta_modulecount==0 ){
		//De-allocate the tracking array, if it was used
		if (ordered_module != NULL)
		{
			free(ordered_module);
		}

		goto Success;
	}

	/* allocate memory for qualified module list */
	delta_modulelist = (MODULE**)malloc(sizeof(MODULE**)*delta_modulecount);
	if(0 == delta_modulelist){
		output_error("unable to allocate memory for deltamode module list");
		/* TROUBLESHOOT
		  Deltamode operation requires more memory than is available.
		  Try freeing up memory by making more heap available or making the model smaller. 
		 */
		return FAILED;
	}
	/* build qualified module list */
	delta_modulecount = 0;
	global_deltamode_updateorder[0]='\0';

	/* See if we want "ordered mode" first */
	if (global_deltamode_force_preferred_order == true)
	{
		//Check the individual ones - tape
		if (modules_found.tape_mod != NULL)
		{
			//Assumes it is first - put in the string
			strcat(global_deltamode_updateorder,modules_found.tape_mod->name);

			//Add to the pointer list
			delta_modulelist[delta_modulecount++] = modules_found.tape_mod;
		}

		//Connection
		if (modules_found.connection_mod != NULL)
		{
			//Add to the string list
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,modules_found.connection_mod->name);

			//Add to the main list
			delta_modulelist[delta_modulecount++] = modules_found.connection_mod;
		}

		//Reliability
		if (modules_found.reliability_mod != NULL)
		{
			//Add to the string list
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,modules_found.reliability_mod->name);

			//Add to the main list
			delta_modulelist[delta_modulecount++] = modules_found.reliability_mod;
		}

		//Residential
		if (modules_found.residential_mod != NULL)
		{
			//Add to the string list
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,modules_found.residential_mod->name);

			//Add to the main list
			delta_modulelist[delta_modulecount++] = modules_found.residential_mod;
		}

		//Powerflow
		if (modules_found.powerflow_mod != NULL)
		{
			//Add to the string list
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,modules_found.powerflow_mod->name);

			//Add to the main list
			delta_modulelist[delta_modulecount++] = modules_found.powerflow_mod;
		}
	}
	//Default else - just do normal routine

	//Reset loop varaible, again
	n=0;

	//Loop through the modules and populate them
	for ( module=module_get_first() ; module!=NULL ; module=module_get_next(module) )
	{
		//See which mode we're in
		if (global_deltamode_force_preferred_order == true)
		{
			if (ordered_module[n++] == true)
			{
				continue;	//Already handled - onward
			}
			//Not handled yet, proceed with normal/unsorted routine
		}

		//Standard/unhandled module population
		if ( 0 != module->deltadesired )
		{
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,module->name);
			delta_modulelist[delta_modulecount++] = module;
		}
	}

	//Free up the malloced array, if done (since not needed anymore)
	if (ordered_module != NULL)
	{
		free(ordered_module);
	}

	/* count qualified objects */
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		if ( obj->flags&OF_DELTAMODE )
		{
			if ( !obj->oclass->update )
				output_debug("object '%s' requested deltamode updates but the class '%s' does not export the update function", object_name(obj, temp_name_buff, 63), obj->oclass->name);
			delta_objectcount++;
			if ( obj->rank > toprank ){
				toprank = obj->rank;
			}
		}
	}

	/* if none, stop here */
	if ( delta_objectcount==0 ){
		goto Success;
	}

	/* allocate final object list */
	delta_objectlist = (OBJECT**)malloc(sizeof(OBJECT*)*delta_objectcount);
	if ( delta_objectlist==NULL)
	{
		output_error("unable to allocate memory for deltamode object list");
		/* TROUBLESHOOT
		  Deltamode operation requires more memory than is available.
		  Try freeing up memory by making more heap available or making the model smaller. 
		 */
		return FAILED;
	}

	/* allocate rank lists */
	ranklist = (OBJECT***)malloc(sizeof(OBJECT**)*(toprank+1));
	if ( 0 == ranklist ){
		output_error("unable to allocate memory for deltamode ranklist");
		/* TROUBLESHOOT
		  Deltamode operation requires more memory than is available.
		  Try freeing up memory by making more heap available or making the model smaller. 
		 */
		return FAILED;
	}
	memset(ranklist,0,sizeof(OBJECT**)*(toprank+1));

	/* allocate rank counts */
	rankcount = (int*)malloc(sizeof(int)*(toprank+1));
	if ( 0 == rankcount ){
		output_error("unable to allocate memory for deltamode rankcount");
		/* TROUBLESHOOT
		  Deltamode operation requires more memory than is available.
		  Try freeing up memory by making more heap available or making the model smaller. 
		 */
		return FAILED;
	}
	memset(rankcount,0,sizeof(int)*(toprank+1));

	/* count qualified objects in each rank */
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		if ( obj->flags&OF_DELTAMODE ){
			rankcount[obj->rank]++;
		}
	}

	/* allocate rank lists */
	for ( n=0 ; n<=toprank ; n++)
	{
		if ( rankcount[n]>0 )
		{
			ranklist[n] = (OBJECT**)malloc(sizeof(OBJECT*)*rankcount[n]);
			if ( !ranklist[n] ){
				output_error("unable to allocate memory for deltamode rankcount %i", n);
				/* TROUBLESHOOT
				  Deltamode operation requires more memory than is available.
				  Try freeing up memory by making more heap available or making the model smaller. 
				 */
				return FAILED;
			}
			rankcount[n] = 0; /* clear for index recount */
		}
		else
			ranklist[n] = NULL;
	}

	/* assign qualified objects to rank lists */
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		if ( obj->flags&OF_DELTAMODE ){
			ranklist[obj->rank][rankcount[obj->rank]++] = obj;
		}
	}

	/* build final object list */
	pObj = delta_objectlist;
	for ( n=0 ; n<=toprank ; n++)
	{
		int m;
		for ( m=0 ; m<rankcount[n] ; m++ ){
			*pObj++ = ranklist[n][m];
		}
		if ( ranklist[n]!=NULL ){
			free(ranklist[n]);
			ranklist[n] = NULL;
		}
	}

	/* release memory */
	free(rankcount);
	rankcount = NULL;
	free(ranklist);
	ranklist = NULL;
Success:
	profile.t_init += clock() - t;
	return SUCCESS;
}

/** Determine whether any modules desire operation in delta mode and if so at what DT
	@return DT=0 if no modules want to run in delta mode; DT>0 if at least one 
	desires running in delta mode; DT=DT_INVALID on error.
 **/
DT delta_modedesired(DELTAMODEFLAGS *flags)
{
	DT dt_desired = DT_INFINITY;
	DT dt;
	MODULE **module;
	for ( module=delta_modulelist; module<delta_modulelist+delta_modulecount; module++ )
	{
		dt = (*(*module)->deltadesired)(flags);

		if ( dt==DT_INVALID )
		{
			output_error("module '%s' return invalid/infinite dt for delta mode inquiry", (*module)->name);
			/* TROUBLESHOOT
			   A module returned an invalid time-difference for deltamode operation.
			   Generally, this is an internal error and should be reported to the GridLAB-D developers.
			 */
			return DT_INVALID;
		}

		/* Update minimum timestep - if it is different */
		if ( dt>=0 && dt < dt_desired )
		{
			dt_desired = dt;
		}
	}
	/* dt_desired unmodified means nobody want delta mode, return 0 */
	return dt_desired;
}

/** Run a series of delta mode updates until mode changes back to event mode
	@return number of seconds to advance clock
 **/
DT delta_update(void)
{
	char temp_name_buff[64];
	clock_t t = clock();
	DT seconds_advance, timestep;
	DELTAT temp_time;
	unsigned int delta_iteration_remaining, delta_iteration_count, delta_forced_iteration, delta_federation_iteration_remaining;
	SIMULATIONMODE interupdate_mode, interupdate_mode_result, clockupdate_result;
	int n;
	double dbl_stop_time;
	double dbl_curr_clk_time;
	OBJECT *d_obj = NULL;
	CLASS *d_oclass = NULL;
	TIMESTAMP xsync_ret_time, t1_val;
	double xform_temp_time;

	/* send preupdate messages */
	timestep=delta_preupdate();
	if ( 0 == timestep ){
		output_error("delta_update(): preupdate failed");
		/* TROUBLESHOOT
		   A module failed to complete a preupdate operation correctly while operating in deltamode.
		   Generally, this is an internal error and should be reported to the GridLAB-D developers.
		 */
		return DT_INVALID;
	}

	/* Populate global stop time as double - just do so only cast it once */
	dbl_stop_time = (double)global_stoptime;

	/* Do the same with the current "timestamp" */
	dbl_curr_clk_time = (double)global_clock;

	global_federation_reiteration = false;

	/* Initialize the forced "post-update" timestep variable */
	delta_forced_iteration = global_deltamode_forced_extra_timesteps;

	/* Initialize the timestep check from the global */
	global_deltamode_maximumtime = (DELTAT)(global_deltamode_maximumtime_pub + 0.5);

	/* process updates until mode is switched or time limit elapses */
	for ( global_deltaclock=0; global_deltaclock<global_deltamode_maximumtime; global_deltaclock+=timestep )
	{
		/* Check to make sure we haven't reached a stop time */
		global_delta_curr_clock = dbl_curr_clk_time + (double)global_deltaclock/(double)DT_SECOND;

		/* Make sure it isn't over the limit */
		if (global_delta_curr_clock>dbl_stop_time)
		{
			break;	/* Just get us out of here */
		}

		delta_federation_iteration_remaining = global_deltamode_iteration_limit;

		/* Initialize the iteration counter - seems silly to do, but saves a fetch */
		delta_iteration_count = 0;

		/* main object update loop */
		realtime_run_schedule();

		/* time context - so messages look proper */
		output_set_delta_time_context(global_clock,global_deltaclock);

		//Create an integer clock value and update the temporary variable for transforms
		t1_val = (TIMESTAMP)(floor(global_delta_curr_clock));
		xform_temp_time = global_delta_curr_clock;

		//Call transform updates
		xsync_ret_time=transform_syncall(t1_val,static_cast<TRANSFORMSOURCE>(XS_DOUBLE|XS_COMPLEX|XS_ENDUSE|XS_SCHEDULE|XS_LOADSHAPE),&xform_temp_time);

		//Error check
		if (xsync_ret_time == TS_INVALID)
		{
			//Error return
			return DT_INVALID;
		}
		//Update checks not really needed - xforms shouldn't drive deltamode

		//Call some "internals" - replicate what was in exec - done after the transform to sequence properly
		xsync_ret_time=randomvar_syncall(t1_val);
		xsync_ret_time=schedule_syncall(t1_val);
		xsync_ret_time=loadshape_syncall(t1_val);
		xsync_ret_time=enduse_syncall(t1_val);

		/* Federation reiteration loop */
		while (delta_federation_iteration_remaining > 0)
		{
			/* Initialize iteration limit counter */
			delta_iteration_remaining = global_deltamode_iteration_limit;
			/* Initialize the iteration counter - seems silly to do, but saves a fetch */
			delta_iteration_count = 0;

			/* Begin deltamode iteration loop */
			while (delta_iteration_remaining>0) /* Iterate on this delta timestep */
			{
				/* Assume we are ready to go on, initially */
				interupdate_mode = SM_EVENT;

				/* Loop through objects with their individual updates */
				for ( n=0 ; n<delta_objectcount ; n++ )
				{
					d_obj = delta_objectlist[n];	/* Shouldn't need NULL checks, since they were done above */
					d_oclass = d_obj->oclass;

					/* See if the object is in service or not */
					if ((d_obj->in_svc_double <= global_delta_curr_clock) && (d_obj->out_svc_double >= global_delta_curr_clock))
					{
						if ( d_oclass->update )	/* Make sure it exists - init should handle this */
						{
							/* Call the object-level interupdate */
							interupdate_mode_result = static_cast<SIMULATIONMODE>(d_oclass->update(d_obj,global_clock,global_deltaclock,timestep,delta_iteration_count));

							/* Check the status and handle appropriately */
							switch ( interupdate_mode_result ) {
								case SM_DELTA_ITER:
									interupdate_mode = SM_DELTA_ITER;
									break;
								case SM_DELTA:
									if (interupdate_mode != SM_DELTA_ITER)
										interupdate_mode = SM_DELTA;
									/* default else - leave it as is (SM_DELTA_ITER) */
									break;
								case SM_ERROR:
									output_error("delta_update(): update failed for object \'%s\'", object_name(d_obj, temp_name_buff, 63));
									/* TROUBLESHOOT
									   An object failed to update correctly while operating in deltamode.
									   Generally, this is an internal error and should be reported to the GridLAB-D developers.
									 */
									return DT_INVALID;
								case SM_EVENT:
								default: /* mode remains untouched */
									break;
							}
						} /*End update exists */
					}/* End in service */
					/* Defaulted else, skip over it (not in service) */
				}

				/* send interupdate messages */
				interupdate_mode_result = delta_interupdate(timestep,delta_iteration_count);

				/* Check interupdate return statii */
				if (interupdate_mode_result == SM_ERROR)
				{
					output_error("delta_update(): interupdate failed");
					/* TROUBLESHOOT
					   A module failed to complete an interupdate correctly while operating in deltamode.
					   Generally, this is an internal error and should be reported to the GridLAB-D developers.
					 */
					return DT_INVALID;
				}

				if (global_deltamode_forced_always == true)
				{
					/* Override both returns, if SM_EVENT (all others okay) */
					if (interupdate_mode_result == SM_EVENT)
						interupdate_mode_result = SM_DELTA;

					if (interupdate_mode == SM_EVENT)
						interupdate_mode = SM_DELTA;
				}

				/* Now reconcile with object-level (if called) -- error is already handled */
				if ((interupdate_mode_result != SM_EVENT) && (interupdate_mode == SM_EVENT))
				{
					interupdate_mode = interupdate_mode_result;	/* It is SM_DELTA or SM_DELTA_ITER, which trumps SM_EVENT */
				}
				else if ((interupdate_mode_result == SM_DELTA_ITER) && (interupdate_mode == SM_DELTA))
				{
					interupdate_mode = interupdate_mode_result;	/* Sets us to SM_DELTA_ITER */
				}
				/* defaulted else - interupdate is SM_DELTA_ITER or both are, so it doesn't matter */

				if ((interupdate_mode == SM_DELTA) || (interupdate_mode == SM_EVENT))
				{
					break;	/* Get us out of the while and proceed as appropriate */
				}
				/* Defaulted else - SM_DELTA_ITER - stay here and continue on */

				/* Update iteration counters */
				delta_iteration_remaining--;
				delta_iteration_count++;
			} /* End iterating loop of deltamode */

			/* If iteration limit reached, error us out */
			if (delta_iteration_remaining==0)
			{
				output_error("delta_update(): interupdate iteration limit reached");
				/*  TROUBLESHOOT
				While performing the interupdate portion of the deltamode updates, too many
				reiterations were requested, so the simulation failed to progress forward.  You can
				increase the iteration count with the global variable deltamode_iteration_limit.
				*/
				return DT_INVALID;
			}

			// We have finished the current timestep. Call delta_clockUpdate.
			clockupdate_result = delta_clockupdate(timestep, interupdate_mode);

			if(clockupdate_result == SM_DELTA_ITER) {
				interupdate_mode = clockupdate_result;
				delta_federation_iteration_remaining--;
				global_federation_reiteration = true;
			} else if(clockupdate_result == SM_DELTA) {
				global_federation_reiteration = false;
				if(interupdate_mode == SM_EVENT) {
					output_error("delta_update(): delta_interupdate() wanted to exit deltamode. However, delta_clockupdate() wanted to continue in deltamode. This shouldn't happen. Please check your object's delta_clockupdate function to determine what went wrong.");
					/*  TROUBLESHOOT
					While performing the interupdate portion of the deltamode updates,
					all objects determined that it was ok to exit deltamode. When this is the
					case, objects delta_clockupdate can only return DELTA_ITER indicating that
					there needs to be a reiteration of the current deltamode timestep or EVENT
					indicating that the simulation can exit deltamode. Please check your object's
					delat_clockupdate function to make sure it adhere's to this behavior.
					*/
					return DT_INVALID;
				} else {
					break;
				}
			} else if(clockupdate_result == SM_EVENT) {
				global_federation_reiteration = false;
				if(interupdate_mode == SM_DELTA) {
					output_error("delta_update(): delta_interupdate() wanted to move to the next deltamode timestep. However, delta_clockupdate() wanted to exit deltamode. This shouldn't happen. Please check your object's delta_clockupdate function to determine what went wrong.");
					/*  TROUBLESHOOT
					While performing the interupdate portion of the deltamode updates,
					all objects determined that it was ok move to the next deltamode timestep.
					When this is the case, objects delta_clockupdate can only return DELTA_ITER
					indicating that there needs to be a reiteration of the current deltamode
					timestep or DELTA indicating that the simulation can move to the next
					deltamode timestep. Please check your object's delta_clockupdate function
					to make sure it adhere's to this behavior.
					*/
					return DT_INVALID;
				} else {
					break;
				}
			} else if(clockupdate_result == SM_ERROR) {
				output_error("delta_update(): clockupdate failed");
				/* TROUBLESHOOT
				 * A module failed to complete an clockupdate correctly while operating in deltamode.
				 * Generally, this is an internal error and should be reported to the GridLAB-D developers.
				 */
				return DT_INVALID;
			}
		}

		/* If federation iteration limit reached, error us out */
		if (delta_federation_iteration_remaining==0)
		{
			output_error("delta_update(): interupdate federation iteration limit reached");
			/*  TROUBLESHOOT
			While performing the clockupdate portion of the deltamode updates, too many
			federate reiterations were requested, so the simulation failed to progress forward. You can
			increase the iteration count with the global variable deltamode_iteration_limit.
			*/
			return DT_INVALID;
		}

		if ( interupdate_mode==SM_EVENT )
		{
			/* See how we need to update - if the forced update is lower, exit*/
			if (delta_forced_iteration < 1)
			{
				/* no module wants deltamode to continue any further */
				break;
			}
			else
			{
				/* Decrement the tracker variable */
				delta_forced_iteration--;

				/* Normal continue - next delta timestep -- allows global overrides to still take place */

				output_verbose("delta_update(): Forced deltamode reiteration - %d left",delta_forced_iteration);
			}
		}
		else if (interupdate_mode == SM_DELTA)
		{
			/* Refresh the forced counter */
			delta_forced_iteration = global_deltamode_forced_extra_timesteps;
		}
		/* Others - Must be an error? */
	}/* End of delta timestep run */

	/* profile */
	if ( profile.t_min==0 || timestep<profile.t_min ) profile.t_min = timestep;
	if ( profile.t_max==0 || timestep>profile.t_max ) profile.t_max = timestep;
	profile.t_delta += global_deltaclock;

	/* send postupdate messages */
	if ( FAILED == delta_postupdate() ){
		output_error("delta_update(): postupdate failed");
		/* TROUBLESHOOT
		   A module failed to complete an postupdate correctly while operating in deltamode.
		   Generally, this is an internal error and should be reported to the GridLAB-D developers.
		 */
		return DT_INVALID;
	}

	profile.t_update += clock() - t;
	seconds_advance = (DT)(global_deltaclock/DT_SECOND);	/* Initial guess, check rounding */
	
	/* See what's left and determine rounding */
	temp_time = global_deltaclock - ((DELTAT)(seconds_advance)*DT_SECOND);

	/* Determine if an increment is necessary */
	if (temp_time != 0)
		seconds_advance++;

	return seconds_advance;
}

static DT delta_preupdate(void)
{
	clock_t t = clock();
	DT timestep;
	MODULE **module;

	/* Convert the published global into the integer version */
	global_deltamode_timestep = (DT)(global_deltamode_timestep_pub + 0.5);

	/* Store it for this check */
	timestep = global_deltamode_timestep;

	/* Check it and make sure it isn't invalid - only zero is invalid */
	/* Unsigned, so a negative number will just roll over */
	/* Would theoretically get caught by the overall zero check, but not really indicate why */
	if (timestep == 0)
	{
		output_error("delta_preupdate(): global deltamode_timestep is zero!");
		/*  TROUBLESHOOT
		The value for the global deltamode_timestep returned as zero.  It needs to be a positive number, with 1 ns
		being the smallest timestep.  If it was specified below 1 ns, try again with a larger timestep.
		*/
		return 0;
	}

	for ( module=delta_modulelist; module<delta_modulelist+delta_modulecount; module++ )
	{
		DT dt = (*module)->preupdate(*module,global_clock,global_deltaclock);
		if ( (dt==0) || (dt == DT_INVALID)){
			output_error("delta_preupdate(): module %s failed", (*module)->name);
			/* TROUBLESHOOT
			   A module failed to complete an preupdate correctly while operating in deltamode.
			   Generally, this is an internal error and should be reported to the GridLAB-D developers.
			 */
			return 0;
		} else if ( dt < timestep ){
			timestep = dt;
		}
	}
	profile.t_preupdate += clock() - t;
	return timestep;
}

static SIMULATIONMODE delta_interupdate(DT timestep,unsigned int iteration_count_val)
{
	clock_t t = clock();
	SIMULATIONMODE mode = SM_EVENT;
	MODULE **module;
	SIMULATIONMODE result;
	for ( module=delta_modulelist; module<delta_modulelist+delta_modulecount; module++ )
	{
		result = (*module)->interupdate(*module,global_clock,global_deltaclock,timestep,iteration_count_val);
		switch ( result ) {
		case SM_DELTA_ITER:
			mode = SM_DELTA_ITER;
			break;
		case SM_DELTA:
			if (mode != SM_DELTA_ITER)
				mode = SM_DELTA;
			/* default else - leave it as is (SM_DELTA_ITER) */
			break;
		case SM_ERROR:
			return SM_ERROR;
		case SM_EVENT:
		default: /* mode remains untouched */
			break;
		}
	}
	profile.t_interupdate += clock() - t;
	profile.t_count++;
	return mode;
}

static SIMULATIONMODE delta_clockupdate(DT timestep, SIMULATIONMODE interupdate_result)
{
	clock_t t = clock();
	double nextTime = 0;
	DT exitDeltaTimestep = 0;
	MODULE ** module;
	SIMULATIONMODE rv = SM_EVENT;
	SIMULATIONMODE result = SM_EVENT;
	if(interupdate_result == SM_DELTA)
	{
		rv = SM_DELTA;
		//calculate the next timestep that we are going to in nanoseconds
		nextTime = global_delta_curr_clock + ((double)timestep)/((double)DT_SECOND);
		for(module=delta_modulelist; module < (delta_modulelist + delta_modulecount); module++)
		{
			if((*module)->deltaClockUpdate != NULL)
			{
				result = (*module)->deltaClockUpdate(*module, nextTime, timestep, interupdate_result);
				switch ( result )
				{
				case SM_DELTA_ITER:
					output_error("delta_clockupdate(): It is too late to reiterate on the current time step.");
					return SM_EVENT;
				case SM_DELTA:
						rv = SM_DELTA;
					/* default else - leave it as is (SM_DELTA_ITER) */
					break;
				case SM_ERROR:
					return SM_ERROR;
				case SM_EVENT:
					rv = SM_DELTA;
					break;
				default: /* mode remains untouched */
					break;
				}
			}
		}
	} else if(interupdate_result == SM_EVENT) {
		rv = SM_EVENT;
		// we are exiting deltamode so we need to determine the timestep to the next whole second.
		DT nextTimeNano = (DT)(ceil(global_delta_curr_clock)*((double)DT_SECOND));
		DT currTimeNano = (DT)(global_delta_curr_clock*((double)DT_SECOND));
		nextTime = ceil(global_delta_curr_clock);
		exitDeltaTimestep = nextTimeNano - currTimeNano;
		for(module=delta_modulelist; module < (delta_modulelist + delta_modulecount); module++)
		{
			if((*module)->deltaClockUpdate != NULL)
			{
				result = (*module)->deltaClockUpdate(*module, nextTime, exitDeltaTimestep, interupdate_result);
				switch ( result )
				{
				case SM_DELTA_ITER:
					output_error("delta_clockupdate(): It is too late to reiterate on the current time step.");
					return SM_ERROR;
				case SM_DELTA:
					output_error("delta_clockupdate(); Every object wishes to exit delta mode. It is too late to stay in it.");
					/* default else - leave it as is (SM_DELTA_ITER) */
					return SM_ERROR;
					break;
				case SM_ERROR:
					return SM_ERROR;
				case SM_EVENT:
					rv = SM_EVENT;
					break;
				default: /* mode remains untouched */
					break;
				}
			}
		}
	}
	profile.t_clockupdate += clock() - t;
	return rv;
}

static STATUS delta_postupdate(void)
{
	clock_t t = clock();
	MODULE **module;
	for ( module=delta_modulelist; module<delta_modulelist+delta_modulecount; module++ )
	{
		if ( !(*module)->postupdate(*module,global_clock,global_deltaclock) )
			return FAILED;
	}
	profile.t_postupdate += clock() - t;
	return SUCCESS;
}

/**@}*/
