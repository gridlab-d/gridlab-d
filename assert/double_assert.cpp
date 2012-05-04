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

EXPORT_CREATE(double_assert);
EXPORT_INIT(double_assert);
EXPORT_COMMIT(double_assert);
EXPORT_NOTIFY(double_assert);

CLASS *double_assert::oclass = NULL;
double_assert *double_assert::defaults = NULL;

double_assert::double_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"double_assert",sizeof(double_assert),PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class double_assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_enumeration,"status",get_status_offset(),
				PT_KEYWORD,"ASSERT_TRUE",ASSERT_TRUE,
				PT_KEYWORD,"ASSERT_FALSE",ASSERT_FALSE,
				PT_KEYWORD,"ASSERT_NONE",ASSERT_NONE,
			PT_enumeration, "once", get_once_offset(),
				PT_KEYWORD,"ONCE_FALSE",ONCE_FALSE,
				PT_KEYWORD,"ONCE_TRUE",ONCE_TRUE,
				PT_KEYWORD,"ONCE_DONE",ONCE_DONE,
			PT_enumeration, "within_mode", get_within_mode_offset(),
				PT_KEYWORD,"WITHIN_VALUE",IN_ABS,
				PT_KEYWORD,"WITHIN_RATIO",IN_RATIO,
			PT_double, "value", get_value_offset(),
			PT_double, "within", get_within_offset(),
			PT_char1024, "target", get_target_offset(),			
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		defaults = this;
		status = ASSERT_TRUE;
		within = 0.0;
		within_mode = IN_ABS;
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

TIMESTAMP double_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	// handle once mode
	if ( once==ONCE_TRUE )
	{
		once_value = value;
		once = ONCE_DONE;
	} 
	else if ( once == ONCE_DONE )
	{
		if ( once_value==value )
		{
			gl_verbose("Assert skipped with ONCE logic");
			return TS_NEVER;
		} 
		else 
		{
			once_value = value;
		}
	}
		
	// get the target property
	gld_property target_prop(get_parent(),target);
	if ( !target_prop.is_valid() || target_prop.get_type()!=PT_double ) 
	{
		gl_error("Specified target %s for %s is not valid.",target,get_parent()->get_name());
		/*  TROUBLESHOOT
		Check to make sure the target you are specifying is a published variable for the object
		that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
		check the wiki page to determine which variables can be published within the object you
		are pointing to with the assert function.
		*/
		return 0;
	}

	// get the within range
	double range = 0.0;
	if ( within_mode == IN_RATIO ) 
	{
		range = value * within;
		if ( range<0.001 ) 
		{	// minimum bounds
			range = 0.001;
		}
	} 
	else if ( within_mode== IN_ABS ) 
	{
		range = within;
	}

	// test the target value
	double x; target_prop.getp(x);
	if ( status == ASSERT_TRUE )
	{
		double m = fabs(x-value);
		if ( _isnan(m) || m>range )
		{				
			gl_verbose("Assert failed on %s: %s %g not within %f of given value %g", 
				get_parent()->get_name(), target, x, range, value);
			return 0;
		}
		gl_verbose("Assert passed on %s", get_parent()->get_name());
		return TS_NEVER;
	}
	else if ( status == ASSERT_FALSE )
	{
		double m = fabs(x-value);
		if ( _isnan(m) || m<range )
		{				
			gl_verbose("Assert failed on %s: %s %g is within %f of given value %g", 
				get_parent()->get_name(), target, x, range, value);
			return 0;
		}
		gl_verbose("Assert passed on %s", get_parent()->get_name());
		return TS_NEVER;
	}
	else
	{
		gl_verbose("Assert test is not being run on %s", get_parent()->get_name());
		return TS_NEVER;
	}

}

int double_assert::postnotify(PROPERTY *prop, char *value)
{
	if ( once==ONCE_DONE && strcmp(prop->name, "value")==0 )
	{
		once = ONCE_TRUE;
	}
	return 1;
}
