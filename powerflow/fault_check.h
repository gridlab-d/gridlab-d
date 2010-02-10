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
	int *Supported_Nodes;		//Nodes with source support (connected to swing somehow)

	fault_check(MODULE *mod);
	fault_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);
	void search_links(unsigned int node_int);	//Function to check connectivity and support of nodes
	void support_check(unsigned int swing_node_int, bool rest_pop_tree);	//Function that performs the connectivity check - this way so can be easily externally accessed

	TIMESTAMP sync(TIMESTAMP t0);

private:
	TIMESTAMP prev_time;	//Previous timestamp - mainly for intialization
};

#endif // _FAULT_CHECK_H
