// $Id: billdump.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _BILLDUMP_H
#define _BILLDUMP_H

#include "powerflow.h"
#include "node.h"
#include "triplex_meter.h"
#include "meter.h"

class billdump : public gld_object
{
public:
	TIMESTAMP runtime;
	char32 group;
	char256 filename;
	int32 runcount;
	typedef enum {
		METER_TP,		
		METER_3P,		
	} METERTYPE;
	enumeration meter_type;
public:
	static CLASS *oclass;
public:
	billdump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t);
	int isa(char *classname);

	void dump(TIMESTAMP t);
};

#endif // _BILLDUMP_H

