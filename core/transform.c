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

/****************************************************************
 * GridLAB-D Variable Handling for transform functions
 ****************************************************************/
GLDVAR *gldvar_create(unsigned int dim)
{
	GLDVAR *vars = NULL;
	vars = (GLDVAR*)malloc(sizeof(GLDVAR)*dim);
	memset(vars,0,sizeof(GLDVAR)*dim);
	return vars;
}
int gldvar_isset(GLDVAR *var, unsigned int n)
{
	return var[n].addr!=NULL;
}
void gldvar_set(GLDVAR *var, unsigned int n, void *addr, PROPERTY *prop)
{
	var[n].addr = addr;
	var[n].prop = prop;
}
void gldvar_unset(GLDVAR *var, unsigned int n)
{
	var[n].addr = var[n].prop = NULL;
}
void *gldvar_getaddr(GLDVAR *var, unsigned int n)
{
	return var[n].addr;
}
void *gldvar_getprop(GLDVAR *var, unsigned int n)
{
	return var[n].prop;
}
PROPERTYTYPE gldvar_gettype(GLDVAR *var, unsigned int n)
{
	return var[n].prop->ptype;
}
char *gldvar_getname(GLDVAR *var, unsigned int n)
{
	return var[n].prop->name;
}
char *gldvar_getstring(GLDVAR *var, unsigned int n, char *buffer, int size)
{
	if ( gldvar_isset(var,n) )
	{
		PROPERTYSPEC *pspec  = property_getspec(var[n].prop->ptype);
		(*pspec->data_to_string)(buffer,size,var[n].addr,var[n].prop);
		return buffer;
	}
	else
		return strncpy(buffer,"NULL",size);
}
UNIT *gldvar_getunits(GLDVAR *var, unsigned int n)
{
	if ( gldvar_isset(var,n) )
		return var[n].prop->unit;
	else
		return NULL;
}

/****************************************************************
 * Transform handling
 ****************************************************************/
TRANSFORM *transform_getnext(TRANSFORM *xform)
{
	return xform?xform->next:schedule_xformlist;
}

