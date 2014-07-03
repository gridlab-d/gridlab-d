/** $Id: meter.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _METER_H
#define _METER_H

#include "node.h"

class meter : public node {
private:
public:
	enum {MT_ONEPHASE=0, MT_THREEPHASE=1} type;
	complex meterkWh; //kWh
	complex demand;  //kW

	// constant current loads
	complex line1_current; //A
	complex line2_current;
	complex line3_current;

	// constant admittance loads
	complex line1_admittance; //Y
	complex line2_admittance;
	complex line3_admittance;

	// constant power loads
	complex line1_power; //S
	complex line2_power;
	complex line3_power;
public:
	static CLASS *oclass;
	static meter *defaults;
	static CLASS *pclass;
public:
	meter(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);

};

GLOBAL CLASS *meter_class INIT(NULL);

#endif
