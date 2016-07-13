/* int_assert
 
 Very simple test that compares double values to any corresponding double value.  If the test
 fails at any time, it throws a 'zero' to the commit function and breaks the simulator out with
 a failure code.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "int_assert.h"

EXPORT_CREATE(int_assert);
EXPORT_INIT(int_assert);
EXPORT_COMMIT(int_assert);
EXPORT_NOTIFY(int_assert);

CLASS *int_assert::oclass = NULL;
int_assert *int_assert::defaults = NULL;

int_assert::int_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"int_assert",sizeof(int_assert),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class int_assert";
		else
			oclass->trl = TRL_PROVEN;
        
		if (gl_publish_variable(oclass,
                                // TO DO:  publish your variables here
                                PT_enumeration,"status",get_status_offset(),
                                PT_KEYWORD,"ASSERT_TRUE",(enumeration)ASSERT_TRUE,
                                PT_KEYWORD,"ASSERT_FALSE",(enumeration)ASSERT_FALSE,
                                PT_KEYWORD,"ASSERT_NONE",(enumeration)ASSERT_NONE,
                                PT_enumeration, "once", get_once_offset(),
                                PT_KEYWORD,"ONCE_FALSE",(enumeration)ONCE_FALSE,
                                PT_KEYWORD,"ONCE_TRUE",(enumeration)ONCE_TRUE,
                                PT_KEYWORD,"ONCE_DONE",(enumeration)ONCE_DONE,
                                PT_enumeration, "within_mode", get_within_mode_offset(),
                                PT_KEYWORD,"WITHIN_VALUE",(enumeration)IN_ABS,
                                PT_KEYWORD,"WITHIN_RATIO",(enumeration)IN_RATIO,
                                PT_int32, "value", get_value_offset(),
                                PT_int32, "within", get_within_offset(),
                                PT_char1024, "target", get_target_offset(),
                                NULL)<1){
            char msg[256];
            sprintf(msg, "unable to publish properties in %s",__FILE__);
            throw msg;
		}
        
		defaults = this;
		status = ASSERT_TRUE;
		within = 0;
		within_mode = IN_ABS;
		value = 0;
		once = ONCE_FALSE;
		once_value = 0;
	}
}

/* Object creation is called once for each object that is created by the core */
int int_assert::create(void)
{
	memcpy(this,defaults,sizeof(*this));
    
	return 1; /* return 1 on success, 0 on failure */
}

int int_assert::init(OBJECT *parent)
{
	char *msg = "A negative value has been specified for within.";
	if (within < 0)
		throw msg;
    /*  TROUBLESHOOT
     Within is the range in which the check is being performed.  Please check to see that you have
     specified a value for "within" and it is not negative.
     */
	return 1;
}

TIMESTAMP int_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
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
	gld_property target_prop(get_parent(),get_target());
	if ( !target_prop.is_valid() || !(target_prop.get_type()==PT_int16 || target_prop.get_type()==PT_int32 || target_prop.get_type()==PT_int64))
	{
		gl_error("Specified target %s for %s is not valid.",get_target(),get_parent()->get_name());
		/*  TROUBLESHOOT
         Check to make sure the target you are specifying is a published variable for the object
         that you are pointing to.  Refer to the documentation of the command flag --modhelp, or
         check the wiki page to determine which variables can be published within the object you
         are pointing to with the assert function.
         */
		return 0;
	}
    
	// get the within range
	int range = 0;
	if ( within_mode == IN_RATIO )
	{
		range = value * within;
		if ( range < 1 )
		{	// minimum bounds
			range = 1;
		}
	}
	else if ( within_mode== IN_ABS )
	{
		range = within;
	}
    
	// test the target value
	int64 x; target_prop.getp(x);
	if ( status == ASSERT_TRUE )
	{
        int64 theDiff = x-value;
        if(theDiff > 0xFFFFFFFF || theDiff < -0xFFFFFFFF){
            gl_warning("int_assert may be incorrect, difference range outside 32-bit abs() range");
        }
		int64 m = fabs((double) theDiff);
		if ( m>range )
		{
			gl_error("Assert failed on %s: %s %i not within %i of given value %i",
                     get_parent()->get_name(), get_target(), x, range, value);
			return 0;
		}
		gl_verbose("Assert passed on %s", get_parent()->get_name());
		return TS_NEVER;
	}
	else if ( status == ASSERT_FALSE )
	{
        int64 theDiff = x-value;
        if(theDiff > 0xFFFFFFFF || theDiff < -0xFFFFFFFF){
            gl_warning("int_assert may be incorrect, difference range outside 32-bit abs() range");
        }
        int64 m = fabs((double) theDiff);
        if ( m<range )
		{				
			gl_error("Assert failed on %s: %s %i is within %i of given value %i",
                     get_parent()->get_name(), get_target(), x, range, value);
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

int int_assert::postnotify(PROPERTY *prop, char *value)
{
	if ( once==ONCE_DONE && strcmp(prop->name, "value")==0 )
	{
		once = ONCE_TRUE;
	}
	return 1;
}
