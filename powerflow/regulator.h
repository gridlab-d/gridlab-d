// $Id: regulator.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _REGULATOR_H
#define _REGULATOR_H

#include "powerflow.h"
#include "link.h"
#include "regulator_configuration.h"

class regulator : public link
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
	
public:
	OBJECT *configuration;
	regulator(MODULE *mod);
	inline regulator(CLASS *cl=oclass):link(cl){};
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
};

#endif // _REGULATOR_H
