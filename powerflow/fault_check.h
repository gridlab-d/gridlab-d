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

	FCSTATE fcheck_state;		//Mode variable
	char1024 output_filename;	//File name to output unconnected bus values
	bool reliability_mode;		//Flag for reliability implementation

	fault_check(MODULE *mod);
	fault_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void search_links(unsigned int node_int);	//Function to check connectivity and support of nodes
	void support_check(unsigned int swing_node_int, bool rest_pop_tree);	//Function that performs the connectivity check - this way so can be easily externally accessed
	void reset_support_check(void);		//Function to re-init the support matrix
	void write_output_file(TIMESTAMP tval);		//Function to write out "unsupported" items

	TIMESTAMP sync(TIMESTAMP t0);


private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
};

#endif // _FAULT_CHECK_H
