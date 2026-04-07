// $Id: triplex_node.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXNODE_H
#define _TRIPLEXNODE_H

#include "node.h"

EXPORT SIMULATIONMODE interupdate_triplex_node(OBJECT *obj, unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

class triplex_node : public node
{
public:
	static CLASS *pclass;
	static CLASS *oclass;
	
	triplex_node(MODULE *mod);
	inline triplex_node(CLASS *cl=oclass):node(cl){};

	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);

	SIMULATIONMODE inter_deltaupdate_triplex_node(unsigned int64 delta_time, unsigned long dt, unsigned int iteration_count_val, bool interupdate_pos);

	friend class triplex_line;
};

#endif // _TRIPLEXNODE_H

