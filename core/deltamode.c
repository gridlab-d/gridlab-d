/** $Id: deltamode.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2011 Battelle Memorial Institute
	@file deltamode.cpp
	@addtogroup modules Deltamode Operation

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

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
	clock_t t = clock();

	/* count qualified modules */
	for ( module=module_get_first() ; module!=NULL ; module=module_get_next(module) )
	{
		if ( 0 != module->deltadesired ){
			// this could probably be counted in module_init()...
			delta_modulecount++;
			if (profile.module_list[0]!=0)
				strcat(profile.module_list,",");
			strcat(profile.module_list,module->name);
		}
	}

	/* if none, stop here */
	if ( delta_modulecount==0 ){
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
	for ( module=module_get_first() ; module!=NULL ; module=module_get_next(module) )
	{
		if ( 0 != module->deltadesired )
		{
			if ( delta_modulecount>0 )
				strcat(global_deltamode_updateorder,",");
			strcat(global_deltamode_updateorder,module->name);
			delta_modulelist[delta_modulecount++] = module;
		}
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
	unsigned int delta_iteration_remaining, delta_iteration_count;
	SIMULATIONMODE interupdate_mode, interupdate_mode_result;
	int n;
	double dbl_stop_time;
	double dbl_curr_clk_time;
	OBJECT *d_obj = NULL;
	CLASS *d_oclass = NULL;

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

	/* process updates until mode is switched or 1 hour elapses */
	for ( global_deltaclock=0; global_deltaclock<global_deltamode_maximumtime; global_deltaclock+=timestep )
	{
		/* Check to make sure we haven't reached a stop time */
		global_delta_curr_clock = dbl_curr_clk_time + (double)global_deltaclock/(double)DT_SECOND;

		/* Make sure it isn't over the limit */
		if (global_delta_curr_clock>dbl_stop_time)
		{
			break;	/* Just get us out of here */
		}

		/* set time context for deltamode */
		output_set_delta_time_context(global_clock,global_deltaclock);

		/* Initialize iteration limit counter */
		delta_iteration_remaining = global_deltamode_iteration_limit;

		/* Initialize the iteration counter - seems silly to do, but saves a fetch */
		delta_iteration_count = 0;

		/* main object update loop */
		realtime_run_schedule();

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
						interupdate_mode_result = d_oclass->update(d_obj,global_clock,global_deltaclock,timestep,delta_iteration_count);

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

		if ( interupdate_mode==SM_EVENT )
		{
			/* no module wants deltamode to continue any further */
			break;
		}
	}

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
	DT timestep = global_deltamode_timestep;
	MODULE **module;
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
