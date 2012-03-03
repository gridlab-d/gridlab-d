/* transform.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include <pthread.h>

#include "platform.h"
#include "object.h"
#include "output.h"
#include "schedule.h"
#include "transform.h"
#include "exception.h"
#include "module.h"

static TRANSFORM *schedule_xformlist=NULL;

TRANSFORM *transform_getnext(TRANSFORM *xform)
{
	return xform?xform->next:schedule_xformlist;
}

int transform_add_function(TRANSFORMSOURCE stype,	/* specifies the type of source */
						   double *source,		    /* pointer to the source value */
						   double *target,			/* pointer to the target value */
						   char *function,			/* function name */
						   OBJECT *obj,				/* object containing target value */
						   PROPERTY *prop,			/* property associated with target value */
						   SCHEDULE *sched)			/* schedule object assoicated with target value, if stype == XS_SCHEDULE */
{
	TRANSFORM *xform = (TRANSFORM*)malloc(sizeof(TRANSFORM));
	if (xform==NULL)
		return 0;
	if ( (xform->function = module_load_transform_function(function))==NULL )
	{
		free(xform);
		return 0;
	}
	xform->source_type = stype;
	xform->source = source;
	xform->source_dim = 1;
	xform->source_addr = source; /* this assumes the double is the first member of the structure */
	xform->source_schedule = sched;
	xform->target_obj = obj;
	xform->target_prop = prop;
	xform->target = target;
	xform->next = schedule_xformlist;
	xform->function_type = XT_EXTERNAL;
	schedule_xformlist = xform;
	return 1;
}
int transform_add( TRANSFORMSOURCE stype,	/* specifies the type of source */
				   double *source,		/* pointer to the source value */
				   double *target,		/* pointer to the target value */
				   double scale,		/* transform scalar */
				   double bias,			/* transform offset */
				   OBJECT *obj,			/* object containing target value */
				   PROPERTY *prop,		/* property associated with target value */
				   SCHEDULE *sched)		/* schedule object assoicated with target value, if stype == XS_SCHEDULE */
{
	TRANSFORM *xform = (TRANSFORM*)malloc(sizeof(TRANSFORM));
	if (xform==NULL)
		return 0;
	xform->source_type = stype;
	xform->source = source;
	xform->source_dim = 1;
	xform->source_addr = source; /* this assumes the double is the first member of the structure */
	xform->source_schedule = sched;
	xform->target_obj = obj;
	xform->target_prop = prop;
	xform->target = target;
	xform->scale = scale;
	xform->bias = bias;
	xform->function_type = XT_LINEAR;
	xform->next = schedule_xformlist;
	schedule_xformlist = xform;
	return 1;
}

clock_t transform_synctime = 0;
TIMESTAMP transform_syncall(TIMESTAMP t1, TRANSFORMSOURCE restrict)
{
	TRANSFORM *xform;
	clock_t start = clock();
	/* process the schedule transformations */
	for (xform=schedule_xformlist; xform!=NULL; xform=xform->next)
	{	
		if (xform->source_type&restrict){
			if((xform->source_type == XS_SCHEDULE) && (xform->target_obj->schedule_skew != 0)){
				TIMESTAMP t = t1 - xform->target_obj->schedule_skew; // subtract so the +12 is 'twelve seconds later', not earlier
				if((t < xform->source_schedule->since) || (t >= xform->source_schedule->next_t)){
					SCHEDULEINDEX index = schedule_index(xform->source_schedule,t);
					double value = schedule_value(xform->source_schedule,index);
					*(xform->target) = value * xform->scale + xform->bias;
				} else {
					*(xform->target) = *(xform->source) * xform->scale + xform->bias;
				}
			} else {
				*(xform->target) = *(xform->source) * xform->scale + xform->bias;
			}
		}
	}
	transform_synctime += clock() - start;
	return TS_NEVER;
}
