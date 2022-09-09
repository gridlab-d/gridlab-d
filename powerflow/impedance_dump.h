// $Id: impedance_dump.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _impedance_dump_H
#define _impedance_dump_H

#include "powerflow.h"
#include "line.h"
#include "transformer.h"
#include "node.h"
#include "regulator.h"
#include "capacitor.h"
#include "switch_object.h"

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
	regulator **pRegulator;
	link_object **pRelay;
	link_object **pSectionalizer;
	link_object **pSeriesReactor;
	switch_object **pSwitch;
	transformer **pTransformer;
	line **pTpLine;
	line **pUgLine;
	capacitor **pCapacitor;
	TIMESTAMP runtime;
	int32 runcount;
	gld::complex *node_voltage;
public:
	static CLASS *oclass;
public:
	impedance_dump(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t);
	static int isa(const char *classname);
	int dump(TIMESTAMP t);
	static gld::complex *get_complex(OBJECT *obj, const char *name);
};

#endif // _impedance_dump_H

