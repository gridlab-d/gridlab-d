/* enum_assert

   Very simple test that compares either integers or can be used to compare enumerated values
   to their corresponding integer values.  If the test fails at any time, it throws a 'zero' to
   the commit function and breaks the simulator out with a failure code.
*/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>
#include <string.h>

#include "enum_assert.h"

EXPORT_CREATE(enum_assert);
EXPORT_INIT(enum_assert);
EXPORT_COMMIT(enum_assert);

CLASS *enum_assert::oclass = NULL;
enum_assert *enum_assert::defaults = NULL;

enum_assert::enum_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"enum_assert",sizeof(enum_assert),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class enum_assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_enumeration,"status",get_status_offset(),
				PT_KEYWORD,"ASSERT_TRUE",(enumeration)ASSERT_TRUE,
				PT_KEYWORD,"ASSERT_FALSE",(enumeration)ASSERT_FALSE,
				PT_KEYWORD,"ASSERT_NONE",(enumeration)ASSERT_NONE,
			PT_int32, "value", get_value_offset(),
			PT_char1024, "target", get_target_offset(),	
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		defaults = this;
		status = ASSERT_TRUE;
		value = 0;
	}
}

/* Object creation is called once for each object that is created by the core */
int enum_assert::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int enum_assert::init(OBJECT *parent)
{
	return 1;
}

TIMESTAMP enum_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{

	gld_property target_prop(get_parent(),get_target());
	if ( !target_prop.is_valid() || target_prop.get_type()!=PT_enumeration ) 
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

	int32 x; target_prop.getp(x);
	if (status == ASSERT_TRUE)
	{
		if (value != x) 
		{
			gl_error("Assert failed on %s: %s=%g did not match %g", 
				get_parent()->get_name(), get_target(), x, value);
			return 0;
		}
		else
		{
			gl_verbose("Assert passed on %s", get_parent()->get_name());
			return TS_NEVER;
		}
	}
	else if (status == ASSERT_FALSE)
	{
		if (value == x)
		{
			gl_error("Assert failed on %s: %s=%g did match %g", 
				get_parent()->get_name(), get_target(), x, value);
			return 0;
		}
		else
		{
			gl_verbose("Assert passed on %s", get_parent()->get_name());
			return TS_NEVER;
		}
	}
	else
	{
		gl_verbose("Assert test is not being run on %s", get_parent()->get_name());
		return TS_NEVER;
	}
	gl_verbose("Assert passed on %s",get_parent()->get_name());
	return TS_NEVER;
}

