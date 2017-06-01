/** $Id: branch.h 4738 2014-07-03 00:55:39Z dchassin $
	@file branch.h
	@addtogroup branch
	@ingroup MODULENAME
	author: Kyle Anderson (GridSpice Project),  kyle.anderson@stanford.edu
	Released in Open Source to Gridlab-D Project


 @{
 **/

#ifndef _branch_H
#define _branch_H


#define MAXSIZE 10000

#include <stdarg.h>
#include "gridlabd.h"

class branch {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	// Variables
	int	F_BUS;
	int	T_BUS;
	char	from[MAXSIZE];
	char	to[MAXSIZE];	
	double	BR_R;
	double 	BR_X;
	double 	BR_B;
	double 	RATE_A;
	double 	RATE_B;
	double 	RATE_C;
	double 	TAP;
	double 	SHIFT;
	int	BR_STATUS;
	double	ANGMIN;
	double 	ANGMAX;
	// only for the output
	double	PF;	//real power injected at “from” bus end (MW)
	double	QF;	//reactive power injected at “from” bus end (MVAr)
	double	PT;	//real power injected at “to” bus end (MW)
	double	QT;	//reactive power injected at “to” bus end (MVAr)
	double	MU_SF;	//Kuhn-Tucker multiplier on MVA limit at “from” bus (u/MVA)
	double	MU_ST;	//Kuhn-Tucker multiplier on MVA limit at “to” bus (u/MVA)
	double	MU_ANGMIN;	//Kuhn-Tucker multiplier lower angle difference limit (u/degree)
	double	MU_ANGMAX;	//Kuhn-Tucker multiplier upper angle difference limit (u/degree)
public:
	/* required implementations */
	branch(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static branch *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
