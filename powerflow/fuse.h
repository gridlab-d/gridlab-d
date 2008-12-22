// $Id: fuse.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _FUSE_H
#define _FUSE_H

#include "powerflow.h"
#include "relay.h"

class fuse : public link
{
protected:
	TIMESTAMP blow_time;
public:
    static CLASS *oclass;
    static CLASS *pclass;

public:
	int time_curve;   // equation, current vs time curve
	double current_limit;	/// maximum current

	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
	fuse(MODULE *mod);
	inline fuse(CLASS *cl=oclass):link(cl){};
	int isa(char *classname);
};

#endif // _FUSE_H
