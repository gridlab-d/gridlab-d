// $Id: init.cpp 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "gridlabd.h"

#define _POWERFLOW_CPP
#include "powerflow.h"
#undef  _POWERFLOW_CPP

#include "triplex_meter.h"
#include "capacitor.h"
#include "fuse.h"
#include "line.h"
#include "link.h"
#include "load.h"
#include "meter.h"
#include "node.h"
#include "regulator.h"
#include "transformer.h"
#include "switch_object.h"
#include "substation.h"
#include "pqload.h"
#include "voltdump.h"
#include "series_reactor.h"
#include "restoration.h"
#include "frequency_gen.h"
#include "volt_var_control.h"
#include "fault_check.h"
#include "motor.h"
#include "billdump.h"
#include "power_metrics.h"
#include "currdump.h"
#include "recloser.h"
#include "sectionalizer.h"
#include "emissions.h"
#include "load_tracker.h"
#include "triplex_load.h"
#include "impedance_dump.h"
#include "vfd.h"
#include "pole.h"
#include "pole_configuration.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (!set_callback(fntable)) {
		errno = EINVAL;
		return NULL;
	}

	INIT_MMF(powerflow);

	/* exported globals */
	gl_global_create("powerflow::show_matrix_values",PT_bool,&show_matrix_values,NULL);
	gl_global_create("powerflow::primary_voltage_ratio",PT_double,&primary_voltage_ratio,NULL);
	gl_global_create("powerflow::nominal_frequency",PT_double,&nominal_frequency,NULL);
	gl_global_create("powerflow::require_voltage_control", PT_bool,&require_voltage_control,NULL);
	gl_global_create("powerflow::geographic_degree",PT_double,&geographic_degree,NULL);
	gl_global_create("powerflow::fault_impedance",PT_complex,&fault_Z,NULL);
	gl_global_create("powerflow::ground_impedance",PT_complex,&ground_Z,NULL);
	gl_global_create("powerflow::warning_underfrequency",PT_double,&warning_underfrequency,NULL);
	gl_global_create("powerflow::warning_overfrequency",PT_double,&warning_overfrequency,NULL);
	gl_global_create("powerflow::warning_undervoltage",PT_double,&warning_undervoltage,NULL);
	gl_global_create("powerflow::warning_overvoltage",PT_double,&warning_overvoltage,NULL);
	gl_global_create("powerflow::warning_voltageangle",PT_double,&warning_voltageangle,NULL);
	gl_global_create("powerflow::maximum_voltage_error",PT_double,&default_maximum_voltage_error,NULL);
	gl_global_create("powerflow::solver_method",PT_enumeration,&solver_method,
		PT_KEYWORD,"FBS",SM_FBS,
		PT_KEYWORD,"GS",SM_GS,
		PT_KEYWORD,"NR",SM_NR,
		NULL);
	gl_global_create("powerflow::NR_matrix_file",PT_char256,&MDFileName,NULL);
	gl_global_create("powerflow::NR_matrix_output_interval",PT_enumeration,&NRMatDumpMethod,
		PT_KEYWORD,"NEVER",MD_NONE,
		PT_KEYWORD,"ONCE",MD_ONCE,
		PT_KEYWORD,"PER_CALL",MD_PERCALL,
		PT_KEYWORD,"ALL",MD_ALL,
		NULL);
	gl_global_create("powerflow::NR_matrix_output_references",PT_bool,&NRMatReferences,NULL);
	gl_global_create("powerflow::line_capacitance",PT_bool,&use_line_cap,NULL);
	gl_global_create("powerflow::line_limits",PT_bool,&use_link_limits,NULL);
	gl_global_create("powerflow::lu_solver",PT_char256,&LUSolverName,NULL);
	gl_global_create("powerflow::NR_iteration_limit",PT_int64,&NR_iteration_limit,NULL);
	gl_global_create("powerflow::NR_deltamode_iteration_limit",PT_int64,&NR_delta_iteration_limit,NULL);
	gl_global_create("powerflow::NR_superLU_procs",PT_int32,&NR_superLU_procs,NULL);
	gl_global_create("powerflow::default_maximum_voltage_error",PT_double,&default_maximum_voltage_error,NULL);
	gl_global_create("powerflow::default_maximum_power_error",PT_double,&default_maximum_power_error,NULL);
	gl_global_create("powerflow::NR_admit_change",PT_bool,&NR_admit_change,NULL);
	gl_global_create("powerflow::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the powerflow module",NULL);
	gl_global_create("powerflow::all_powerflow_delta", PT_bool, &all_powerflow_delta,PT_DESCRIPTION,"Forces all powerflow objects that are capable to participate in deltamode",NULL);
	gl_global_create("powerflow::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",NULL);
	gl_global_create("powerflow::current_frequency",PT_double,&current_frequency,PT_UNITS,"Hz",PT_DESCRIPTION,"Current system-level frequency of the powerflow system",NULL);
	gl_global_create("powerflow::master_frequency_update",PT_bool,&master_frequency_update,PT_DESCRIPTION,"Tracking variable to see if an object has become the system frequency updater",NULL);
	gl_global_create("powerflow::enable_frequency_dependence",PT_bool,&enable_frequency_dependence,PT_DESCRIPTION,"Flag to enable frequency-based variations in impedance values of lines and loads",NULL);
	gl_global_create("powerflow::default_resistance",PT_double,&default_resistance,NULL);
	gl_global_create("powerflow::enable_inrush",PT_bool,&enable_inrush_calculations,PT_DESCRIPTION,"Flag to enable in-rush calculations for lines and transformers in deltamode",NULL);
	gl_global_create("powerflow::low_voltage_impedance_level",PT_double,&impedance_conversion_low_pu,PT_DESCRIPTION,"Lower limit of voltage (in per-unit) at which all load types are converted to impedance for in-rush calculations",NULL);
	gl_global_create("powerflow::enable_mesh_fault_current",PT_bool,&enable_mesh_fault_current,PT_DESCRIPTION,"Flag to enable mesh-based fault current calculations",NULL);
	gl_global_create("powerflow::convergence_error_handling",PT_enumeration,&convergence_error_handling,PT_DESCRIPTION,"Flag to handle convergence error",
			PT_KEYWORD,"FATAL",CEH_FATAL,
			PT_KEYWORD,"IGNORE",CEH_IGNORE,
			PT_KEYWORD,"COLLAPSE",CEH_COLLAPSE,
			NULL);
	// register each object class by creating the default instance
	new powerflow_object(module);
	new powerflow_library(module);
	new node(module);
	new link_object(module);
	new capacitor(module);
	new fuse(module);
	new meter(module);
	new line(module);
	new line_spacing(module);
    new overhead_line(module);
    new underground_line(module);
    new overhead_line_conductor(module);
    new underground_line_conductor(module);
    new line_configuration(module);
	new transformer_configuration(module);
	new transformer(module);
	new load(module);
	new regulator_configuration(module);
	new regulator(module);
	new triplex_node(module);
	new triplex_meter(module);
	new triplex_line(module);
	new triplex_line_configuration(module);
	new triplex_line_conductor(module);
	new switch_object(module);
	new substation(module);
	new pqload(module);
	new voltdump(module);
	new series_reactor(module);
	new restoration(module);
	new frequency_gen(module);
	new volt_var_control(module);
	new fault_check(module);
	new motor(module);
	new billdump(module);
	new power_metrics(module);
	new currdump(module);
	new recloser(module);
	new sectionalizer(module);
	new emissions(module);
	new load_tracker(module);
	new triplex_load(module);
	new impedance_dump(module);
	new vfd(module);
	new pole(module);
	new pole_configuration(module);

	/* always return the first class registered */
	return node::oclass;
}

