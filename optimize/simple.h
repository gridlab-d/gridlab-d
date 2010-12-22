/** $Id: auction.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file simple.h
	@addtogroup optimize
	@ingroup simple

 @{
 **/

#ifndef _SIMPLE_H
#define _SIMPLE_H

#include <stdarg.h>
#include "gridlabd.h"
#include "optimize.h"

class simple {
protected:
	char1024 objective;
	char1024 variable;
	char1024 constraint;
	double delta;
	double epsilon;
	int32 trials;
private:
	int32 trial;
	int32 pass;
	double last_x;
	double last_y;
	double last_dy;
	double next_x;
	double *pObjective;
	double *pVariable;
	double *pConstraint;
	typedef enum {LT,LE,EQ,GE,GT,NE} CONSTRAINTOP;
	struct {
		CONSTRAINTOP op;
		double value;
	} constrain;
public:
	/* required implementations */
	simple(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static simple *defaults;
};

#endif

/**@}*/
