/** $Id: test.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _CLIMATETEST_H
#define _CLIMATETEST_H

#include "gridlabd.h"
#include "test_framework.h"
extern "C"
{
#include "timestamp.h"
}
#include "test_callbacks.h"
#include "climate.h"

/**
 * Unit test class for the Climate Module.  This test class is also being
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
class climate_module_test : public test_helper
{
	OBJECT *obj;
public:
	climate_module_test(){}
	
	/**
	 * Called by CPPUnit to ensure that any special pre-testing steps are
	 * completed before the tests are executed.
	 */
	void setup()
	{
		//MODULE* mod = find_module("climate");
	}

	/**
	 * Test the creation of a climate object.  Part of this tests includes
	 * retrieving the class that this object will represent, creating the
	 * object, and testing certain post creation assumptions.
	 *
	 * For each feature of the module that we want to test, we should create
	 * a separate test function.  A function prototype is acceptable here,
	 * with the implementation of the method in a .cpp file.
	 */
	void test_create()
	{
		CLASS *cl = get_class_by_name("climate");
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

	void test_tmy2(){

		CLASS *cl = get_class_by_name("climate");
		CPPUNIT_ASSERT(cl != NULL);
		obj = gl_create_object(cl);
		CPPUNIT_ASSERT(obj != NULL);
							 //S,       SE,      SW,E,       W, NE,      NW,N  
		double test_data[9] = {33.70209,36.79533,29,35.32216,29,30.14556,29,29,30.38584};

		strcpy(OBJECTDATA(obj,climate)->tmyfile,"ca.los_angeles.23174");
		//strcpy(OBJECTDATA(obj,climate)->tmyfile,"co.boulder_denver.94018");

		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		DATETIME dt;
		dt.day = 1;
		dt.month = 1;
		dt.year = 2007;
		dt.hour = 7;
		dt.minute = 0;
		dt.second = 0;
		strcpy(dt.tz,"PST");

		gl_convert("W/m^2", "W/sf", &(test_data[0]));
		gl_convert("W/m^2", "W/sf", &(test_data[1]));
		gl_convert("W/m^2", "W/sf", &(test_data[2]));
		gl_convert("W/m^2", "W/sf", &(test_data[3]));
		gl_convert("W/m^2", "W/sf", &(test_data[4]));
		gl_convert("W/m^2", "W/sf", &(test_data[5]));
		gl_convert("W/m^2", "W/sf", &(test_data[6]));
		gl_convert("W/m^2", "W/sf", &(test_data[7]));
		gl_convert("W/m^2", "W/sf", &(test_data[8]));

		TIMESTAMP tstamp = gl_mktime(&dt);
		TIMESTAMP newtime;

		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);
		for(COMPASS_PTS c_point = CP_S; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
			double data = OBJECTDATA(obj,climate)->solar_flux[c_point];
			double test_diff = data < test_data[c_point] ? test_data[c_point] - data : data - test_data[c_point];
			CPPUNIT_ASSERT(test_diff < .1);
		}
		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);
		tstamp = tstamp - 3600;
		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);
		tstamp = tstamp - 3600;
		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);
		tstamp = tstamp - 3600;
		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);
		tstamp = tstamp - 3600;
		newtime = local_callbacks->myobject_sync(obj,tstamp,PC_BOTTOMUP);

		
	}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(climate_module_test);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	//CPPUNIT_TEST(test_create);
	CPPUNIT_TEST(test_tmy2);
	CPPUNIT_TEST_SUITE_END();
};



/*
 * Registers the test suite with the CPPUnit registration factory.  In most
 * cases, this is all that is required for it to be included and run by the
 * module_test method in test.cpp.
 */
CPPUNIT_TEST_SUITE_REGISTRATION(climate_module_test);

#endif