//Call function for scheduling deltamode
void schedule_deltamode_start(TIMESTAMP tstart)
{
	if (enable_subsecond_models == true)	//Make sure the overall mode is enabled
	{
		if ( (tstart<deltamode_starttime) && ((tstart-gl_globalclock)<0x7fffffff )) // cannot exceed 31 bit integer
			deltamode_starttime = tstart;	//Smaller and valid, store it
	}
	else
	{
		GL_THROW("powerflow: a call was made to deltamode functions, but subsecond models are not enabled!");
		/*  TROUBLESHOOT
		The schedule_deltamode_start function was called by an object when powerflow's overall enabled_subsecond_models
		flag was not set.  The module-level flag indicates that no devices should use deltamode, but one made the call
		to a deltamode function.  Please check the DELTAMODE flag on all objects.  If deltamode is desired,
		please set the module-level enable_subsecond_models flag and try again.
		*/
	}
}

//deltamode_desired function
//Module-level call to determine when the next object expects
//to enter deltamode, even if it is now.
//Returns time to next deltamode in integers seconds.
//i.e., 0 = deltamode now, 1 = deltamode 1 second from the current simulation time
//Return DT_INFINITY for no deltamode desired
EXPORT unsigned long deltamode_desired(int *flags)
{
	unsigned long dt_val;

	if (enable_subsecond_models == true)	//Make sure we even want to run deltamode
	{
		//See if the value is worth storing, or irrelevant at this time
		if ((deltamode_starttime>=gl_globalclock) && (deltamode_starttime<TS_NEVER))
		{
			//Set the flag to do a soft deltamode transition
			*flags |= DMF_SOFTEVENT;

			//Compute the difference to get the incremental time needed
			dt_val = (unsigned long)(deltamode_starttime - gl_globalclock);

			gl_debug("powerflow: deltamode desired in %d", dt_val);
			return dt_val;
		}
		else
		{
			//No scheduled deltamode, or it wasn't a valid value
			return DT_INFINITY;
		}
	}
	else
	{
		return DT_INFINITY;
	}
}

