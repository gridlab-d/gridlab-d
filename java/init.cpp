/* $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
 *	Copyright (C) 2008 Battelle Memorial Institute
 *	glmjava module
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "holder.h"
#include "gridlabd.h"
#include "gridlabd_java.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	PROPERTYTYPE p;

	if(sizeof(void *) == sizeof(int32))
		p = PT_int32;
	if(sizeof(void *) == sizeof(int64))
		p = PT_int64;

	// set the GridLAB core API callback table
	callback = fntable;


	gl_global_create("glmjava::classpath",PT_char256,get_classpath(),NULL);
	gl_global_create("glmjava::libpath",PT_char256,get_libpath(),NULL);
	gl_global_create("glmjava::jcallback",p,get_jcb(),PT_ACCESS,PA_REFERENCE,NULL);
	//	default values
	strcpy(get_classpath(), "Win32/Debug/");
	strcpy(get_libpath(), "Win32/Debug/");


//	if(get_jvm() == NULL){
//		gl_error("Unable to initialize JVM in java->init()");
//		return NULL;
//	}

	// very nonstandard.
	return new_holder(module);
}

EXPORT int check(){
	return 0;
}

//static CALLBACKS *callback = NULL;


/* EOF */
