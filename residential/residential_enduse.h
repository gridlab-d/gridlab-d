/** $Id: residential_enduse.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file residential_enduse.h
	@addtogroup residential_enduse

 @{
 **/

#ifndef _RESIDENTIALENDUSE_H
#define _RESIDENTIALENDUSE_H

#include "residential.h"

typedef enum {
	OV_NORMAL = 0,
	OV_ON = 1,
	OV_OFF = 2,
} OVERRIDE;

typedef enum {
	PS_OFF=0,
	PS_ON=1,
	PS_UNKNOWN=2,
} POWERSTATE;

class residential_enduse : public gld_object
{
public:
	CIRCUIT *pCircuit;	// pointer to circuit data
	enduse load;	// load data
	loadshape shape;
	enumeration re_override;
	enumeration power_state;
public:
	static CLASS *oclass;
	residential_enduse(MODULE *mod);
	int create(bool connect_shape=true);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);

};

#endif // _RESIDENTIALENDUSE_H

/**@}**/
