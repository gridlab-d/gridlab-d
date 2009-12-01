/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file residential_enduse.h
	@addtogroup residential_enduse

 @{
 **/

#ifndef _RESIDENTIALENDUSE_H
#define _RESIDENTIALENDUSE_H

#include "residential.h"

typedef enum {
	OV_ON = 1,
	OV_NORMAL = 0,
	OV_OFF = -1,
} OVERRIDE;

class residential_enduse  
{
public:
	CIRCUIT *pCircuit;	// pointer to circuit data
	enduse load;	// load data
	loadshape shape;
	OVERRIDE override;

public:
	static CLASS *oclass;
	residential_enduse(MODULE *mod);
	int create(bool connect_shape=true);
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _RESIDENTIALENDUSE_H

/**@}**/
