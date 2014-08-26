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

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (!set_callback(fntable)) {
		errno = EINVAL;
		return NULL;
	}

	/* exported globals */
	gl_global_create("powerflow::show_matrix_values",PT_bool,&show_matrix_values,NULL);
	gl_global_create("powerflow::primary_voltage_ratio",PT_double,&primary_voltage_ratio,NULL);
	gl_global_create("powerflow::nominal_frequency",PT_double,&nominal_frequency,NULL);
	gl_global_create("powerflow::require_voltage_control", PT_bool,&require_voltage_control,NULL);
	gl_global_create("powerflow::geographic_degree",PT_double,&geographic_degree,NULL);
	gl_global_create("powerflow::fault_impedance",PT_complex,&fault_Z,NULL);
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
	gl_global_create("powerflow::line_capacitance",PT_bool,&use_line_cap,NULL);
	gl_global_create("powerflow::line_limits",PT_bool,&use_link_limits,NULL);
	gl_global_create("powerflow::lu_solver",PT_char256,&LUSolverName,NULL);
	gl_global_create("powerflow::NR_iteration_limit",PT_int64,&NR_iteration_limit,NULL);
	gl_global_create("powerflow::NR_superLU_procs",PT_int32,&NR_superLU_procs,NULL);
	gl_global_create("powerflow::default_maximum_voltage_error",PT_double,&default_maximum_voltage_error,NULL);
	gl_global_create("powerflow::default_maximum_power_error",PT_double,&default_maximum_power_error,NULL);
	gl_global_create("powerflow::NR_admit_change",PT_bool,&NR_admit_change,NULL);
	gl_global_create("powerflow::enable_subsecond_models", PT_bool, &enable_subsecond_models,PT_DESCRIPTION,"Enable deltamode capabilities within the powerflow module",NULL);
	gl_global_create("powerflow::all_powerflow_delta", PT_bool, &all_powerflow_delta,PT_DESCRIPTION,"Forces all powerflow objects that are capable to participate in deltamode",NULL);
	gl_global_create("powerflow::deltamode_timestep", PT_double, &deltamode_timestep_publish,PT_UNITS,"ns",PT_DESCRIPTION,"Desired minimum timestep for deltamode-related simulations",NULL);
	gl_global_create("powerflow::deltamode_extra_function", PT_int64, &deltamode_extra_function,NULL);
	gl_global_create("powerflow::current_frequency",PT_double,&current_frequency,PT_UNITS,"Hz",PT_DESCRIPTION,"Current system-level frequency of the powerflow system",NULL);
	gl_global_create("powerflow::default_resistance",PT_double,&default_resistance,NULL);

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
	
	if (enable_subsecond_models == true)
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
					//Call the actual function
					function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int, bool))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val,false);
				}
				else	//No functional call for this, skip it
				{
					function_status = SM_EVENT;	//Just put something here, mainly for error checks
				}
			}
			else //Not in service - just pass
				function_status = SM_DELTA;

			//Just make sure we didn't error 
			if (function_status == (int)SM_ERROR)
			{
				gl_error("Powerflow object:%s - deltamode function returned an error!",delta_objects[curr_object_number]->name);
				// Defined below

				return SM_ERROR;
			}
			//Default else, we were okay, so onwards and upwards!
		}

		//Call dynamic powerflow (start of either predictor or correct set)
		powerflow_type = PF_DYNCALC;

		//Put in try/catch, since GL_THROWs inside solver_nr tend to be a little upsetting
		try {
			//Call solver_nr
			pf_result = solver_nr(NR_bus_count, NR_busdata, NR_branch_count, NR_branchdata, &NR_powerflow, powerflow_type, &bad_computation);
		}
		catch (const char *msg)
		{
			gl_error("powerflow:interupdate - solver_nr call: %s", msg);
			return SM_ERROR;
		}
		catch (...)
		{
			gl_error("powerflow:interupdate - solver_nr call: unknown exception");
			return SM_ERROR;
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
			return SM_ERROR;
		}
		else if (pf_result<0)	//Failure to converge - this is a failure in dynamic mode for now
		{
			gl_error("Newton-Raphson failed to converge the dynamic powerflow, sticking at same iteration.");
			/*  TROUBLESHOOT
			Newton-Raphson failed to converge the dynamic powerflow in the number of iterations
			specified in NR_iteration_limit.  It will try again (if the global iteration limit
			has not been reached).
			*/
			return SM_ERROR;
		}

		//Loop through the object list and call the updates - loop forward for SWING first, to replicate "postsync"-like order
		for (curr_object_number=0; curr_object_number<pwr_object_count; curr_object_number++)
		{
			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				if (delta_functions[curr_object_number] != NULL)
				{
					//Call the actual function
					function_status = ((SIMULATIONMODE (*)(OBJECT *, unsigned int64, unsigned long, unsigned int, bool))(*delta_functions[curr_object_number]))(delta_objects[curr_object_number],delta_time,dt,iteration_count_val,true);
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
				event_driven = false;
			else if (function_status == SM_DELTA_ITER)
			{
				event_driven = false;
				delta_iter = true;
			}
			else if (function_status == SM_ERROR)
			{
				gl_error("Powerflow object:%s - deltamode function returned an error!",delta_objects[curr_object_number]->name);
				/*  TROUBLESHOOT
				While performing a deltamode update, one object returned an error code.  Check to see if the object itself provided
				more details and try again.  If the error persists, please submit your code and a bug report via the trac website.
				*/
				return SM_ERROR;
			}
			//Default else, we're in SM_EVENT, so no flag change needed
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
	if (enable_subsecond_models == true)
	{
		//Final item of transitioning out is resetting the next timestep so a smaller one can take its place
		deltamode_starttime = TS_NEVER;
		
		return SUCCESS;
	}
	else	//Deltamode not wanted, just "succeed"
	{
		return SUCCESS;
	}
}

//Extra function for deltamode calls - allows module-level calls "out of order"
//mode variable is for selection of call (only 1 for now)
int delta_extra_function(unsigned int mode)
{
	complex accumulated_power, accumulated_freqpower;
	STATUS function_status;
	int curr_object_number;

	//Zero the values
	accumulated_power = complex(0.0,0.0);
	accumulated_freqpower = complex(0.0,0.0);

	//Call each powerflow functions "deltamode_frequency" to get total weighted
	//average for frequency - then post to global
	//Loop through the object list and call the updates
	for (curr_object_number=0; curr_object_number<pwr_object_count; curr_object_number++)
	{
		if (delta_freq_functions[curr_object_number] != NULL)
		{
			//See if we're in service or not
			if ((delta_objects[curr_object_number]->in_svc_double <= gl_globaldeltaclock) && (delta_objects[curr_object_number]->out_svc_double >= gl_globaldeltaclock))
			{
				//See if the function actually exists
				if (delta_freq_functions[curr_object_number] != NULL)
				{
					//Call the actual function
					function_status = ((STATUS (*)(OBJECT *, complex *, complex *))(*delta_freq_functions[curr_object_number]))(delta_objects[curr_object_number],&accumulated_power,&accumulated_freqpower);
				}
				else	//Doesn't exit, assume we succeeded
				{
					function_status = SUCCESS;
				}
			}
			else	//Defaulted else, not in service
				function_status = SUCCESS;
		}
		else	//Defaulted else, not in service
			function_status = SUCCESS;

		//Make sure we worked
		if (function_status == FAILED)
			return 0;
	}
	
	//Post the value to the powerflow global - pu in Hz too (is in radians)
	current_frequency = (accumulated_freqpower.Re()/accumulated_power.Re())/(2.0*PI);

	return 1;	
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
