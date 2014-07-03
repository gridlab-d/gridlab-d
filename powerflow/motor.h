// $Id: motor.h 4738 2014-07-03 00:55:39Z dchassin $
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