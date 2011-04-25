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

CLASS *enum_assert::oclass = NULL;
enum_assert *enum_assert::defaults = NULL;

enum_assert::enum_assert(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gl_register_class(module,"enum_assert",sizeof(enum_assert),0x00);
		if (oclass==NULL)
			throw "unable to register class enum_assert";
		else
			oclass->trl = TRL_PROVEN;

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_enumeration,"status",PADDR(status),
				PT_KEYWORD,"ASSERT_TRUE",ASSERT_TRUE,
				PT_KEYWORD,"ASSERT_FALSE",ASSERT_FALSE,
				PT_KEYWORD,"ASSERT_NONE",ASSERT_NONE,
			PT_int32, "value", PADDR(value),
			PT_char1024, "target", PADDR(target),	
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

complex *enum_assert::get_complex(OBJECT *obj, char *name)
{
	PROPERTY *p = gl_get_property(obj,name);
	if (p==NULL || p->ptype!=PT_complex)
		return NULL;
	return (complex*)GETADDR(obj,p);
}

EXPORT int create_enum_assert(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(enum_assert::oclass);
		if (*obj!=NULL)
		{
			enum_assert *my = OBJECTDATA(*obj,enum_assert);
			gl_set_parent(*obj,parent);
			return my->create();
		}
		else
			return 0;
	}
	CREATE_CATCHALL(enum_assert);
}



EXPORT int init_enum_assert(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,enum_assert)->init(parent);
		else
			return 0;
	}
	INIT_CATCHALL(enum_assert);
}
EXPORT TIMESTAMP commit_enum_assert(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2)
{
	char buff[64];
	enum_assert *ea = OBJECTDATA(obj,enum_assert);

		int32 *x = (int32*)gl_get_int32_by_name(obj->parent,ea->target);
		if (x==NULL) {
			gl_error("Specified target %s for %s is not valid.",ea->target,gl_name(obj->parent,buff,64));
			/*  TROUBLESHOOT
			Check to make sure the target you are specifying is a published variable for the object
			that you are pointing to.  Refer to the documentation of the command flag --modhelp, or 
			check the wiki page to determine which variables can be published within the object you
			are pointing to with the assert function.
			*/
			return 0;
		}
		else if (ea->status == ea->ASSERT_TRUE)
		{
			if (ea->value != *x) 
			{
				gl_verbose("Assert failed on %s: %s did not match %i", 
					gl_name(obj->parent,buff,64), ea->target, ea->value);
				return 0;
			}
			else
			{
				gl_verbose("Assert passed on %s", 
					gl_name(obj->parent,buff,64));
				return TS_NEVER;
			}
		}
		else if (ea->status == ea->ASSERT_FALSE)
		{
			if (ea->value == *x)
			{
				gl_verbose("Assert failed on %s: %s did match %i", 
					gl_name(obj->parent,buff,64), ea->target, ea->value);
				return 0;
			}
			else
			{
				gl_verbose("Assert passed on %s", 
					gl_name(obj->parent,buff,64));
				return TS_NEVER;
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
EXPORT TIMESTAMP sync_enum_assert(OBJECT *obj, TIMESTAMP t0)
{
	return TS_NEVER;
}
