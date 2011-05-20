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
#include "relay.h"
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
	gl_global_create("powerflow::lu_solver",PT_char256,&LUSolverName,NULL);
	gl_global_create("powerflow::acceleration_factor",PT_double,&acceleration_factor,NULL);
	gl_global_create("powerflow::NR_iteration_limit",PT_int64,&NR_iteration_limit,NULL);
	gl_global_create("powerflow::NR_superLU_procs",PT_int32,&NR_superLU_procs,NULL);
	gl_global_create("powerflow::default_maximum_voltage_error",PT_double,&default_maximum_voltage_error,NULL);

	// register each object class by creating the default instance
	new powerflow_object(module);
	new powerflow_library(module);
	new node(module);
	new link(module);
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
	new relay(module);
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

	/* always return the first class registered */
	return node::oclass;
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
	int i, j;
	GLOBALVAR *gvroot = NULL;
	PFLIST anchor, *tlist = NULL;
	link **linklist = NULL,
		 **linkqueue = NULL;

	objct = gl_get_object_count();
	anchor.ptr = NULL;
	anchor.next = NULL;

	nodemap = (int *)malloc((size_t)(objct*sizeof(int)));
	linkmap = (int *)malloc((size_t)(objct*sizeof(int)));
	tomap = (int *)malloc((size_t)(objct*sizeof(int)));
	linkqueue = (link **)malloc((size_t)(objct*sizeof(link *)));
	linklist = (link **)malloc((size_t)(objct*sizeof(link *)));
	memset(nodemap, 0, objct*sizeof(int));
	memset(linkmap, 0, objct*sizeof(int));
	memset(tomap, 0, objct*sizeof(int));
	memset(linkqueue, 0, objct*sizeof(link *));
	memset(linklist, 0, objct*sizeof(link *));
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
			link *pLink = OBJECTDATA(obj,link);
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

	for(i = 0; i < objct; ++i){ /* locate unlinked nodes */
		if(nodemap[i] != 0){
			if(linkmap[i] * nodemap[i] > 0){ /* there is a node at [i] and links to it*/
				;
			} else if(linkmap[i] == 1){ /* either a feeder or an endpoint */
				;
			} else { /* unattached node */
				gl_error("node:%i: node with no links to or from it", i);
				nodemap[i] *= -1; /* mark as unlinked */
				++errcount;
			}
		}
	}
	for(i = 0; i < objct; ++i){ /* mark by islands*/
		if(nodemap[i] > 0){ /* has linked node */
			linkmap[i] = i; /* island until proven otherwise */
		} else {
			linkmap[i] = -1; /* just making sure... */
		}
	}

	queueb = 0;
	for(i = 0; i < queuect; ++i){
		if(linklist[i] != NULL){ /* consume the next item */
			linkqueue[queueb] = linklist[i];
			linklist[i] = NULL;
			queueb++;
		}
		while(queuef < queueb){
			/* expand this island */
			linkmap[linkqueue[queuef]->to->id] = linkmap[linkqueue[queuef]->from->id];
			/* capture the adjacent nodes */
			for(j = 0; j < queuect; ++j){
				if(linklist[j] != NULL){
					if(linklist[j]->from->id == linkqueue[queuef]->to->id){
						linkqueue[queueb] = linklist[j];
						linklist[j] = NULL;
						++queueb;
					}
				}
			}
			++queuef;
		}
		/* we've consumed one island, grab another */
	}
	for(i = 0; i < objct; ++i){
		if(nodemap[i] != 0){
			gl_testmsg("node:%i on island %i", i, linkmap[i]);
			if(linkmap[i] == i){
				++islandct;
			}
		}
		if(tomap[i] > 1){
			FINDLIST *cow = gl_find_objects(FL_NEW,FT_ID,SAME,i,NULL);
			OBJECT *moo = gl_find_next(cow, NULL);
			char grass[64];
			gl_output("object #%i, \'%s\', has more than one link feeding to it (this will diverge)", i, gl_name(moo, grass, 64));
		}
	}
	gl_output("Found %i islands", islandct);
	tlist = anchor.next;
	while(tlist != NULL){
		PFLIST *tptr = tlist;
		tlist = tptr->next;
		free(tptr);
	}

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
				link *l;
				if(!gl_object_isa(now, "link"))
					continue;
				l = OBJECTDATA(now, link);
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
	return 0;
}
