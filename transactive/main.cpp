/// transactive/main.cpp
// Copyright (C) 2016, Stanford University
// Author: dchassin@slac.stanford.edu

#define DLMAIN

#include "transactive.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	try {
		CLASS *first = (new house(module))->oclass;
		new agent(module);
		return first; /* always return the first class registered */
	}
	catch (const char *msg)
	{
		gl_error("module::init(module='%s'): initialization failed: %s", module->name, msg);
		return NULL;
	}
	catch (...)
	{
		gl_error("module::init(module='%s'): initialization failed (unhandled exception)", module->name);
		return NULL;
	}

}


EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

