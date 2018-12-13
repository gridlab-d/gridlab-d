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
#include "exec.h"

SET_MYCONTEXT(DMC_TRANSFORM)

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

TRANSFERFUNCTION *tflist = NULL; ///< transfer function list
int write_term(char *buffer,double a,char *x,int n,bool first)
{
	int len = 0;
	if ( fabs(a)>1e-6 ) // non-zero
	{
		if ( n==0 ) // z^0 term
		{
			if ( fabs(a-1)<1e-6 )
				len += sprintf(buffer+len,"%s",first?"1":"+1");
			else if ( fabs(-a-1)<1e-6 )
				len += sprintf(buffer+len,"%s","-1");
			else
				len += sprintf(buffer+len,first?"%.4f":"%+.4f",a);
		}
		else // z^n term
		{
			if ( fabs(a-1)>1e-6 ) // non-unitary coefficient
				len += sprintf(buffer+len,"%.4f",a);
			else if ( fabs(-a-1)<1e-6 ) // -1 coefficient
				len += sprintf(buffer+len,"-");
			len += sprintf(buffer+len,"%s",x); // domain variable
			if ( n>1 ) // higher-order
				len += sprintf(buffer+len,"^%d",n);
		}
	}
	return len;
}
int transfer_function_add(char *name,		///< transfer function name
						  char *domain,		///< domain variable name
						  double timestep,	///< timestep (seconds)
						  double timeskew,	///< timeskew (seconds)
						  unsigned int n,		///< denominator order
						  double *a,			///< denominator coefficients
						  unsigned int m,		///< numerator order
						  double *b)			///< numerator coefficients
{
	TRANSFERFUNCTION *tf = (TRANSFERFUNCTION*)malloc(sizeof(TRANSFERFUNCTION));
	if ( tf == NULL )
	{
		output_error("transfer_function_add(): memory allocation failure");
		return 0;
	}
	memset(tf,0,sizeof(TRANSFERFUNCTION));
	strncpy(tf->name,name,sizeof(tf->name)-1);
	strncpy(tf->domain,domain,sizeof(tf->domain)-1);
	tf->timestep = timestep;
	tf->timeskew = timeskew;
	tf->n = n;
	tf->a = (double*)malloc(sizeof(double)*n);
	if ( tf->a == NULL )
	{
		output_error("transfer_function_add(): memory allocation failure");
		return 0;
	}
	memcpy(tf->a,a,sizeof(double)*n);
	tf->m = m;
	tf->b = (double*)malloc(sizeof(double)*m);
	if ( tf->b == NULL )
	{
		output_error("transfer_function_add(): memory allocation failure");
		return 0;
	}
	memcpy(tf->b,b,sizeof(double)*m);
	tf->next = tflist;
	tflist = tf;

	// debugging output
	if ( global_debug_output )
	{
		char num[1024]="", den[1024]="";
		int i;
		int len;
		int count1=0, count2=0;
		for ( len=0,i=n-1 ; i>=0 ; i-- )
		{
			int c = write_term(den+len,a[i],domain,i,len==0);
			if ( c>0 ) count1++;
			len +=c;
		}
		for ( len=0,i=m-1 ; i>=0 ; i-- )
		{
			int c = write_term(num+len,b[i],domain,i,len==0);
			if ( c>0 ) count2++;
			len +=c;
		}
		IN_MYCONTEXT output_debug("defining transfer function %s(%s) = %s%s%s/%s%s%s, step=%.0fs, skew=%.0fs", name,domain,
			count1>1?"(":"",num,count1>1?")":"",
			count2>1?"(":"",den,count2>1?")":"",
			timestep,timeskew);
	}
	return 1;
}
TRANSFERFUNCTION *find_filter(char *name)
{
	TRANSFERFUNCTION *tf;
	for ( tf = tflist ; tf != NULL ; tf = tf->next )
	{
		if ( strcmp(tf->name,name)==0 )
			return tf;
	}
	return NULL;
}
int get_source_type(PROPERTY *prop)
{
	/* TODO extend this to support multiple sources */
	switch ( prop->ptype ) {
	case PT_double: return XS_DOUBLE; 
	case PT_complex: return XS_COMPLEX; 
	case PT_loadshape: return XS_LOADSHAPE; 
	case PT_enduse: return XS_ENDUSE; 
	case PT_random: return XS_RANDOMVAR; 
	default:
		output_error("tranform/get_source_type(PROPERTY *prop='%s'): unsupported source property type '%s'",
			prop->name,property_getspec(prop->ptype)->name);
		return 0;
	}
}
int transform_add_filter(OBJECT *target_obj,		/* pointer to the target object (lhs) */
						 PROPERTY *target_prop,	/* pointer to the target property */
						 char *filter,			/* filter name to use */
						 OBJECT *source_obj,		/* object containing source value (rhs) */
						 PROPERTY *source_prop)		/* schedule object associated with target value, used if stype == XS_SCHEDULE */
{
	char buffer1[1024], buffer2[1024];
	TRANSFORM *xform;
	TRANSFERFUNCTION *tf;

	// find the filter
	tf = find_filter(filter);
	if ( tf == NULL )
	{
		output_error("transform_add_filter(source='%s:%s',filter='%s',target='%s:%s'): transfer function not defined",
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,filter, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
		return 0;
	}

	// allocate memory for the transform
	xform = (TRANSFORM*)malloc(sizeof(TRANSFORM));
	if ( xform == NULL )
	{
		output_error("transform_add_filter(source='%s:%s',filter='%s',target='%s:%s'): memory allocation failure",
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,filter, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
		return 0;
	}
	xform->x = (double*)malloc(sizeof(double)*(tf->n-1));
	if ( xform->x == NULL )
	{
		output_error("transform_add_filter(source='%s:%s',filter='%s',target='%s:%s'): memory allocation failure",
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,filter, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
		free(xform);
		return 0;
	}
	memset(xform->x,0,sizeof(double)*(tf->n-1));

	// build tranform
	xform->source = object_get_addr(source_obj,source_prop->name);
	xform->source_type = get_source_type(source_prop);
	xform->target_obj = target_obj;
	xform->target_prop = target_prop;
	xform->function_type = XT_FILTER;
	xform->tf = tf;
	xform->y = object_get_addr(target_obj,target_prop->name);
	xform->t2 = (int64)(global_starttime/tf->timestep)*tf->timestep + tf->timeskew;
	xform->next = schedule_xformlist;
	schedule_xformlist = xform;

	if ( global_debug_output )
	{
		IN_MYCONTEXT output_debug("added filter '%s' from source '%s:%s' to target '%s:%s'", filter,
			object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
	}
	return 1;

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
	xform->source_type = get_source_type(source_prop);

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
	IN_MYCONTEXT output_debug("added external transform %s:%s <- %s(%s:%s)", object_name(target_obj,buffer1,sizeof(buffer1)),target_prop->name,function, object_name(source_obj,buffer2,sizeof(buffer2)),source_prop->name);
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
	IN_MYCONTEXT output_debug("added linear transform %s:%s <- scale=%.3g, bias=%.3g", object_name(obj,buffer,sizeof(buffer)), prop->name, scale, bias);
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

TIMESTAMP apply_filter(TRANSFERFUNCTION *f,	///< transfer function
					   double *u,			///< input vector
					   double *x,			///< state vector
					   double *y,			///< output vector
					   TIMESTAMP t1)		///< current time value
{
	unsigned int n = f->n-1;
	unsigned int m = f->m;
	double *a = f->a;
	double *b = f->b;
	static double *dx = NULL;
	static unsigned int len = 0;
	unsigned int i;

	// memory check
	if ( n > len )
	{
		len = (n/4+1)*4;
		dx = (double*)realloc(dx,len);
	}

	// observable form
	for ( i=0 ; i<n ; i++ )
	{
		if ( i==0 )
			dx[i] = -a[i]*x[n-1];
		else
			dx[i] = x[i-1] - a[i]*x[n-1];
		if ( i<m )
			dx[i] += b[i] * (*u);
	}
	memcpy(x,dx,sizeof(double)*n);
	*y = x[n-1]; // output
	return ((int64)(t1/f->timestep)+1)*f->timestep + f->timeskew;
}

/** apply the transform, source is optional and xform.source is used when source is NULL 
    @return timestamp for next update, TS_NEVER for none, TS_ZERO for error
**/
TIMESTAMP transform_apply(TIMESTAMP t1, TRANSFORM *xform, double *source)
{
	char buffer[1024];
	TIMESTAMP t2;
	switch (xform->function_type) {
	case XT_LINEAR:
#ifdef _DEBUG
		IN_MYCONTEXT output_debug("running linear transform for %s:%s", object_name(xform->target_obj,buffer,sizeof(buffer)), xform->target_prop->name);
#endif
		cast_from_double(xform->target_prop->ptype, xform->target, (source?(*source):(*(xform->source))) * xform->scale + xform->bias);
		t2 = TS_NEVER;
		break;
	case XT_EXTERNAL:
#ifdef _DEBUG
		IN_MYCONTEXT output_debug("running external transform for %s:%s", object_name(xform->target_obj,buffer,sizeof(buffer)), xform->target_prop->name);
#endif
		xform->retval = (*xform->function)(xform->nlhs, xform->plhs, xform->nrhs, xform->prhs);
		if ( xform->retval==-1 ) /* error */
			t2 = TS_ZERO;
		else if ( xform->retval==0 ) /* no timer */
			t2 = TS_NEVER;
		else
			t2 = t1 + xform->retval; /* timer given */
		break;
	case XT_FILTER:
#ifdef _DEBUG
		IN_MYCONTEXT output_debug("running filter transform for %s:%s", object_name(xform->target_obj,buffer,sizeof(buffer)), xform->target_prop->name);
#endif
		if ( xform->t2 <= t1 )
			xform->t2 = apply_filter(xform->tf,xform->source,xform->x,xform->y,t1);
		t2 = xform->t2;
		break;
	default:
		output_error("transform_apply(): invalid function type %d", xform->function_type);
		t2 = TS_ZERO;
		break;
	}
	return t2;
}

clock_t transform_synctime = 0;
TIMESTAMP transform_syncall(TIMESTAMP t1, TRANSFORMSOURCE source)
{
	TRANSFORM *xform;
	clock_t start = (clock_t)exec_clock();
	TIMESTAMP t2 = TS_NEVER;
	TIMESTAMP tskew, tSkewSince, tSkewNext, t;

	/* process the schedule transformations */
	for (xform=schedule_xformlist; xform!=NULL; xform=xform->next)
	{	
		if (xform->source_type&source){
			if((xform->source_type == XS_SCHEDULE) && (xform->target_obj->schedule_skew != 0)){
			    tskew = t1 - xform->target_obj->schedule_skew; // subtract so the +12 is 'twelve seconds later', not earlier
			    SCHEDULEINDEX index = schedule_index(xform->source_schedule,tskew);
			    int32 dtnext = schedule_dtnext(xform->source_schedule,index)*60;
			    double value = schedule_value(xform->source_schedule,index);
			    t = (dtnext == 0 ? TS_NEVER : t1 + dtnext - (tskew % 60));
			    if ( t < t2 ) t2 = t;
				if((tskew <= xform->source_schedule->since) || (tskew >= xform->source_schedule->next_t)){
					t = transform_apply(t1,xform,&value);
					if ( t<t2 ) t2=t;
				} 
				else 
				{
					t = transform_apply(t1,xform,NULL);
					if ( t<t2 ) t2=t;
				}
			} else {
				t = transform_apply(t1,xform,NULL);
				if ( t<t2 ) t2=t;
			}
		}
	}
	transform_synctime += (clock_t)exec_clock() - start;
	return t2;
}

int transform_saveall(FILE *fp)
{
	int count = 0;
	TRANSFORM *xform;
	for (xform=schedule_xformlist; xform!=NULL; xform=xform->next)
	{
		// TODO write conversion from transform/filter to string definition
		OBJECT *obj = xform->target_obj;
		PROPERTY *prop = xform->target_prop;
		char name[1024];
		object_name(obj,name,sizeof(name));
		count += fprintf(fp,"#warning transform to %s.%s was not saved\n", name, prop->name);
	}
	return count;
}
