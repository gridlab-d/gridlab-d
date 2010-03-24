// $Id: triplex_node.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXNODE_H
#define _TRIPLEXNODE_H

#include "node.h"

class triplex_node : public node
{
public:
	static CLASS *pclass;
	static CLASS *oclass;
	
	triplex_node(MODULE *mod);
	inline triplex_node(CLASS *cl=oclass):node(cl){};

public:

	complex impedance[3];
	complex pub_shunt[3];
	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	int isa(char *classname);

	bool NR_mode;

	friend class triplex_line;
};

#endif // _TRIPLEXNODE_H

