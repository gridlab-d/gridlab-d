// $Id: sync_check.h 
//	Copyright (C) 2020 Battelle Memorial Institute

#ifndef _SYNC_CHECK_H
#define _SYNC_CHECK_H

#include "powerflow.h"

class sync_check : public powerflow_object
{
public:
	static CLASS *oclass;
	static CLASS *pclass;
public:

	sync_check(MODULE *mod);
	sync_check(CLASS *cl=oclass):powerflow_object(cl){};
	int create(void);
	int init(OBJECT *parent=NULL);
	int isa(char *classname);

	TIMESTAMP presync(TIMESTAMP t0);
    TIMESTAMP sync(TIMESTAMP t0);
	TIMESTAMP postsync(TIMESTAMP t0);

private:
    bool arm_sync;

};

#endif // _SYNC_CHECK_H
