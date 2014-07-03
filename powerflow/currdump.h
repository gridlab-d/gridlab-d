// $Id: currdump.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _currdump_H
#define _currdump_H

#include "powerflow.h"
#include "link.h"

typedef enum {
	CDM_RECT,
	CDM_POLAR
} CDMODE;

class currdump : public gld_object
{
public:
	TIMESTAMP runtime;
	char32 group;
	char256 filename;
	int32 runcount;
	enumeration mode;
public:
	static CLASS *oclass;
public:
	currdump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t);
	int isa(char *classname);

	void dump(TIMESTAMP t);
};

#endif // _currdump_H

