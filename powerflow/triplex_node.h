// $Id: triplex_node.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXNODE_H
#define _TRIPLEXNODE_H

#include "node.h"

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

public:

	complex impedance[3];	///< impedance load value
	complex pub_shunt[3];	///< shunt impedance load value
	int create(void);
	int init(OBJECT *parent=NULL);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	int isa(char *classname);

	friend class triplex_line;
};

#endif // _TRIPLEXNODE_H

