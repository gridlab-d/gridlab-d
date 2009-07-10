/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file loadshape.c
	@addtogroup loadshape
**/

#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

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
	double sum = fabs(e->current_fraction) + fabs(e->impedance_fraction) + fabs(e->power_fraction);

	/* TODO initialize enduse data */
	if (sum==0)
	{
		output_error("enduse_init(...) sum of zip fractions is zero");
		return 1;
	}

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

int convert_from_enduse(char *string,int size,void *data, PROPERTY *prop)
{
	return 0;
}

int convert_to_enduse(char *string, void *data, PROPERTY *prop)
{
	enduse *e = (enduse*)data;
	char buffer[1024];
	char *token = NULL;

	/* check string length before copying to buffer */
	if (strlen(string)>sizeof(buffer)-1)
	{
		output_error("convert_to_loadshape(string='%-.64s...', ...) input string is too long (max is 1023)",string);
		return 0;
	}
	strcpy(buffer,string);

	/* parse tuples separate by semicolon*/
	while ((token=strtok(token==NULL?buffer:NULL,";"))!=NULL)
	{
		/* colon separate tuple parts */
		char *param = token;
		char *value = strchr(token,':');

		/* isolate param and token and eliminte leading whitespaces */
		while (isspace(*param) || iscntrl(*param)) param++;		
		if (value==NULL)
			value="1";
		else
			*value++ = '\0'; /* separate value from param */
		while (isspace(*value) || iscntrl(*value)) value++;

		// parse params
		if (strcmp(param,"current_fraction")==0)
			e->current_fraction = atof(value);
		else if (strcmp(param,"impedance_fraction")==0)
			e->impedance_fraction = atof(value);
		else if (strcmp(param,"power_fraction")==0)
			e->power_fraction = atof(value);
		else if (strcmp(param,"power_factor")==0)
			e->power_factor = atof(value);
		else if (strcmp(param,"loadshape")==0)
		{
			/* TODO resolve loadshape */
			output_warning("convert_to_enduse(string='%-.64s...', ...) unable to link loadshape '%s' (not implemented)",string,value);
		}
		else
		{
			output_error("convert_to_enduse(string='%-.64s...', ...) parameter '%s' is not valid",string,param);
			return 0;
		}
	}

	/* reinitialize the loadshape */
	if (enduse_init((enduse*)data))
		return 0;

	/* everything converted ok */
	return 1;
}