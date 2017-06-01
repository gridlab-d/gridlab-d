/** $Id: simple.h 4738 2014-07-03 00:55:39Z dchassin $
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

typedef enum {OG_EXTREMUM, OG_MINIMUM, OG_MAXIMUM} OBJECTIVEGOAL;

class simple : public gld_object {
protected:
	OBJECTIVEGOAL goal; // objective goal description
	char1024 objective; // objective variable name
	char1024 variable; // decision variable name
	char1024 constraint; // constraint description
	double delta; // delta used in calculating slopes
	double epsilon; // maximum error used in calculating completion
	int32 trials; // maximum number of trials allowed
private:
	int32 trial; // trial counter
	int32 pass; // pass number (0-2 is order estimate, 3 is constrained)
	double last_x; // last value of decision variable found
	double last_y; // last value of objective variable found
	double last_dy; // last slope of objective variable found
	double next_x; // next value of decision variable to use
	double *pObjective; // objective variable
	double *pVariable; // decision variable
	double *pConstraint; // constraint variable
	struct {
		bool (*op)(double,double); // constraint operation
		double value; // constraint value
	} constrain; // describe a constraint
	bool constraint_broken(double x); // detect constraint
	double search_step; // use to deal with constraints
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
