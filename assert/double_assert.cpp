/* double_assert

   Very simple test that compares double values to any corresponding double value.  If the test 
   fails at any time, it throws a 'zero' to the commit function and breaks the simulator out with 
   a failure code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "double_assert.h"

CLASS *double_assert::oclass = NULL;
double_assert *double_assert::defaults = NULL;

double_assert::double_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"double_assert",sizeof(double_assert),0x00);
		if (oclass==NULL)
			throw "unable to register class double_assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_enumeration,"status",PADDR(status),
				PT_KEYWORD,"ASSERT_TRUE",ASSERT_TRUE,
				PT_KEYWORD,"ASSERT_FALSE",ASSERT_FALSE,
				PT_KEYWORD,"ASSERT_NONE",ASSERT_NONE,
			PT_enumeration, "once", PADDR(once),
				PT_KEYWORD,"ONCE_FALSE",ONCE_FALSE,
				PT_KEYWORD,"ONCE_TRUE",ONCE_TRUE,
				PT_KEYWORD,"ONCE_DONE",ONCE_DONE,
			PT_double, "value", PADDR(value),
			PT_double, "within", PADDR(within),
			PT_char1024, "target", PADDR(target),			
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		defaults = this;
		status = ASSERT_TRUE;
		within = 0.0;
		value = 0.0;
		once = ONCE_FALSE;
		once_value = 0;
	}
}

/* Object creation is called once for each object that is created by the core */
int double_assert::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int double_assert::init(OBJECT *parent)
{
	char *msg = "A non-positive value has been specified for within.";
	if (within <= 0.0)
		throw msg;
		/*  TROUBLESHOOT
		Within is the range in which the check is being performed.  Please check to see that you have
		specified a value for "within" and it is positive.
		*/
	return 1;
}

complex *double_assert::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

EXPORT int create_double_assert(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(double_assert::oclass);
		if (*obj!=NULL)
		{
			double_assert *my = OBJECTDATA(*obj,double_assert);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(double_assert);
}



EXPORT int init_double_assert(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,double_assert)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(double_assert);
}

EXPORT TIMESTAMP sync_double_assert(OBJECT *obj, TIMESTAMP t0)
{
	return TS_NEVER;
}

EXPORT TIMESTAMP commit_double_assert(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	//OBJECT *obj;
	char buff[64];
	double_assert *da = OBJECTDATA(obj,double_assert);

	//bj = OBJECTHDR(this);
	if(da->once == da->ONCE_TRUE){
		da->once_value = da->value;
		da->once = da->ONCE_DONE;
	} else if (da->once == da->ONCE_DONE){
		if(da->once_value == da->value){
			gl_verbose("Assert skipped with ONCE logic");
			return TS_NEVER;
		} else {
			da->once_value = da->value;
		}
	}
		
	double *x = (double*)gl_get_double_by_name(obj->parent,da->target);
	if (x==NULL) 
	{
		gl_error("Specified target %s for %s is not valid.",da->target,gl_name(obj->parent,buff,64));
		/*  TROUBLESHOOT
		Check to make sure the target you are specifying is a published variable for the object
		that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
		check the wiki page to determine which variables can be published within the object you
		are pointing to with the assert function.
		*/
		return 0;
	}
	else if (da->status == da->ASSERT_TRUE)
	{
		double m = fabs(*x-da->value);
		if (_isnan(m) || m>da->within)
		{				
			gl_verbose("Assert failed on %s: %s %g not within %f of given value %g", 
				gl_name(obj->parent, buff, 64), da->target, *x, da->within, da->value);
			return 0;
		}
		gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
		return TS_NEVER;
	}
	else if (da->status == da->ASSERT_FALSE)
	{
		double m = fabs(*x-da->value);
		if (_isnan(m) || m<da->within)
		{				
			gl_verbose("Assert failed on %s: %s %g is within %f of given value %g", 
				gl_name(obj->parent, buff, 64), da->target, *x, da->within, da->value);
			return 0;
		}
		gl_verbose("Assert passed on %s", gl_name(obj->parent, buff, 64));
		return TS_NEVER;
	}
	else
	{
		gl_verbose("Assert test is not being run on %s", gl_name(obj->parent, buff, 64));
		return TS_NEVER;
	}

}

EXPORT int notify_double_assert(OBJECT *obj, int update_mode, PROPERTY *prop){
	double_assert *da = OBJECTDATA(obj,double_assert);
	if(update_mode == NM_POSTUPDATE && (da->once == da->ONCE_DONE) && (strcmp(prop->name, "value") == 0)){
		da->once = da->ONCE_TRUE;
	}
	return 1;
}
