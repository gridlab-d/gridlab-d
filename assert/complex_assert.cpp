/* complex_assert

   Very simple test that compares complex values to any corresponding complex value.  It breaks the
   tests down into a test for the real value and a test for the imagniary portion.  If either test 
   fails at any time, it throws a 'zero' to the commit function and breaks the simulator out with 
   a failure code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "complex_assert.h"

CLASS *complex_assert::oclass = NULL;
complex_assert *complex_assert::defaults = NULL;

complex_assert::complex_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"complex_assert",sizeof(complex_assert),0x00);
		if (oclass==NULL)
			throw "unable to register class complex_assert";
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
			PT_enumeration, "operation", PADDR(operation),
				PT_KEYWORD,"FULL",FULL,
				PT_KEYWORD,"REAL",REAL,
				PT_KEYWORD,"IMAGINARY",IMAGINARY,
				PT_KEYWORD,"MAGNITUDE",MAGNITUDE,
				PT_KEYWORD,"ANGLE",ANGLE, // If using, please specify in radians
			PT_complex, "value", PADDR(value),
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
		operation = FULL;
	}
}

/* Object creation is called once for each object that is created by the core */
int complex_assert::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int complex_assert::init(OBJECT *parent)
{
	if (within <= 0.0){
		char *msg = "A non-positive value has been specified for within.";
		throw msg;
		/*  TROUBLESHOOT
		Within is the range in which the check is being performed.  Please check to see that you have
		specified a value for "within" and it is positive.
		*/
	}
	return 1;
}

complex *complex_assert::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

EXPORT int create_complex_assert(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(complex_assert::oclass);
		if (*obj!=NULL)
		{
			complex_assert *my = OBJECTDATA(*obj,complex_assert);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(complex_assert);
}



EXPORT int init_complex_assert(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,complex_assert)->init(parent); 
		else
			return 0;
	}
	INIT_CATCHALL(complex_assert);
}

EXPORT TIMESTAMP commit_complex_assert(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	char buff[64];
	complex_assert *ca = OBJECTDATA(obj,complex_assert);

	if(ca->once == ca->ONCE_TRUE){
		ca->once_value = ca->value;
		ca->once = ca->ONCE_DONE;
	} else if (ca->once == ca->ONCE_DONE){
		if(ca->once_value == ca->value){
			gl_verbose("Assert skipped with ONCE logic");
			return TS_NEVER;
		} else {
			ca->once_value = ca->value;
		}
	}
	complex *x = (complex*)gl_get_complex_by_name(obj->parent,ca->target);
	if (x==NULL) {
		gl_error("Specified target %s for %s is not valid.",ca->target,gl_name(obj->parent,buff,64));
		/*  TROUBLESHOOT
		Check to make sure the target you are specifying is a published variable for the object
		that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
		check the wiki page to determine which variables can be published within the object you
		are pointing to with the assert function.
		*/
	}
	else if (ca->status == ca->ASSERT_TRUE)
	{
		if (ca->operation == ca->FULL || ca->operation == ca->REAL || ca->operation == ca->IMAGINARY)
		{	
			complex error = *x - ca->value;
			double real_error = error.Re();
			double imag_error = error.Im();
	
			if ((_isnan(real_error) || fabs(real_error)>ca->within) && (ca->operation == ca->FULL || ca->operation == ca->REAL))
			{
				gl_verbose("Assert failed on %s: real part of %s %g not within %f of given value %g", 
				gl_name(obj->parent,buff,64), ca->target, x->Re(), ca->within, ca->value.Re());

				return 0;
			}
			if ((_isnan(imag_error) || fabs(imag_error)>ca->within) && (ca->operation == ca->FULL || ca->operation == ca->IMAGINARY))
			{
				gl_verbose("Assert failed on %s: imaginary part of %s %+gi not within %f of given value %+gi", 
				gl_name(obj->parent,buff,64), ca->target, x->Im(), ca->within, ca->value.Im());

				return 0;
			}
		}
		else if (ca->operation == ca->MAGNITUDE)
		{
			complex val = *x;
			double magnitude_error = val.Mag() - ca->value.Mag();

			if ( _isnan(magnitude_error) || fabs(magnitude_error) > ca->within )
			{
				gl_verbose("Assert failed on %s: Magnitude of %s (%g) not within %f of given value %g", 
				gl_name(obj->parent,buff,64), ca->target, val.Mag(), ca->within, ca->value.Mag());

				return 0;
			}
		}
		else if (ca->operation == ca->ANGLE)
		{
			complex val = *x;
			double angle_error = val.Arg() - ca->value.Arg();

			if ( _isnan(angle_error) || fabs(angle_error) > ca->within )
			{
				gl_verbose("Assert failed on %s: Angle of %s (%g) not within %f of given value %g", 
				gl_name(obj->parent,buff,64), ca->target, val.Arg(), ca->within, ca->value.Arg());

				return 0;
			}
		}
	}
	else if (ca->status == ca->ASSERT_FALSE)
	{
		complex error = *x - ca->value;
		double real_error = error.Re();
		double imag_error = error.Im();
		if ((_isnan(real_error) || fabs(real_error)<ca->within)||(_isnan(imag_error) || fabs(imag_error)<ca->within)){
			if (_isnan(real_error) || fabs(real_error)<ca->within) {
				gl_verbose("Assert failed on %s: real part of %s %g is within %f of %g", 
				gl_name(obj->parent,buff,64), ca->target, x->Re(), ca->within, ca->value.Re());
			}
			if (_isnan(imag_error) || fabs(imag_error)<ca->within) {
				gl_verbose("Assert failed on %s: imaginary part of %s %+gi is within %f of %gi", 
				gl_name(obj->parent,buff,64), ca->target, x->Im(), ca->within, ca->value.Im());
			}
			return 0;
		}
	}
	else
	{
		gl_verbose("Assert test is not being run on %s", gl_name(obj->parent, buff, 64));
		return TS_NEVER;
	}
	gl_verbose("Assert passed on %s",gl_name(obj->parent,buff,64));
	return TS_NEVER;
}

EXPORT int notify_complex_assert(OBJECT *obj, int update_mode, PROPERTY *prop){
	complex_assert *ca = OBJECTDATA(obj,complex_assert);
	if(update_mode == NM_POSTUPDATE && (ca->once == ca->ONCE_DONE) && (strcmp(prop->name, "value") == 0)){
		ca->once = ca->ONCE_TRUE;
	}
	return 1;
}

EXPORT TIMESTAMP sync_complex_assert(OBJECT *obj, TIMESTAMP t0)
{
	return TS_NEVER;
}
