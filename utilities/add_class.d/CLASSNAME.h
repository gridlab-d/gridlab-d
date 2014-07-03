/** $Id: CLASSNAME.h 4738 2014-07-03 00:55:39Z dchassin $
	@file CLASSNAME.h
	@addtogroup CLASSNAME
	@ingroup MODULENAME

 @{
 **/

#ifndef _CLASSNAME_H
#define _CLASSNAME_H

#include <stdarg.h>
#include "gridlabd.h"

class CLASSNAME {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	CLASSNAME(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static CLASSNAME *defaults;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
