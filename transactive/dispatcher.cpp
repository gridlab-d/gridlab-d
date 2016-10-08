// $Id$

#include "dispatcher.h"

EXPORT_IMPLEMENT(dispatcher)
EXPORT_SYNC(dispatcher)
IMPLEMENT_ISA(dispatcher)

dispatcher::dispatcher(MODULE *module)
{
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(module,"dispatcher",sizeof(dispatcher),PC_BOTTOMUP);

		if (oclass==NULL)
			exception("unable to register class");

		// publish the class properties
	if (gl_publish_variable(oclass,
		// insert properties here
		NULL) < 1)
		exception("unable to publish properties");
	}
}

int dispatcher::create()
{
	return 1;
}

int dispatcher::init(OBJECT *parent)
{
	return 1;
}

TIMESTAMP dispatcher::presync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP dispatcher::sync(TIMESTAMP t1)
{
	return TS_NEVER;
}

TIMESTAMP dispatcher::postsync(TIMESTAMP t1)
{
	return TS_NEVER;
}

