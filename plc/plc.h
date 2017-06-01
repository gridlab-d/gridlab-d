/* $Id: plc.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	Defines a programmable logical controller (PLC)
 */

#ifndef _PLC_H
#define _PLC_H

#include <stdarg.h>
#include "gridlabd.h"

#include "machine.h"

class plc {
public:
	char1024 source;
	object network;
private:
	machine *controller;
public:
	static CLASS *oclass;
	static plc* defaults;
public:
	plc(MODULE *mod);
	~plc(void);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

	inline machine *get_machine() {return controller;};
};

#endif
