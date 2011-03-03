// $Id: fault_check.h 2009-12-15 15:00:00Z d3x593 $
//	Copyright (C) 2009 Battelle Memorial Institute

#ifndef _FAULT_CHECK_H
#define _FAULT_CHECK_H

#include "powerflow.h"

class fault_check : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:
	//Move typedef in here so reliabilty-related items can access the enumeration
	typedef enum {
			SINGLE=0,		//Runs one fault_check, right at the beginning of powerflow
			ONCHANGE=1,	//Runs fault_check everytime a Jacobian reconfiguration is requested
			ALLT=2			//Runs fault_check on every iteration
			} FCSTATE;

	int **Supported_Nodes;		//Nodes with source support (connected to swing somehow)
	char *Alteration_Nodes;		//Similar to Supported_Nodes, but used to track alteration progression (namely, has the side been handled)

	FCSTATE fcheck_state;		//Mode variable
	char1024 output_filename;	//File name to output unconnected bus values
	bool reliability_mode;		//Flag for reliability implementation
	OBJECT *rel_eventgen;		//Eventgen object in reliability - allows "unscheduled" faults

	fault_check(MODULE *mod);
	fault_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void search_links(int node_int);	//Function to check connectivity and support of nodes
	void support_check(int swing_node_int, bool rest_pop_tree);	//Function that performs the connectivity check - this way so can be easily externally accessed
	void reset_support_check(void);		//Function to re-init the support matrix
	void write_output_file(TIMESTAMP tval);		//Function to write out "unsupported" items

	void support_check_alterations(int baselink_int, bool rest_mode);	//Function to update powerflow for "no longer supported" devices
	void reset_alterations_check(void);									//Function to re-init the alterations support matrix - mainly used to see if ends have been touched or not

	void support_search_links(int node_int, int node_start, bool impact_mode);	//Function to check connectivity and support of nodes and remove/restore components as necessary
	void momentary_activation(int node_int);	//Function to progress down a branch and flag momentary outages

	TIMESTAMP sync(TIMESTAMP t0);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
};

EXPORT int powerflow_alterations(OBJECT *thisobj, int baselink,bool rest_mode);
EXPORT double handle_sectionalizer(OBJECT *thisobj, int sectionalizer_number);

#endif // _FAULT_CHECK_H
