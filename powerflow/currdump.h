// $Id: meter.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _currdump_H
#define _currdump_H

#include "powerflow.h"
#include "link.h"

typedef enum {
	CDM_RECT,
	CDM_POLAR
} CDMODE;

class currdump
{
public:
	TIMESTAMP runtime;
	char32 group;
	char256 filename;
	int32 runcount;
	CDMODE mode;
public:
	static CLASS *oclass;
public:
	currdump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	int commit(TIMESTAMP t);
	int isa(char *classname);

	void dump(TIMESTAMP t);
};

#endif // _currdump_H