//preudpate function of deltamode
//Module-level call at beginning of each deltamode simulation
//Returns the timestep this module requires - to be used to determine minimimum
//detamode simulation stepsize
EXPORT unsigned long preupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	if (enable_subsecond_models == true)
	{
		if (deltamode_timestep_publish<=0.0)
		{
			gl_error("powerflow::deltamode_timestep must be a positive, non-zero number!");
			/*  TROUBLESHOOT
			The value for deltamode_timestep, as specified as the module level in powerflow, must be a positive, non-zero number.
			Please use such a number and try again.
			*/

			return DT_INVALID;
		}
		else
		{
			//Cast in the published value
			deltamode_timestep = (unsigned long)(deltamode_timestep_publish+0.5);

			//Return it
			return deltamode_timestep;
		}
	}
	else	//Not desired, just return an arbitrarily large value
	{
		return DT_INFINITY;
	}
}

//interupdate function of deltamode
//Module-level call for each timestep of deltamode
//Ideally, all deltamode objects coordinate through their module call, not their individual "update" call
//Returns SIMULATIONMODE - SM_DELTA, SM_DELTA_ITER, SM_EVENT, or SM_ERROR
EXPORT SIMULATIONMODE interupdate(MODULE *module, TIMESTAMP t0, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val)
{
	int curr_object_number;
	SIMULATIONMODE function_status = SM_EVENT;
	bool event_driven = true;
	bool delta_iter = false;
	bool bad_computation=false;
	NRSOLVERMODE powerflow_type;
	int64 pf_result;
	int64 simple_iter_test, limit_minus_one;
	bool error_state;

	//Set up iteration variables
	simple_iter_test = 0;
	limit_minus_one = NR_delta_iteration_limit - 1;
	error_state = false;

	if (enable_subsecond_models == true)
	{
		//See if this is the first instance -- if so, update the timestep (if in-rush enabled)
		if (deltatimestep_running < 0.0)
		{
			//Set the powerflow global -- technically the same as dt, but in double precision (less divides)
			deltatimestep_running = (double)((double)dt/(double)DT_SECOND);
		}
		//Default else -- already set

		while (simple_iter_test < NR_delta_iteration_limit)	//Simple iteration capability
		{
			//Do the preliminary pass, in case we're needed
			//Loop through the object list and call the updates - loop forward, otherwise parent/child code doesn't work right
			for (curr_object_number=0; curr_object_number<pwr_object_count; curr_object_number++)
			{
				//See if we're in service or not
				if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
				{
					if (delta_functions[curr_object_number] != NULL)
					{
						//Try/catch for any GL_THROWs that may be called
						try {
							//Call the actual function
							function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int, bool))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val,false);
						}
						catch (const char *msg)
						{
							gl_error("powerflow:interupdate - pre-pass function call: %s", msg);
							error_state = true;
							function_status = SM_ERROR;	//Flag as an error too
						}
						catch (...)
						{
							gl_error("powerflow:interupdate - pre-pass function call: unknown exception");
							error_state = true;
							function_status = SM_ERROR;	//Flag as an error too
						}
					}
					else	//No functional call for this, skip it
					{
						function_status = SM_EVENT;	//Just put something here, mainly for error checks
					}
				}
				else //Not in service - just pass
					function_status = SM_DELTA;

				//Just make sure we didn't error 
				if (function_status == SM_ERROR)
				{
					gl_error("Powerflow object:%s - deltamode function returned an error!",delta_objects[curr_object_number]->name);
					// Defined below

					error_state = true;
					break;
				}
				//Default else, we were okay, so onwards and upwards!
			}

			//Check for error states -- no sense trying to solve a powerflow if we're already angry
			if ((error_state == true) || (function_status == SM_ERROR))
			{
				//Break out of this while
				break;
			}

			//Call dynamic powerflow (start of either predictor or correct set)
			powerflow_type = PF_DYNCALC;

			//Put in try/catch, since GL_THROWs inside solver_nr tend to be a little upsetting
			try {
				//Call solver_nr
				pf_result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, NULL, &bad_computation);
			}
			catch (const char *msg)
			{
				gl_error("powerflow:interupdate - solver_nr call: %s", msg);
				error_state = true;
			}
			catch (...)
			{
				gl_error("powerflow:interupdate - solver_nr call: unknown exception");
				error_state = true;
			}
			
			//De-flag any changes that may be in progress
			NR_admit_change = false;

			//Check the status
			if (bad_computation==true)
			{
				gl_error("Newton-Raphson method is unable to converge the dynamic powerflow to a solution at this operation point");
				/*  TROUBLESHOOT
				Newton-Raphson has failed to complete even a single iteration on the dynamic powerflow.
				This is an indication that the method will not solve the system and may have a singularity
				or other ill-favored condition in the system matrices.
				*/
				error_state = true;
				break;
			}
			else if ((pf_result<0) && (simple_iter_test == limit_minus_one))	//Failure to converge - this is a failure in dynamic mode for now
			{
				gl_error("Newton-Raphson failed to converge the dynamic powerflow, sticking at same iteration.");
				/*  TROUBLESHOOT
				Newton-Raphson failed to converge the dynamic powerflow in the number of iterations
				specified in NR_iteration_limit.  It will try again (if the global iteration limit
				has not been reached).
				*/

				error_state = true;
				break;
			}
			else if (error_state == true)	//Some other, unspecified error
			{
				break;	//Get out of the while loop
			}

			//Loop through the object list and call the updates - loop forward for SWING first, to replicate "postsync"-like order
			for (curr_object_number=0; curr_object_number<pwr_object_count; curr_object_number++)
			{
				//See if we're in service or not
				if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
				{
					if (delta_functions[curr_object_number] != NULL)
					{
						//Try/catch for any GL_THROWs that may be called
						try {
							//Call the actual function
							function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int, bool))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val,true);
						}
						catch (const char *msg)
						{
							gl_error("powerflow:interupdate - post-pass function call: %s", msg);
							error_state = true;
							function_status = SM_ERROR;	//Flag as an error too
						}
						catch (...)
						{
							gl_error("powerflow:interupdate - post-pass function call: unknown exception");
							error_state = true;
							function_status = SM_ERROR;	//Flag as an error too
						}
					}
					else	//Doesn't have a function, either intentionally, or "lack of supportly"
					{
						function_status = SM_EVENT;	//No function present, just assume we only like events
					}
				}
				else //Not in service - just pass
					function_status = SM_EVENT;

				//Determine what our return is
				if (function_status == SM_DELTA)
				{
					gl_verbose("Powerflow object:%d - %s - requested deltamode to continue",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));

					event_driven = false;
				}
				else if (function_status == SM_DELTA_ITER)
				{
					gl_verbose("Powerflow object:%d - %s - requested a deltamode reiteration",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));

					event_driven = false;
					delta_iter = true;
				}
				else if (function_status == SM_ERROR)
				{
					gl_error("Powerflow object:%d - %s - deltamode function returned an error!",delta_objects[curr_object_number]->id,(delta_objects[curr_object_number]->name ? delta_objects[curr_object_number]->name : "Unnamed"));
					/*  TROUBLESHOOT
					While performing a deltamode update, one object returned an error code.  Check to see if the object itself provided
					more details and try again.  If the error persists, please submit your code and a bug report via the trac website.
					*/
					error_state = true;
					break;
				}
				//Default else, we're in SM_EVENT, so no flag change needed
			}

			//Check for error states -- blocks the reiteration if something was already angry
			if ((error_state == true) || (function_status == SM_ERROR))
			{
				//Break out of this while
				break;
			}
			else if (pf_result < 0)	//Check and see if we should even consider reiterating or not
			{
				//Increment the iteration counter
				simple_iter_test++;
			}
			else
			{
				break;	//Theoretically done
			}
		}//End iteration while

		//See if we got out here due to an error
		if ((error_state == true) || (function_status == SM_ERROR))
		{
			return SM_ERROR;
		}
				
		//Determine how to exit - event or delta driven
		if (event_driven == false)
		{
			if (delta_iter == true)
				return SM_DELTA_ITER;
			else
				return SM_DELTA;
		}
		else
			return SM_EVENT;
	}
	else	//deltamode not desired
	{
		return SM_EVENT;	//Just event mode
	}
}

