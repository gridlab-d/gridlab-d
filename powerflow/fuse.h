// $Id: fuse.h 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _FUSE_H
#define _FUSE_H

#include "powerflow.h"
#include "relay.h"

#define Open_Z 1000000;	//Open impedance - 100 MOhm

class fuse : public link
{
public:
    static CLASS *oclass;
    static CLASS *pclass;

public:
	typedef enum {GOOD=0, BLOWN=1} FUSESTATE;

	int create(void);
	int init(OBJECT *parent);
	int fuse_state(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0);
	fuse(MODULE *mod);
	inline fuse(CLASS *cl=oclass):link(cl){};
	int isa(char *classname);

	double current_limit;
	double mean_replacement_time;
	TIMESTAMP fix_time_A;
	TIMESTAMP fix_time_B;
	TIMESTAMP fix_time_C;
	FUSESTATE phase_A_status;
	FUSESTATE phase_B_status;
	FUSESTATE phase_C_status;

private:
	TIMESTAMP Prev_Time;
	void fuse_check(set phase_to_check, complex *fcurr, complex *curr);
};

#endif // _FUSE_H
