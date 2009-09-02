// $Id: meter.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _VOLTDUMP_H
#define _VOLTDUMP_H

#include "powerflow.h"
#include "node.h"

class voltdump
{
public:
	TIMESTAMP runtime;
	char32 group;
	char32 filename;
public:
	static CLASS *oclass;
public:
	voltdump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	int commit(TIMESTAMP t);
	int isa(char *classname);

	void dump(TIMESTAMP t);
};

#endif // _VOLTDUMP_H