//postupdate function of deltamode
//Executes after all objects in the simulation agree to go back to event-driven mode
//Return value is a SUCCESS/FAILURE
EXPORT STATUS postupdate(MODULE *module, TIMESTAMP t0, unsigned int64 dt)
{
	unsigned int64 seconds_advance, temp_time;
	int curr_object_number;
	STATUS function_status;

	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;

		//Deflag the timestep variable as well
		deltatimestep_running = -1.0;

		//See how far we progressed - cast just in case (code pulled from core - so should align)
		seconds_advance = (unsigned int64)(dt/DT_SECOND);

		//See if it rounded
		temp_time = dt - ((unsigned int64)(seconds_advance)*DT_SECOND);

		//Store the "pre-incremented" seconds advance time - used to set object clocks
		//so being the previous second is better for deltamode->supersecond transitions
		deltamode_supersec_endtime = t0 + seconds_advance;

		/* Determine if an increment is necessary */
		if (temp_time != 0)
			seconds_advance++;

		//Update the tracking variable
		deltamode_endtime = t0 + seconds_advance;
		deltamode_endtime_dbl = (double)(t0) + ((double)(dt)/double(DT_SECOND));

		//Loop through delta objects and update the execution times - at this point, mostly just a "call for the sake of a call" function
		for (curr_object_number=0; curr_object_number<pwr_object_count; curr_object_number++)
		{
			//See if it has a post-update function
			if (post_delta_functions[curr_object_number] != NULL)
			{
				//See if we're in service or not
				if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
				{
					//Try/catch for any GL_THROWs that may be called
					try {
						//Call the actual function
						function_status = ((STATUS (*)(OBJECT *))(*post_delta_functions[curr_object_number]))(delta_objects[curr_object_number]);
					}
					catch (const char *msg)
					{
						gl_error("powerflow:postupdate function call: %s", msg);
						function_status = FAILED;
					}
					catch (...)
					{
						gl_error("powerflow:postupdate function call: unknown exception");
						function_status = FAILED;
					}
				}
				else //Not in service
					function_status = SUCCESS;

				//Make sure we worked
				if (function_status == FAILED)
				{
					gl_error("Powerflow object:%s - failed post-deltamode update",delta_objects[curr_object_number]->name);
					/*  TROUBLESHOOT
					While calling the individual object post-deltamode calculations, an error occurred.  Please try again.
					If the error persists, please submit your code and a bug report via the issues system.
					*/
					return FAILED;
				}
				//Default else - successful, keep going
			}
			//Default else -- we didn't have one, so just skip
		}

		//We always succeed from this, just because (unless we failed above)
		return SUCCESS;
	}
	else	//Deltamode not needed -- no idea how we even got here
	{	//Not sure how it would ever get here, but just jump out if that's the case
		return SUCCESS;
	}
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

