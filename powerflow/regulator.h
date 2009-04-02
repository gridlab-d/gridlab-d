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
	double VtapChange;
	double tapChangePer;
	double Vlow;
	double Vhigh;
	complex V2[3], Vcomp[3];
	int16 tap[3];
	complex volt[3];
	complex D_mat[3][3];
	complex W_mat[3][3];
	complex curr[3];

public:
	static CLASS *oclass;
	static CLASS *pclass;
	
public:
	OBJECT *configuration;
	regulator(MODULE *mod);
	inline regulator(CLASS *cl=oclass):link(cl){};
	int create(void);
	int init(OBJECT *parent);
	//TIMESTAMP presync(TIMESTAMP t0);
	//TIMESTAMP sync(TIMESTAMP t0);
	//TIMESTAMP postsync(TIMESTAMP t0);
	int isa(char *classname);
};

#endif // _REGULATOR_H
