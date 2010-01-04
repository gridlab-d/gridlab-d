// $Id: motor.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _INDUCTION_MOTOR_H
#define _INDUCTION_MOTOR_H

#include "powerflow.h"
#include "node.h"

class motor : public node
{
public:

protected:

private:

public:
	int create(void);
	TIMESTAMP presync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);
	TIMESTAMP sync(TIMESTAMP t0);
	motor(MODULE *mod);
	inline motor(CLASS *cl=oclass):node(cl){};
	int init(OBJECT *parent);
	int isa(char *classname);
	static CLASS *pclass;
	static CLASS *oclass;

};

#endif