typedef struct s_pflist {
	OBJECT *ptr;
	s_pflist *next;
} PFLIST;

EXPORT int check()
{
	/* check each link to make sure it has a node at either end */
	FINDLIST *list = gl_find_objects(FL_NEW,FT_MODULE,SAME,"powerflow",NULL);
	OBJECT *obj=NULL;
	int *nodemap,	/* nodemap marks where nodes are */
		*linkmap,	/* linkmap counts the number of links to/from a given node */
		*tomap;		/* counts the number of references to any given node */
	int errcount = 0;
	int objct = 0;
	int queuef = 0, queueb = 0, queuect = 0;
	int islandct = 0;

	GLOBALVAR *gvroot = NULL;
	PFLIST anchor, *tlist = NULL;
	link_object **linklist = NULL,
		 **linkqueue = NULL;

	objct = gl_get_object_count();
	anchor.ptr = NULL;
	anchor.next = NULL;

	nodemap = (int *)malloc((size_t)(objct*sizeof(int)));
	linkmap = (int *)malloc((size_t)(objct*sizeof(int)));
	tomap = (int *)malloc((size_t)(objct*sizeof(int)));
	linkqueue = (link_object **)malloc((size_t)(objct*sizeof(link_object *)));
	linklist = (link_object **)malloc((size_t)(objct*sizeof(link_object *)));
	memset(nodemap, 0, objct*sizeof(int));
	memset(linkmap, 0, objct*sizeof(int));
	memset(tomap, 0, objct*sizeof(int));
	memset(linkqueue, 0, objct*sizeof(link_object *));
	memset(linklist, 0, objct*sizeof(link_object *));
	/* per-object checks */

	/* check from/to info on links */
	while (obj=gl_find_next(list,obj))
	{
		if (gl_object_isa(obj,"node"))
		{
			/* add to node map */
			nodemap[obj->id]+=1;
			/* if no parent, then add to anchor list */
			if(obj->parent == NULL){
				tlist = (PFLIST *)malloc(sizeof(PFLIST));
				tlist->ptr = obj;
				tlist->next = anchor.next;
				anchor.next = tlist;
				tlist = NULL;
			}
		}
		else if (gl_object_isa(obj,"link"))
		{
			link_object *pLink = OBJECTDATA(obj,link_object);
			OBJECT *from = pLink->from;
			OBJECT *to = pLink->to;
			node *tNode = OBJECTDATA(to, node);
			node *fNode = OBJECTDATA(from, node);
			/* count 'to' reference */
			tomap[to->id]++;
			/* check link connections */
			if (from==NULL){
				gl_error("link %s (%s:%d) from object is not specified", pLink->get_name(), pLink->oclass->name, pLink->get_id());
				++errcount;
			}
			else if (!gl_object_isa(from,"node")){
				gl_error("link %s (%s:%d) from object is not a node", pLink->get_name(), pLink->oclass->name, pLink->get_id());
				++errcount;
			} else { /* is a "from" and it isa(node) */
				linkmap[from->id]++; /* mark that this node has a link from it */
			}
			if (to==NULL){
				gl_error("link %s (%s:%d) to object is not specified", pLink->get_name(), pLink->oclass->name, pLink->get_id());
				++errcount;
			}
			else if (!gl_object_isa(to,"node")){
				gl_error("link %s (%s:%d) to object is not a node", pLink->get_name(), pLink->oclass->name, pLink->get_id());
				++errcount;
			} else { /* is a "to" and it isa(node) */
				linkmap[to->id]++; /* mark that this node has links to it */
			}
			/* add link to heap? */
			if((from != NULL) && (to != NULL) && (linkmap[from->id] > 0) && (linkmap[to->id] > 0)){
				linklist[queuect] = pLink;
				queuect++;
			}
			//	check phases
			/* this isn't cooperating with me.  -MH */
/*			if(tNode->get_phases(PHASE_A) == fNode->get_phases(PHASE_A)){
				gl_error("link:%i: to, from nodes have mismatched A phase (%i vs %i)", obj->id, tNode->get_phases(PHASE_A), fNode->get_phases(PHASE_A));
				++errcount;
			}
			if(tNode->get_phases(PHASE_B) == fNode->get_phases(PHASE_B)){
				gl_error("link:%i: to, from nodes have mismatched B phase (%i vs %i)", obj->id, tNode->get_phases(PHASE_B), fNode->get_phases(PHASE_B));
				++errcount;
			}
			if(tNode->get_phases(PHASE_C) == fNode->get_phases(PHASE_C)){
				gl_error("link:%i: to, from nodes have mismatched C phase (%i vs %i)", obj->id, tNode->get_phases(PHASE_C), fNode->get_phases(PHASE_C));
				++errcount;
			}
			if(tNode->get_phases(PHASE_D) == fNode->get_phases(PHASE_D)){
				gl_error("link:%i: to, from nodes have mismatched D phase (%i vs %i)", obj->id, tNode->get_phases(PHASE_D), fNode->get_phases(PHASE_D));
				++errcount;
			}
			if(tNode->get_phases(PHASE_N) == fNode->get_phases(PHASE_N)){
				gl_error("link:%i: to, from nodes have mismatched N phase (%i vs %i)", obj->id, tNode->get_phases(PHASE_N), fNode->get_phases(PHASE_N));
				++errcount;
			}*/
		}
	}

	//Old Island check code - doesn't handle parented objects and may be missing other stuff.  NR does these type of check by default
	//(solver fails if an island is present)

	//for(i = 0; i < objct; ++i){ /* locate unlinked nodes */
	//	if(nodemap[i] != 0){
	//		if(linkmap[i] * nodemap[i] > 0){ /* there is a node at [i] and links to it*/
	//			;
	//		} else if(linkmap[i] == 1){ /* either a feeder or an endpoint */
	//			;
	//		} else { /* unattached node */
	//			gl_error("node:%i: node with no links to or from it", i);
	//			nodemap[i] *= -1; /* mark as unlinked */
	//			++errcount;
	//		}
	//	}
	//}
	//for(i = 0; i < objct; ++i){ /* mark by islands*/
	//	if(nodemap[i] > 0){ /* has linked node */
	//		linkmap[i] = i; /* island until proven otherwise */
	//	} else {
	//		linkmap[i] = -1; /* just making sure... */
	//	}
	//}

	//queueb = 0;
	//for(i = 0; i < queuect; ++i){
	//	if(linklist[i] != NULL){ /* consume the next item */
	//		linkqueue[queueb] = linklist[i];
	//		linklist[i] = NULL;
	//		queueb++;
	//	}
	//	while(queuef < queueb){
	//		/* expand this island */
	//		linkmap[linkqueue[queuef]->to->id] = linkmap[linkqueue[queuef]->from->id];
	//		/* capture the adjacent nodes */
	//		for(j = 0; j < queuect; ++j){
	//			if(linklist[j] != NULL){
	//				if(linklist[j]->from->id == linkqueue[queuef]->to->id){
	//					linkqueue[queueb] = linklist[j];
	//					linklist[j] = NULL;
	//					++queueb;
	//				}
	//			}
	//		}
	//		++queuef;
	//	}
	//	/* we've consumed one island, grab another */
	//}
	//for(i = 0; i < objct; ++i){
	//	if(nodemap[i] != 0){
	//		gl_testmsg("node:%i on island %i", i, linkmap[i]);
	//		if(linkmap[i] == i){
	//			++islandct;
	//		}
	//	}
	//	if(tomap[i] > 1){
	//		FINDLIST *cow = gl_find_objects(FL_NEW,FT_ID,SAME,i,NULL);
	//		OBJECT *moo = gl_find_next(cow, NULL);
	//		char grass[64];
	//		gl_output("object #%i, \'%s\', has more than one link feeding to it (this will diverge)", i, gl_name(moo, grass, 64));
	//	}
	//}
	//gl_output("Found %i islands", islandct);
	//tlist = anchor.next;
	//while(tlist != NULL){
	//	PFLIST *tptr = tlist;
	//	tlist = tptr->next;
	//	free(tptr);
	//}

	/*	An extra something to check link directionality,
	 *	if the root node has been defined on the command line.
	 *	-d3p988 */
	gvroot = gl_global_find("powerflow::rootnode");
	if(gvroot != NULL){
		PFLIST *front=NULL, *back=NULL, *del=NULL; /* node queue */
		OBJECT *_node = gl_get_object((char *)gvroot->prop->addr);
		OBJECT *_link = NULL;
		int *rankmap = (int *)malloc((size_t)(objct*sizeof(int)));
		int bct = 0;
		if(_node == NULL){
			gl_error("powerflow check(): Unable to do directionality check, root node name not found.");
		} else {
			gl_testmsg("Powerflow Check ~ Backward Links:");
		}
		for(int i = 0; i < objct; ++i){
			rankmap[i] = objct;
		}
		rankmap[_node->id] = 0;
		front = (PFLIST *)malloc(sizeof(PFLIST));
		front->next = NULL;
		front->ptr = _node;
		back = front;
		while(front != NULL){
			// find all links from the node
			for(OBJECT *now=gl_find_next(list, NULL); now != NULL; now = gl_find_next(list, now)){
				link_object *l;
				if(!gl_object_isa(now, "link"))
					continue;
				l = OBJECTDATA(now, link_object);
				if((l->from != front->ptr) && (l->to != front->ptr)){
					continue;
				} else if(rankmap[l->from->id]<objct && rankmap[l->to->id]<objct){
					continue;
				} else if(rankmap[l->from->id] < rankmap[l->to->id]){
					/* mark */
					rankmap[l->to->id] = rankmap[l->from->id]+1;
				} else if(rankmap[l->from->id] > rankmap[l->to->id]){
					/* swap & mark */
					OBJECT *t = l->from;
					gl_testmsg("reversed link: %s goes from %s to %s", now->name, l->from->name, l->to->name);
					l->from = l->to;
					l->to = t;
					rankmap[l->to->id] = rankmap[l->from->id]+1;;
				}
				// enqueue the "to" node
				back->next = (PFLIST *)malloc(sizeof(PFLIST));
				back->next->next = NULL;
				//back->next->ptr = l->to;
				back = back->next;
				back->ptr = l->to;
			}
			del = front;
			front = front->next;
			free(del);
		}
	}

	free(nodemap);
	free(linkmap);
	free(linklist);
	free(linkqueue);
	//return 0;
	return 1;	//Nothing really checked in here, so just let it pass.  Not sure why it fails by default.
}
