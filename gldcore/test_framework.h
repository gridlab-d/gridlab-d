/** $Id: test_framework.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _TESTFRAMEWORK_H
#define _TESTFRAMEWORK_H

#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TextOutputter.h>
#include "gridlabd.h"
#include "test_callbacks.h"

#ifndef _NO_CPPUNIT

TEST_CALLBACKS *local_callbacks;

class test_helper: public CppUnit::TestFixture
{
public:
	
	static CLASS* get_class_by_name(char *name){
		return local_callbacks->get_class_by_name(name);
	}

	static TIMESTAMP get_global_clock(){
		return local_callbacks->get_global_clock();
	}

	static TIMESTAMP myobject_sync(OBJECT *obj, TIMESTAMP ts,PASSCONFIG pass)
	{
		return local_callbacks->myobject_sync(obj,ts,pass);
	}

	template <class T> static T *create_object(const char *classname)
	{
		CLASS *glclass = get_class_by_name((char *) classname);
		OBJECT *gl_obj = gl_create_object(glclass);
		T *obj = OBJECTDATA(gl_obj, T);
		obj->create();
		return obj;
	}
};
#endif
#endif
