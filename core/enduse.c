/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape
**/

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "platform.h"
#include "output.h"
#include "loadshape.h"
#include "exception.h"
#include "convert.h"
#include "globals.h"
#include "random.h"
#include "schedule.h"
#include "enduse.h"

static enduse *enduse_list = NULL;

int enduse_create(void *data)
{
	enduse *e = (enduse*)data;
	e->shape = NULL;
	e->next = enduse_list;
	enduse_list = e;
	return 1;
}

int enduse_init(enduse *e)
{
	double sum = e->current_zip + e->impedance_zip + e->power_zip;

	/* TODO initialize enduse data */
	if (sum==0)
	{
		output_error("enduse_init(...) zip not specified");
		return 1;
	}
	e->current_zip /= sum;
	e->impedance_zip /= sum;
	e->power_zip /= sum;

	return 0;
}

int enduse_initall(void)
{
	enduse *e;
	for (e=enduse_list; e!=NULL; e=e->next)
	{
		if (enduse_init(e)==1)
			return FAILED;
	}
	return SUCCESS;
}

TIMESTAMP enduse_sync(enduse *e, TIMESTAMP t1)
{
	/* TODO update power, energy, demand, etc. from shape->load */
	return TS_NEVER;
}

TIMESTAMP enduse_syncall(TIMESTAMP t1)
{
	return TS_NEVER;
}

int convert_to_enduse(char *string, void *data, PROPERTY *prop)
{
	return 0;
}

int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop)
{
	return 0;
}