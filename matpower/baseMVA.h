/** $Id: baseMVA.h 4738 2014-07-03 00:55:39Z dchassin $
	@file baseMVA.h
	@addtogroup baseMVA
	@ingroup MODULENAME

 @{
 **/

#ifndef _baseMVA_H
#define _baseMVA_H

#include <stdarg.h>
#include "gridlabd.h"

class baseMVA {
private:
	/* TODO: put private variables here */
protected:
	/* TODO: put unpublished but inherited variables */
public:
	/* TODO: put published variables here */
public:
	/* required implementations */
	baseMVA(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static baseMVA *defaults;

	//variables
	int BASEMVA;
#ifdef OPTIONAL
	static CLASS *pclass; /**< defines the parent class */
	TIMESTAMP plc(TIMESTAMP t0, TIMESTAMP t1); /**< defines the default PLC code */
#endif
};

#endif

/**@}*/
