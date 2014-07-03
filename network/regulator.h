/** $Id: regulator.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/


#ifndef _REGULATOR_H
#define _REGULATOR_H

#include "link.h"

class regulator : public link {
public:
	enum {RT_LTC, RT_VR} Type;
	double Vmax;
	double Vmin;
	double Vstep;
	OBJECT* CTlink;
	OBJECT* PTbus;
	double TimeDelay;	
public:
	static CLASS *oclass;
	static regulator *defaults;
	static CLASS *pclass;
public:
	regulator(MODULE *mod);
	int create();
	TIMESTAMP sync(TIMESTAMP t0);
};

GLOBAL CLASS *regulator_class INIT(NULL);

#endif
