/** $Id: gen_cost.h 4738 2014-07-03 00:55:39Z dchassin $
	@file gen_cost.h
	@addtogroup gen_cost
	@ingroup MODULENAME

 @{
 **/

#ifndef _gen_cost_H
#define _gen_cost_H

#include <stdarg.h>
#include "gridlabd.h"
#include <string>
using namespace std;

#define MAX_NODE 10000

class gen_cost {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	int 	MODEL;
	double	STARTUP;
	double	SHUTDOWN;
	int	NCOST;
	char	COST[MAX_NODE];	//currently, we define a large enough size. We need to modify it to a dynamic size in the future. 
// Open ticket: Issue 4 @Yizheng

public:
	/* required implementations */
	gen_cost(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static gen_cost *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
