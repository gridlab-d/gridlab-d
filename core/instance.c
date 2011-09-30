/* instance.c
   Copyright (C) 2011, Battelle Memorial Institute
 */

#include "instance.h"

static instance *instance_list = NULL;

int convert_to_instance(char *string, void *data, PROPERTY *prop)
{
	/* @todo */
	return 0;
}

int convert_from_instance(char *string,int size,void *data, PROPERTY *prop)
{
	/* @todo */
	return 0;
}
int instance_create(instance *inst)
{
	/* @todo */
	return 0;
}
int instance_init(instance *inst)
{
	/* @todo */
	return 0;
}

int instance_initall(void)
{
	/* @todo */
	return 0;
}

TIMESTAMP instance_sync(instance *inst, TIMESTAMP t1)
{
	/* @todo */
	return TS_NEVER;
}
TIMESTAMP instance_syncall(TIMESTAMP t1)
{
	/* @todo */
	return TS_NEVER;
}