int transform_add_external(	OBJECT *target_obj,		/* pointer to the target object (lhs) */
							PROPERTY *target_prop,	/* pointer to the target property */
							char *function,			/* function name to use */
							OBJECT *source_obj,		/* object containing source value (rhs) */
							PROPERTY *source_prop)		/* schedule object associated with target value, used if stype == XS_SCHEDULE */
{
	char buffer1[1024], buffer2[1024];
	TRANSFORM *xform = (TRANSFORM*)malloc(sizeof(TRANSFORM));
	if (xform==NULL)
		return 0;
	if ( (xform->function = module_get_transform_function(function))==NULL )
	{
		output_error("transform_add_external(source='%s:%s',function='%s',target='%s:%s'): function is not defined (probably a missing or invalid extern directive)", 
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,function, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
		free(xform);
		return 0;
	}

	xform->function_type = XT_EXTERNAL;

	/* apply source type */
	/* TODO extend this to support multiple sources */
	xform->source_type = 0;
	switch (source_prop->ptype) {
	case PT_double: xform->source_type |= XS_DOUBLE; break;
	case PT_complex: xform->source_type |= XS_COMPLEX; break;
	case PT_loadshape: xform->source_type |= XS_LOADSHAPE; break;
	case PT_enduse: xform->source_type |= XS_ENDUSE; break;
	default:
		output_error("transform_add_external(source='%s:%s',function='%s',target='%s:%s'): unsupported source property type '%s'", 
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,function, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name,property_getspec(source_prop->ptype)->name);
		break;
	}

	/* build lhs (target) */
	/* TODO extend to support multiple targets, e.g., complex */
	xform->nlhs = 1;
	xform->plhs = gldvar_create(xform->nlhs);
	gldvar_set(xform->plhs,0,object_get_addr(target_obj,target_prop->name),target_prop);
	xform->target_obj = target_obj;
	xform->target_prop = target_prop;

	/* build rhs (source) */
	/* TODO extend this to support more than one source */
	xform->nrhs = 1;
	xform->prhs = gldvar_create(xform->nrhs);
	gldvar_set(xform->prhs,0,object_get_addr(source_obj,source_prop->name),source_prop);

	xform->next = schedule_xformlist;
	schedule_xformlist = xform;
	output_debug("added external transform %s:%s <- %s(%s:%s)", object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,function, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
	return 1;
}
int transform_add_linear(	TRANSFORMSOURCE stype,	/* specifies the type of source */
							double *source,		/* pointer to the source value */
							void *target,		/* pointer to the target value */
							double scale,		/* transform scalar */
							double bias,			/* transform offset */
							OBJECT *obj,			/* object containing target value */
							PROPERTY *prop,		/* property associated with target value */
							SCHEDULE *sched)		/* schedule object assoicated with target value, if stype == XS_SCHEDULE */
{
	char buffer[1024];
	TRANSFORM *xform = (TRANSFORM*)malloc(sizeof(TRANSFORM));
	if (xform==NULL)
		return 0;
	xform->source_type = stype;
	xform->source = source;
	xform->nrhs = 1;
	xform->nlhs = 1;
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
	output_debug("added linear transform %s:%s <- scale=%.3g, bias=%.3g", object_name(obj,buffer,sizeof(buffer)), prop->name, scale, bias);
	return 1;
}

void cast_from_double(PROPERTYTYPE ptype, void *addr, double value)
{
	switch ( ptype ) {
	case PT_void: break;
	case PT_double: *(double*)addr = value; break;
	case PT_complex: ((complex*)addr)->r = value; ((complex*)addr)->i = 0; break;
	case PT_bool: *(int32*)addr = (value!=0); 
	case PT_int16: *(int16*)addr = (int16)value; break;
	case PT_int32: *(int32*)addr = (int32)value; break;
	case PT_int64: *(int64*)addr = (int64)value; break;
	case PT_enumeration: *(enumeration*)addr = (enumeration)value; break;
	case PT_set: *(set*)addr = (set)value; break;
	case PT_object: break;
	case PT_timestamp: *(int64*)addr = (int64)value; break;
	case PT_float: *(float*)addr = (float)value; break;
	case PT_loadshape: ((loadshape*)addr)->load = value; break;
	case PT_enduse: ((enduse*)addr)->total.r = value; break;
	default: break;
	}
}


/** apply the transform, source is optional and xform.source is used when source is NULL 
    @return timestamp for next update, TS_NEVER for none, TS_ZERO for error
**/
int64 apply_transform(TIMESTAMP t1, TRANSFORM *xform, double *source)
{
	char buffer[1024];
	TIMESTAMP t2;
	switch (xform->function_type) {
	case XT_LINEAR:
#ifdef _DEBUG
		output_debug("running linear transform for %s:%s", object_name(xform->target_obj,buffer,sizeof(buffer)), xform->target_prop->name);
#endif
		cast_from_double(xform->target_prop->ptype, xform->target, (source?(*source):(*(xform->source))) * xform->scale + xform->bias);
		t2 = TS_NEVER;
		break;
	case XT_EXTERNAL:
#ifdef _DEBUG
		output_debug("running external transform for %s:%s", object_name(xform->target_obj,buffer,sizeof(buffer)), xform->target_prop->name);
#endif
		xform->retval = (*xform->function)(xform->nlhs, xform->plhs, xform->nrhs, xform->prhs);
		if ( xform->retval==-1 ) /* error */
			t2 = TS_ZERO;
		else if ( xform->retval==0 ) /* no timer */
			t2 = TS_NEVER;
		else
			t2 = t1 + xform->retval; /* timer given */
		break;
	default:
		output_error("apply_transform(): invalid function type %d", xform->function_type);
		t2 = TS_ZERO;
		break;
	}
	return t2;
}

clock_t transform_synctime = 0;
TIMESTAMP transform_syncall(TIMESTAMP t1, TRANSFORMSOURCE source)
{
	TRANSFORM *xform;
	clock_t start = exec_clock();
	TIMESTAMP t2 = TS_NEVER;

	/* process the schedule transformations */
	for (xform=schedule_xformlist; xform!=NULL; xform=xform->next)
	{	
		if (xform->source_type&source){
			if((xform->source_type == XS_SCHEDULE) && (xform->target_obj->schedule_skew != 0)){
				TIMESTAMP t = t1 - xform->target_obj->schedule_skew; // subtract so the +12 is 'twelve seconds later', not earlier
				if((t < xform->source_schedule->since) || (t >= xform->source_schedule->next_t)){
					SCHEDULEINDEX index = schedule_index(xform->source_schedule,t);
					double value = schedule_value(xform->source_schedule,index);
					TIMESTAMP t = apply_transform(t1,xform,&value);
					if ( t<t2 ) t2=t;
				} 
				else 
				{
					TIMESTAMP t = apply_transform(t1,xform,NULL);
					if ( t<t2 ) t2=t;
				}
			} else {
				TIMESTAMP t = apply_transform(t1,xform,NULL);
				if ( t<t2 ) t2=t;
			}
		}
	}
	transform_synctime += exec_clock() - start;
	return t2;
}
