/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file residential_enduse.h
	@addtogroup residential_enduse

 @{
 **/

#ifndef _RESIDENTIALENDUSE_H
#define _RESIDENTIALENDUSE_H

#include "residential.h"

class residential_enduse  
{
public:
	CIRCUIT *pCircuit;	// pointer to circuit data
	enduse load;	// load data
	loadshape shape;

public:
	static CLASS *oclass;
	residential_enduse(MODULE *mod);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _RESIDENTIALENDUSE_H

/**@}**/
