// $id$
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _POWERFLOW_TEST_H
#define _POWERFLOW_TEST_H
#define VALIDATE_THRESHOLD .0002

#include "gridlabd.h"

#ifndef _NO_CPPUNIT

#include "test_framework.h"

#include "test_callbacks.h"
// Include any module specific headers needed for testing.

#include "node.h"
#include "link.h"

class powerflow_test_helper : public test_helper
{
protected:
	static void test_powerflow_link(class link_object *test_link, double tolerance,
			set phases1, set phases2, const complex bs_V2_in[3],
			const complex bs_I2_in[3], const complex bs_I1_out[3],
			const complex fs_V1_in[3], const complex fs_I2_in[3],
			const complex fs_V2_out[3])
	{	
		node *src_node = create_object<node>("node");
		load *load_node = create_object<load>("load");

		src_node->phases = phases1;
		load_node->phases = phases2;
		load_node->voltageA = bs_V2_in[0];
		load_node->voltageB = bs_V2_in[1];
		load_node->voltageC = bs_V2_in[2];
		load_node->currentA = bs_I2_in[0];
		load_node->currentB = bs_I2_in[1];
		load_node->currentC = bs_I2_in[2];

		test_link->from = GETOBJECT(src_node);
		test_link->to = GETOBJECT(load_node);

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		CPPUNIT_ASSERT(local_callbacks->sync_all(PC_PRETOPDOWN) != FAILED);
		CPPUNIT_ASSERT(local_callbacks->sync_all(PC_BOTTOMUP) != FAILED);
		complex I1test[3];
		I1test[0]=src_node->currentA;
		I1test[1]=src_node->currentB;
		I1test[2]=src_node->currentC;


		CPPUNIT_ASSERT((src_node->currentA - bs_I1_out[0]).IsZero(tolerance));
		CPPUNIT_ASSERT((src_node->currentB - bs_I1_out[1]).IsZero(tolerance));
		CPPUNIT_ASSERT((src_node->currentC - bs_I1_out[2]).IsZero(tolerance));

		src_node->voltageA = fs_V1_in[0];
		src_node->voltageB = fs_V1_in[1];
		src_node->voltageC = fs_V1_in[2];
		load_node->currentA = fs_I2_in[0];
		load_node->currentB = fs_I2_in[1];
		load_node->currentC = fs_I2_in[2];

		CPPUNIT_ASSERT(local_callbacks->sync_all(PC_POSTTOPDOWN) != FAILED);
		// check the top down values

		complex V2test[3];
		V2test[0]=load_node->voltageA ;
		V2test[1]=load_node->voltageB ;
		V2test[2]=load_node->voltageC ;

		CPPUNIT_ASSERT((load_node->voltageA - fs_V2_out[0]).IsZero(tolerance));
		CPPUNIT_ASSERT((load_node->voltageB - fs_V2_out[1]).IsZero(tolerance));
		CPPUNIT_ASSERT((load_node->voltageC - fs_V2_out[2]).IsZero(tolerance));
	}
};


#include "regulator_test.h"
#include "transformer_test.h"
#include "overheadline_test.h"
#include "undergroundline_test.h"
#include "relay_test.h"
#include "meter_test.h"
#include "triplexline_test.h"
#include "fuse_test.h"


//using namespace std;

/**
 * Sample unit test for a  Module.  There are basically 3 steps to creating a 
 * CPPUnit test:
 *     -# Write tests - create the methods of the test class that will
 *        exercise the module
 *     -# Create test Suite - Call the CPPUnit macros that will create
 *        the suite() function which will be called by the test runner
 *     -# Register the test suite with the CPPUnit factory.  This is
 *        done by using a single macro that takes the name of the test
 *        class as a parameter.
 *
 */

/*
 * Registers the test suite with the CPPUnit registration factory.  In most
 * cases, this is all that is required for it to be included and run by the
 * module_test method in test.cpp.
 */
CPPUNIT_TEST_SUITE_REGISTRATION(overhead_line_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(transformer_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(undergroundline_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(regulator_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(relay_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(meter_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(fuse_tests);
CPPUNIT_TEST_SUITE_REGISTRATION(triplex_line_tests);


#endif
#endif
