/** $Id: gen.h 4738 2014-07-03 00:55:39Z dchassin $
	@file gen.h
	@addtogroup gen
	@ingroup MODULENAME

 @{
 **/

#ifndef _gen_H
#define _gen_H

#include <stdarg.h>
#include "gridlabd.h"

#define MAX_NODE 10000

class gen {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	static CLASS *oclass;
	static gen *defaults;

	// variable
	int	GEN_BUS;
	double	PG;
	double 	QG;
	double 	QMAX;
	double 	QMIN;
	double 	VG;
	double	MBASE;
	double	GEN_STATUS;
	double	PMAX;
	double	PMIN;
	double 	PC1;
	double 	PC2;
	double 	QC1MIN;
	double 	QC1MAX;
	double	QC2MIN;
	double 	QC2MAX;
	double 	RAMP_AGC;
	double 	RAMP_10;
	double 	RAMP_30;
	double	RAMP_Q;
	double	APF;
	// Only used in Output
	double	MU_PMAX; //Kuhn-Tucker multiplier on upper Pg limit (u/MW)
	double	MU_PMIN; //Kuhn-Tucker multiplier on lower Pg limit (u/MW)
	double	MU_QMAX; //Kuhn-Tucker multiplier on upper Qg limit (u/MVAr)
	double 	MU_QMIN; //Kuhn-Tucker multiplier on lower Qg limit (u/MVAr)

	// Price Info
	double  Price;  // from the gen_cost

	// Cost Info
	int 	MODEL;
	double	STARTUP;
	double	SHUTDOWN;
	int	NCOST;
	char	COST[MAX_NODE];	//currently, we define a large enough size. We need to modify it to a dynamic size in the future. 
// Open ticket: Issue 4 @Yizheng
public:
	/* required implementations */
	gen(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
