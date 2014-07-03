// $Id: triplexline_test.h 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRIPLEXLINE_TEST_H
#define _TRIPLEXLINE_TEST_H

#include "line.h"

#ifndef _NO_CPPUNIT

/**
 * Test class for triplex lines.  This class contains unit tests for different
 * configurations of triplex lines.
 */
class triplex_line_tests : public test_helper
{
	
	
public:
	triplex_line_tests(){}
	/**
	 * Called by CPPUnit to ensure that any special pre-testing steps are
	 * completed before the tests are executed.
	 */
	void setUp(){
	}

	void tearDown(){
		local_callbacks->remove_objects();
	}

	/**
	 * Test the creation of an object.  Part of this test includes
	 * retrieving the class that this object will represent, creating the
	 * object, and testing certain post creation assumptions.
	 *
	 * For each feature of the module that we want to test, we should create
	 * a separate test function.  A function prototype is acceptable here,
	 * with the implementation of the method in a .cpp file.
	 */
	/*void test_create(){
		CLASS *cl = get_class_by_name("sample");
		CPPUNIT_ASSERT(cl != NULL);
		obj = gl_create_object(cl,sizeof(sample_obj)); // Sample_obj is the name of the cpp class or struct.
		CPPUNIT_ASSERT(obj != NULL);
		
	}*/

	/**
	 * This function performs a unit test for triplex lines.  Triplex lines
	 * can't be tested independantly, but require a node at either end.  In this
	 * test, we will create 2 nodes, anchor them at either end of a single 
	 * triplex line, and then perform a series of sync's to perform the sweeps
	 * that will perform the calculations to find the current in the line at a
	 * given timestamp.
	 */
	void test_triplex_line(){
		OBJECT *node1,*node2; // Triplex Node objects
		OBJECT *tpl1; // Triplex line object
		// Test values
		complex* node1_phA_I_test = new complex(159.91,-79.788);
		complex* node1_phB_I_test = new complex(-199.4,106.36);
		complex* node1_phC_I_test = new complex(39.494,-26.569);

		complex* node2_phA_V_test = new complex(115.44,-.26192);
		complex* node2_phB_V_test = new complex(122.45,-2.542);
		complex* node2_phC_V_test = new complex(0,0);

		CLASS *cl = get_class_by_name("triplex_node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,triplex_node)->create();
		cl = get_class_by_name("triplex_node");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,triplex_node)->create();
		cl = get_class_by_name("triplex_line");
		tpl1 = gl_create_object(cl);
		OBJECTDATA(tpl1,triplex_line)->create();

		OBJECTDATA(tpl1,triplex_line)->from = node1;
		OBJECTDATA(tpl1,triplex_line)->to = node2;
		OBJECTDATA(tpl1,triplex_line)->length = 100;

		// Next we set up the parameters needed for the test
		OBJECTDATA(tpl1,triplex_line)->phases = PHASE_ABCN;
		OBJECTDATA(node1,triplex_node)->phases = PHASE_ABCN;
		OBJECTDATA(node2,triplex_node)->phases = PHASE_ABCN;
		OBJECTDATA(node1,triplex_node)->voltageA = OBJECTDATA(node2,triplex_node)->voltageA = complex(115.44,-0.26192);
		OBJECTDATA(node2,triplex_node)->currentA = complex(159.91,-79.788);
		OBJECTDATA(node1,triplex_node)->voltageB = OBJECTDATA(node2,triplex_node)->voltageB = complex(122.45,-2.5421);
		OBJECTDATA(node2,triplex_node)->currentB = complex(-199.4,106.36);
		OBJECTDATA(node1,triplex_node)->voltageC = OBJECTDATA(node2,triplex_node)->voltageC = complex(0,0);
		OBJECTDATA(node2,triplex_node)->currentC = complex(39.494,-26.569);
		
		cl = get_class_by_name("triplex_line_conductor");
		OBJECT *olc_a = gl_create_object(cl); // L1 conductor
		OBJECTDATA(olc_a,triplex_line_conductor)->geometric_mean_radius = 0.0111;
		OBJECTDATA(olc_a,triplex_line_conductor)->resistance = 0.97;

		OBJECT *olc_b = gl_create_object(cl); // L2 conductor
		OBJECTDATA(olc_b,triplex_line_conductor)->geometric_mean_radius = 0.0111;
		OBJECTDATA(olc_b,triplex_line_conductor)->resistance = 0.97;

		OBJECT *olc_c = gl_create_object(cl); // LN conductor
		OBJECTDATA(olc_c,triplex_line_conductor)->geometric_mean_radius = 0.0111;
		OBJECTDATA(olc_c,triplex_line_conductor)->resistance = 0.97;

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("triplex_line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		triplex_line_configuration *lc = OBJECTDATA(line_config,triplex_line_configuration);
		//lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = olc_c;
		lc->phaseN_conductor = NULL;
		lc->ins_thickness = 0.08; // replace with test value
		lc->diameter = 0.368; // replace with test value
		
		OBJECTDATA(tpl1,triplex_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,triplex_node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,triplex_node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,triplex_node)->voltageA = -1024.9;
		OBJECTDATA(node1,triplex_node)->voltageA.SetImag(2108.2);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,triplex_node)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,triplex_node)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(triplex_line_tests);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	CPPUNIT_TEST(test_triplex_line);
	CPPUNIT_TEST_SUITE_END();

	
};

#endif
#endif
