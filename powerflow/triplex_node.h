// $Id: triplex_node.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXNODE_H
#define _TRIPLEXNODE_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_triplex_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class triplex_node : public node
{
public:
	enum { 
		ND_OUT_OF_SERVICE = 0, ///< out of service flag for nodes
		ND_IN_SERVICE = 1,     ///< in service flag for nodes - default
	};
	enumeration service_status;
	static CLASS *pclass;
	static CLASS *oclass;
	
	triplex_node(MODULE *mod);
	inline triplex_node(CLASS *cl=oclass):node(cl){};

private:
	//************* NOTE - these can probably be deleted/removed when
	//deprecated properties of triplex_node are removed  ***************//
	gld::complex impedance[3];	///< impedance load value
	gld::complex pub_shunt[3];	///< shunt impedance load value
	
	gld::complex pub_power[3];	//< power load value - published (replaced by triplex_load)
	gld::complex pub_current[4];	//< current load value - published (replaced by triplex_load)

	//Tracker - works like triplex_load code, but will be irrelevant once deprecated code removed
	gld::complex prev_node_load_values[2][3];		//Current handled separately
	gld::complex prev_node_load_current_values[4];	//Current is bigger

	//************ End deprecated note region *****************//
public:
	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);

	void BOTH_triplex_node_sync_fxn(void);

	SIMULATIONMODE inter_deltaupdate_triplex_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	friend class triplex_line;
};

#endif // _TRIPLEXNODE_H

