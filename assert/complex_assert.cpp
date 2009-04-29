

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
		oclass = gl_register_class(module,"complex_assert",sizeof(complex_assert),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);

		if (gl_publish_variable(oclass,
			// TO DO:  publish your variables here
			PT_double, "value", PADDR(value),
			PT_double, "within", PADDR(within),
			PT_char32, "target", PADDR(target),	
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		defaults = this;		
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
	return 1;
}
TIMESTAMP complex_assert::postsync(TIMESTAMP t0, TIMESTAMP t1)
{
	OBJECT *obj;
	char buff[64];
	obj = OBJECTHDR(this);

	if (t0>0)
	{
		complex *x = (complex*)gl_get_complex_by_name(obj->parent,target);
		complex m = (*x-value).Mag();
		if (!m.IsFinite() || m>within){
			gl_warning("assert failed on %s: %s (%g%+gi) not within %f of %g%+gi", 
			gl_name(obj->parent,buff,64), target, x->Re(), x->Im(), within, value.Re(), value.Im());
			return t1;
		}
		return TS_NEVER;
	} 
	else {
		return t1+1;
	}
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
	}
	catch (char *msg)
	{
		gl_error("create_complex_assert: %s", msg);
	}
	return 1;
}



EXPORT int init_complex_assert(OBJECT *obj, OBJECT *parent) 
{
	try 
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,complex_assert)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_complex_assert(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 0;
}

EXPORT TIMESTAMP sync_complex_assert(OBJECT *obj, TIMESTAMP t0)
{
	complex_assert *my = OBJECTDATA(obj,complex_assert);
	TIMESTAMP t1 = my->postsync(obj->clock, t0);
	obj->clock = t0;
	return t1;
}