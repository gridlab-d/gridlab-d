/** $Id: line.h 683 2008-06-18 20:16:29Z d3g637 $
	@file line.h
	@addtogroup line
	@ingroup MODULENAME

 @{
 **/

#ifndef _line_H
#define _line_H

//#include <iostream.h>
#include <stdarg.h>
#include "gridlabd.h"

class line {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
	static CLASS *oclass;
	static line *defaults;
	
	// Variables
	int	F_BUS;
	int	T_BUS;
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
public:
	/* required implementations */
	line(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);


#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
