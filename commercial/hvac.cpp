/** $Id: hvac.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "hvac.h"

hvac::hvac() 
{
}

hvac::~hvac()
{
}

void hvac::create() 
{
}

TIMESTAMP hvac::sync(TIMESTAMP t0) 
{
	return TS_NEVER; 
}
void hvac::pre_update(void) 
{
}

void hvac::post_update(void) 
{
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
CDECL CLASS *hvac_class = NULL;
CDECL OBJECT *last_hvac = NULL;

EXPORT int create_hvac(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(hvac_class);
	if (*obj!=NULL)
	{
		last_hvac = *obj;
		hvac *my = OBJECTDATA(*obj,hvac);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_hvac(OBJECT *obj, TIMESTAMP t0)
{
	TIMESTAMP t1 = OBJECTDATA(obj,hvac)->sync(t0);
	obj->clock = t0;
	return t1;
}

