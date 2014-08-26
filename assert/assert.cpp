/** $Id: assert.cpp 4738 2014-07-03 00:55:39Z dchassin $

   General purpose assert objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "assert.h"

EXPORT_CREATE_C(assert,g_assert);
EXPORT_INIT_C(assert,g_assert);
EXPORT_COMMIT_C(assert,g_assert);
EXPORT_NOTIFY_C(assert,g_assert);

CLASS *g_assert::oclass = NULL;
g_assert *g_assert::defaults = NULL;

g_assert::g_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"assert",sizeof(g_assert),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class assert";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration,"status",get_status_offset(),PT_DESCRIPTION,"desired outcome of assert test",
				PT_KEYWORD,"INIT",(enumeration)AS_INIT,
				PT_KEYWORD,"TRUE",(enumeration)AS_TRUE,
				PT_KEYWORD,"FALSE",(enumeration)AS_FALSE,
				PT_KEYWORD,"NONE",(enumeration)AS_NONE,
			PT_char1024, "target", get_target_offset(),PT_DESCRIPTION,"the target property to test",
			PT_char32, "part", get_part_offset(),PT_DESCRIPTION,"the target property part to test",
			PT_enumeration,"relation",get_relation_offset(),PT_DESCRIPTION,"the relation to use for the test",
				PT_KEYWORD,"==",(enumeration)TCOP_EQ,
				PT_KEYWORD,"<",(enumeration)TCOP_LT,
				PT_KEYWORD,"<=",(enumeration)TCOP_LE,
				PT_KEYWORD,">",(enumeration)TCOP_GT,
				PT_KEYWORD,">=",(enumeration)TCOP_GE,
				PT_KEYWORD,"!=",(enumeration)TCOP_NE,
				PT_KEYWORD,"inside",(enumeration)TCOP_IN,
				PT_KEYWORD,"outside",(enumeration)TCOP_NI,
			PT_char1024, "value", get_value_offset(),PT_DESCRIPTION,"the value to compare with for binary tests",
			PT_char1024, "within", get_value2_offset(),PT_DESCRIPTION,"the bounds within which the value must bed compared",
			PT_char1024, "lower", get_value_offset(),PT_DESCRIPTION,"the lower bound to compare with for interval tests",
			PT_char1024, "upper", get_value2_offset(),PT_DESCRIPTION,"the upper bound to compare with for interval tests",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(g_assert));
		status = AS_INIT;
		relation=TCOP_EQ;
	}
}

int g_assert::create(void) 
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int g_assert::init(OBJECT *parent)
{
	gld_property target(get_parent(),get_target());
	if ( !target.is_valid() )
		exception("target '%s' property '%s' does not exist", get_parent()?get_parent()->get_name():"global",get_target());

	set_status(AS_TRUE);
	return 1;
}

TIMESTAMP g_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	// get the target property
	gld_property target_prop(get_parent(),get_target());
	if ( !target_prop.is_valid() ) 
	{
		gl_error("%s: target %s is not valid",get_target(),get_name());
		/*  TROUBLESHOOT
		Check to make sure the target you are specifying is a published variable for the object
		that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
		check the wiki page to determine which variables can be published within the object you
		are pointing to with the assert function.
		*/
		return 0;
	}

	// determine the relation status
	if ( status==AS_NONE ) 
	{
		gl_verbose("%s: test is not being run on %s", get_name(), get_parent()->get_name());
		return TS_NEVER;
	}
	else
	{
		if ( evaluate_status() != get_status() )
		{
			gld_property relation_prop(my(),"relation");
			gld_keyword *pKeyword = relation_prop.find_keyword(relation);
			char buf[1024];
			char *p = get_part();
			gl_error("%s: assert failed on %s %s.%s.%s %s %s %s %s", get_name(), status==AS_TRUE?"":"NOT",
				get_parent()?get_parent()->get_name():"global variable", get_target(), get_part(), target_prop.to_string(buf,sizeof(buf))?buf:"(void)", pKeyword->get_name(), get_value(), get_value2());
			return 0;
		}
		else
		{
			gl_verbose("%s: assert passed on %s", get_name(), get_parent()?get_parent()->get_name():"global variable");
			return TS_NEVER;
		}
	}
	// should never get here
}

g_assert::ASSERTSTATUS g_assert::evaluate_status(void)
{
	gld_property target_prop(get_parent(),get_target());
	if ( strcmp(get_part(),"")==0 )
		return target_prop.compare(relation,get_value(),get_value2()) ? AS_TRUE : AS_FALSE ;
	else
		return target_prop.compare(relation,get_value(),get_value2(),get_part()) ? AS_TRUE : AS_FALSE ;
}

int g_assert::postnotify(PROPERTY *prop, char *value)
{
	// TODO notify handler for changed value
	return 1;
}
