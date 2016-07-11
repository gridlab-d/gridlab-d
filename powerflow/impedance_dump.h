// $Id: impedance_dump.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _impedance_dump_H
#define _impedance_dump_H

#include "powerflow.h"
#include "line.h"
#include "transformer.h"
#include "node.h"

typedef enum {
	IDM_RECT,
	IDM_POLAR
} IDMODE;

class impedance_dump : public gld_object
{
public:
	int first_run;
	char32 group;
	char256 filename;
	link_object **pFuse;
	line **pOhLine;
	link_object **pRecloser;
	link_object **pRegulator;
	link_object **pRelay;
	link_object **pSectionalizer;
	link_object **pSeriesReactor;
	link_object **pSwitch;
	transformer **pTransformer;
	line **pTpLine;
	line **pUgLine;
	TIMESTAMP runtime;
	int32 runcount;
	complex *node_voltage;
public:
	static CLASS *oclass;
public:
	impedance_dump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t);
	int isa(char *classname);
	int dump(TIMESTAMP t);
	complex *get_complex(OBJECT *obj, char *name);
};

#endif // _impedance_dump_H

