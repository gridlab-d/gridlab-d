// $ID$
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _PLCTEST_H
#define _PLCTEST_H

#include "gridlabd.h"
#include "test_framework.h"

#include "test_callbacks.h"
#include "plc.h"

/**
 * Unit test class for the plc Module.  This test class is also being
 * developed as a sample to be used in creating tests for other modules.
 * There are basically 3 steps to creating a CPPUnit test:
 *     1. Write tests - create the methods of the test class that will
 *        exercise the module
 *     2. Create test Suite - Call the CPPUnit macros that will create
 *        the suite() function which will be called by the test runner
 *     3. Register the test suite with the CPPUnit factory.  This is
 *        done by using a single macro that takes the name of the test
 *        class as a parameter.
 *
 */
class plc_module_test : public test_helper
{
	OBJECT *obj;
public:
	plc_module_test(){}

	/**
	 * Called by CPPUnit to ensure that any special pre-testing steps are
	 * completed before the tests are executed.
	 */
	void setup()
	{
		//MODULE* mod = find_module("plc");
	}

	/**
	 * Test the creation of a plc object.  Part of this tests includes
	 * retrieving the class that this object will represent, creating the
	 * object, and testing certain post creation assumptions.
	 *
	 * For each feature of the module that we want to test, we should create
	 * a separate test function.  A function prototype is acceptable here,
	 * with the implementation of the method in a .cpp file.
	 */
	void test_create()
	{
		CLASS *cl = get_class_by_name("plc");
		CPPUNIT_ASSERT(cl != NULL);
		obj = gl_create_object(cl);
		CPPUNIT_ASSERT(obj != NULL);

		// TODO: Need to attach a TMY Tape object before this call will actually work without blowing up.
		//TIMESTAMP ts = myobject_sync(obj,get_global_clock(),0x01);

		double *pTout;
		double *pRhout;
		double *pSolar;
		// TODO: Fill in when lookup functions are complete
		// Get property values for comparison.
		pTout = (double*)GETADDR(obj,gl_get_property(obj,"temperature"));
		pRhout = (double*)GETADDR(obj,gl_get_property(obj,"humidity"));
		pSolar = (double*)GETADDR(obj,gl_get_property(obj,"solar_flux"));
	}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(plc_module_test);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	CPPUNIT_TEST(test_create);
	CPPUNIT_TEST_SUITE_END();
};

/*
 * Registers the test suite with the CPPUnit registration factory.  In most
 * cases, this is all that is required for it to be included and run by the
 * module_test method in test.cpp.
 */
CPPUNIT_TEST_SUITE_REGISTRATION(plc_module_test);

#endif

