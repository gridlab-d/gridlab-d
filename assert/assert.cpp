/** $Id$

   General purpose assert objects

 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "assert.h"

CLASS *g_assert::oclass = NULL;
g_assert *g_assert::defaults = NULL;

g_assert::g_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"assert",sizeof(g_assert),0x00);
		if (oclass==NULL)
			throw "unable to register class assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			PT_enumeration,"status",PADDR(status),PT_DESCRIPTION,"desired outcome of assert test",
				PT_KEYWORD,"TRUE",AS_TRUE,
				PT_KEYWORD,"FALSE",AS_FALSE,
				PT_KEYWORD,"NONE",AS_NONE,
			PT_char1024, "target", PADDR(target),PT_DESCRIPTION,"the target property to test",
			PT_char32, "part", PADDR(part),PT_DESCRIPTION,"the target property part to test",
			PT_enumeration,"relation",PADDR(relation),PT_DESCRIPTION,"the relation to use for the test",
				PT_KEYWORD,"==",TCOP_EQ,
				PT_KEYWORD,"<",TCOP_LT,
				PT_KEYWORD,"<=",TCOP_LE,
				PT_KEYWORD,">",TCOP_GT,
				PT_KEYWORD,">=",TCOP_GE,
				PT_KEYWORD,"!=",TCOP_NE,
				PT_KEYWORD,"inside",TCOP_IN,
				PT_KEYWORD,"outside",TCOP_NI,
			PT_char1024, "value", PADDR(value),PT_DESCRIPTION,"the value to compare with for binary tests",
			PT_char1024, "lower", PADDR(value),PT_DESCRIPTION,"the lower bound to compare with for interval tests",
			PT_char1024, "upper", PADDR(value2),PT_DESCRIPTION,"the upper bound to compare with for interval tests",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		defaults = this;
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
	if ( get_parent()==NULL )
		exception("no parent object was specified");

	gld_property target(get_parent(),get_target());
	if ( !target.is_valid() )
		exception("target %s does not exist in parent %s",get_target(), get_parent()->get_name());

	set_status(AS_TRUE);
	return 1;
}

TIMESTAMP g_assert::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	// get the target property
	gld_property target(get_parent()->my(),get_target());
	if ( !target.is_valid() ) 
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
	if ( get_status()==AS_NONE ) 
	{
		gl_verbose("%s: test is not being run on %s", get_name(), get_parent()->get_name());
		return TS_NEVER;
	}
	else
	{
		if ( evaluate_status() != get_status() )
		{
			gld_property relation(my(),"relation");
			gld_keyword *pKeyword = relation.find_keyword(get_relation());
			gld_property target(get_parent(),get_target());
			char buf[1024];
			char *p = get_part();
			gl_error("%s: assert failed on %s %s.%s.%s %s %s %s %s", get_name(), get_status()==AS_TRUE?"":"NOT",
				get_parent()->get_name(), get_target(), get_part(),target.to_string(buf,sizeof(buf))?buf:"(void)", pKeyword->get_name(), get_value(), get_value2());
			return 0;
		}
		else
		{
			gl_verbose("%s: assert passed on %s", get_name(), get_parent()->get_name());
			return TS_NEVER;
		}
	}
	// should never get here
}

g_assert::ASSERTSTATUS g_assert::evaluate_status(void)
{
	gld_property target(get_parent(),get_target());
	if ( strcmp(get_part(),"")==0 )
		return target.compare(get_relation(),get_value(),get_value2()) ? AS_TRUE : AS_FALSE ;
	else
		return target.compare(get_relation(),get_value(),get_value2(), get_part()) ? AS_TRUE : AS_FALSE ;
}

int g_assert::postnotify(PROPERTY *prop, char *value)
{
	// TODO notify handler for changed value
	return 1;
}


//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_assert(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(g_assert::oclass);
		if ( *obj != NULL )
		{
			g_assert *my = OBJECTDATA(*obj,g_assert);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(assert);
}



EXPORT int init_assert(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,g_assert)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(assert);
}

EXPORT TIMESTAMP sync_assert(OBJECT *obj, TIMESTAMP t0)
{
	return TS_NEVER;
}

EXPORT TIMESTAMP commit_assert(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	g_assert *da = OBJECTDATA(obj,g_assert);
	try {
		if ( obj!=NULL )
			return da->commit(t1,t2);
		else
			return TS_NEVER;
	}
	T_CATCHALL(g_assert,commit);
}

EXPORT int notify_assert(OBJECT *obj, int update_mode, PROPERTY *prop, char *value)
{
	g_assert *da = OBJECTDATA(obj,g_assert);
	try {
		if ( obj!=NULL )
		{
			switch (update_mode) {
			case NM_POSTUPDATE:
				return da->postnotify(prop,value);
			case NM_PREUPDATE:
			default:
				return 1;
			}
		}
		else
			return 0;
	}
	T_CATCHALL(assert,commit);
	return 1;
}
