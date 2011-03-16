// $Id: meter.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _VOLTDUMP_H
#define _VOLTDUMP_H

#include "powerflow.h"
#include "node.h"

typedef enum {
	VDM_RECT,
	VDM_POLAR
} VDMODE;

class voltdump
{
public:
	TIMESTAMP runtime;
	char32 group;
	char256 filename;
	int32 runcount;
	VDMODE mode;
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

