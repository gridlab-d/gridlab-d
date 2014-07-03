// $id$
//	Copyright (C) 2008 Battelle Memorial Institute
#ifndef _OVERHEADLINE_TEST_H
#define _OVERHEADLINE_TEST_H

#include "line.h"
#ifndef _NO_CPPUNIT
/**
 * Test class for overhead lines.  This class contains unit tests for different
 * configurations of overhead lines.
 */
class overhead_line_tests : public test_helper
{
	
	
public:
	overhead_line_tests(){}
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

	void test_node_create(){
		OBJECT* obj;
		CLASS *cl = get_class_by_name("node");
		obj = gl_create_object(cl);
		CPPUNIT_ASSERT(obj != NULL);
	}

	/**
	 * This function performs a unit test for overhead lines.  Overhead lines
	 * can't be tested independantly, but require a node at either end.  In this
	 * test, we will create 2 nodes, anchor them at either end of a single 
	 * overhead line, and then perform a series of sync's to perform the sweeps
	 * that will perform the calculations to find the current in the line at a
	 * given timestamp.
	 */
	void test_overhead_line_a(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		// Test values
		complex* node1_phA_I_test = new complex(-37.2922,60.5939);
		complex* node2_phA_V_test = new complex(-1017.2,2106.6);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_A|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_N;
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = -1017.4;
		OBJECTDATA(node1,node)->voltageA.SetImag(2106.5);
		OBJECTDATA(node2,load)->voltageA.SetImag(2106.5);
		OBJECTDATA(node2,load)->currentA.SetReal(-37.2922);
		OBJECTDATA(node2,load)->currentA.SetImag(60.5939);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 5.0249;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_a = gl_create_object(cl);
		OBJECTDATA(olc_a,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_a,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = 0;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageA = -1024.9;
		OBJECTDATA(node1,node)->voltageA.SetImag(2108.2);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_b(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase B test" << endl;
		complex* node1_phB_I_test = new complex(-37.2922,60.5939);
		complex* node2_phB_V_test = new complex(-1017.2,2106.6);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_B|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_B|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_B|PHASE_N;
		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1017.4;
		OBJECTDATA(node1,node)->voltageB.SetImag(2106.5);
		OBJECTDATA(node2,load)->voltageB.SetImag(2106.5);
		OBJECTDATA(node2,load)->currentB.SetReal(-37.2922);
		OBJECTDATA(node2,load)->currentB.SetImag(60.5939);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 5.0249;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_b = gl_create_object(cl);
		OBJECTDATA(olc_b,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_b,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;

		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);
		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentB.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentB.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phB_I_test->Re() ? rI_1 / node1_phB_I_test->Re() : node1_phB_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phB_I_test->Im() ? iI_1 / node1_phB_I_test->Im() : node1_phB_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageB = -1024.9;
		OBJECTDATA(node1,node)->voltageB.SetImag(2108.2);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_c(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase C test" << endl;
		complex* node1_phC_I_test = new complex(-37.2922,60.5939);
		complex* node2_phC_V_test = new complex(-1017.2,2106.6);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_C|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1017.4;
		OBJECTDATA(node1,node)->voltageC.SetImag(2106.5);
		OBJECTDATA(node2,load)->voltageC.SetImag(2106.5);
		OBJECTDATA(node2,load)->currentC.SetReal(-37.2922);
		OBJECTDATA(node2,load)->currentC.SetImag(60.5939);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 5.0249;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_c = gl_create_object(cl);
		OBJECTDATA(olc_c,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_c,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;

		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseC_conductor = olc_c;
		lc->phaseB_conductor = 0;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);
		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentC.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentC.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phC_I_test->Re() ? rI_1 / node1_phC_I_test->Re() : node1_phC_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phC_I_test->Im() ? iI_1 / node1_phC_I_test->Im() : node1_phC_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageC = -1024.9;
		OBJECTDATA(node1,node)->voltageC.SetImag(2108.2);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_ab(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase AB test" << endl;
		// Test values
		complex* node1_phA_I_test = new complex(34.73,55.192);
		complex* node2_phA_V_test = new complex(-1138.6,2150);
		complex* node1_phB_I_test = new complex(-34.73,-55.192);
		complex* node2_phB_V_test = new complex(-1311.7,-2101.1);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_A|PHASE_B|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_B|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_B|PHASE_N;
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = -1138.5;
		OBJECTDATA(node1,node)->voltageA.SetImag(2150.2);
		OBJECTDATA(node2,load)->voltageA.SetImag(2150.2);
		OBJECTDATA(node2,load)->currentA.SetReal(34.73);
		OBJECTDATA(node2,load)->currentA.SetImag(55.192);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1311.5;
		OBJECTDATA(node1,node)->voltageB.SetImag(2100.4);
		OBJECTDATA(node2,load)->voltageB.SetImag(2100.4);
		OBJECTDATA(node2,load)->currentB.SetReal(-34.73);
		OBJECTDATA(node2,load)->currentB.SetImag(-55.192);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 5.66;
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 5.32;
		OBJECTDATA(ls,line_spacing)->distance_AtoB = 7;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_a = gl_create_object(cl);
		OBJECTDATA(olc_a,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_a,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_b = gl_create_object(cl);
		OBJECTDATA(olc_b,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_b,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageA = -1139.2;
		OBJECTDATA(node1,node)->voltageA.SetImag(2155.2);
		OBJECTDATA(node1,node)->voltageB = -1311.1;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2106.3);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_ac(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase AC test" << endl;
		// Test values
		complex* node1_phA_I_test = new complex(48.931,-39.794);
		complex* node2_phA_V_test = new complex(2362.9,-220.14);
		complex* node1_phC_I_test = new complex(-37.292,60.594);
		complex* node2_phC_V_test = new complex(-1024.6,2108.3);


		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_A|PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_C|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2362.7;
		OBJECTDATA(node1,node)->voltageA.SetImag(-220.02);
		OBJECTDATA(node2,load)->voltageA.SetImag(-220.02);
		OBJECTDATA(node2,load)->currentA.SetReal(48.931);
		OBJECTDATA(node2,load)->currentA.SetImag(-39.794);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1024.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2108.3);
		OBJECTDATA(node2,load)->voltageC.SetImag(2108.3);
		OBJECTDATA(node2,load)->currentC.SetReal(-37.292);
		OBJECTDATA(node2,load)->currentC.SetImag(60.594);


		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 5.66;
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 5.32;
		OBJECTDATA(ls,line_spacing)->distance_AtoC = 7;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_a = gl_create_object(cl);
		OBJECTDATA(olc_a,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_a,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_c = gl_create_object(cl);
		OBJECTDATA(olc_c,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_c,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;


		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = 0;
		lc->phaseC_conductor = olc_c;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageA = 2367.6;
		OBJECTDATA(node1,node)->voltageA.SetImag(-219.64);
		OBJECTDATA(node1,node)->voltageC = -1030.4;
		OBJECTDATA(node1,node)->voltageC.SetImag(2110.9);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_bc(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase BC test" << endl;
		// Test values
		complex* node1_phB_I_test = new complex(-34.73,-55.192);
		complex* node2_phB_V_test = new complex(-1311.7,-2101.1);
		complex* node1_phC_I_test = new complex(34.73,55.192);
		complex* node2_phC_V_test = new complex(-1138.6,2150);


		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 300;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_B|PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->phases = PHASE_B|PHASE_C|PHASE_N;
		OBJECTDATA(node2,load)->phases = PHASE_B|PHASE_C|PHASE_N;
		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1311.5;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2100.4);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2100.4);
		OBJECTDATA(node2,load)->currentB.SetReal(-34.73);
		OBJECTDATA(node2,load)->currentB.SetImag(-55.192);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1138.5;
		OBJECTDATA(node1,node)->voltageC.SetImag(2150.2);
		OBJECTDATA(node2,load)->voltageC.SetImag(2150.2);
		OBJECTDATA(node2,load)->currentC.SetReal(34.73);
		OBJECTDATA(node2,load)->currentC.SetImag(55.192);


		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 5.32;
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 5.66;
		OBJECTDATA(ls,line_spacing)->distance_BtoC = 7;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_b = gl_create_object(cl);
		OBJECTDATA(olc_b,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_b,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_c = gl_create_object(cl);
		OBJECTDATA(olc_c,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_c,overhead_line_conductor)->resistance = 1.12;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;


		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = olc_c;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentB.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentB.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phB_I_test->Re() ? rI_1 / node1_phB_I_test->Re() : node1_phB_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phB_I_test->Im() ? iI_1 / node1_phB_I_test->Im() : node1_phB_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageB = -1311.1;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2106.3);
		OBJECTDATA(node1,node)->voltageC = -1139.2;
		OBJECTDATA(node1,node)->voltageC.SetImag(2155.2);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_abc_delta(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase ABC_Delta test" << endl;
		// Test values
		complex* node1_phA_I_test = new complex(108.4,-47.132);
		complex* node2_phA_V_test = new complex(7171.2,-17.944);

		complex* node1_phB_I_test = new complex(-111.34,-100.96);
		complex* node2_phB_V_test = new complex(-3613.5,-6200.6);

		complex* node1_phC_I_test = new complex(2.847,148.27);
		complex* node2_phC_V_test = new complex(-3563.5,6220.9);
//----------------------------------------------------------------------

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 2000;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 7943.2;
		OBJECTDATA(node1,node)->voltageA.SetImag(458);
		OBJECTDATA(node2,load)->voltageA.SetImag(458);
		OBJECTDATA(node2,load)->currentA.SetReal(108.4);
		OBJECTDATA(node2,load)->currentA.SetImag(-47.132);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -2916.9;
		OBJECTDATA(node1,node)->voltageB.SetImag(-6740.4);
		OBJECTDATA(node2,load)->voltageB.SetImag(-6740.4);
		OBJECTDATA(node2,load)->currentB.SetReal(-111.34);
		OBJECTDATA(node2,load)->currentB.SetImag(-100.96);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -3879.1;
		OBJECTDATA(node1,node)->voltageC.SetImag(6585.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(6585.4);
		OBJECTDATA(node2,load)->currentC.SetReal(2.847);
		OBJECTDATA(node2,load)->currentC.SetImag(148.27);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 0;
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 0;
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 0;
		OBJECTDATA(ls,line_spacing)->distance_AtoB = 2.5;
		OBJECTDATA(ls,line_spacing)->distance_BtoC = 4.5;
		OBJECTDATA(ls,line_spacing)->distance_AtoC = 7;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_a = gl_create_object(cl);
		OBJECTDATA(olc_a,overhead_line_conductor)->geometric_mean_radius = .0244;
		OBJECTDATA(olc_a,overhead_line_conductor)->resistance = .278;

		OBJECT *olc_b = gl_create_object(cl);
		OBJECTDATA(olc_b,overhead_line_conductor)->geometric_mean_radius = .0244;
		OBJECTDATA(olc_b,overhead_line_conductor)->resistance = .278;

		OBJECT *olc_c = gl_create_object(cl);
		OBJECTDATA(olc_c,overhead_line_conductor)->geometric_mean_radius = .0244;
		OBJECTDATA(olc_c,overhead_line_conductor)->resistance = .278;

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(overhead_line_conductor));
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = 0;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = olc_c;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageA = 7199.6;
		OBJECTDATA(node1,node)->voltageA.SetImag(0);
		OBJECTDATA(node1,node)->voltageB = -3599.8;
		OBJECTDATA(node1,node)->voltageB.SetImag(-6235);
		OBJECTDATA(node1,node)->voltageC = -3599.8;
		OBJECTDATA(node1,node)->voltageC.SetImag(6235);

		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_overhead_line_abcn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // Overhead line object
		//cout << "Phase ABC_WYE test" << endl;
		// Test values
		complex* node1_phA_I_test = new complex(64.315,-49.78);
		complex* node2_phA_V_test = new complex(2441.9,-108.49);

		complex* node1_phB_I_test = new complex(-57.095,-21.814);
		complex* node2_phB_V_test = new complex(-1315.5,-2124.3);

		complex* node1_phC_I_test = new complex(10.383,61.845);
		complex* node2_phC_V_test = new complex(-1136.9,2155.1);
//----------------------------------------------------------------------

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("overhead_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,overhead_line)->create();

		OBJECTDATA(ohl1,overhead_line)->from = node1;
		OBJECTDATA(ohl1,overhead_line)->to = node2;
		OBJECTDATA(ohl1,overhead_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,overhead_line)->phases = PHASE_ABCN;
		OBJECTDATA(node1,node)->phases = PHASE_ABCN;
		OBJECTDATA(node2,load)->phases = PHASE_ABCN;
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2442.6;
		OBJECTDATA(node1,node)->voltageA.SetImag(-109.21);
		OBJECTDATA(node2,load)->voltageA.SetImag(-109.21);
		OBJECTDATA(node2,load)->currentA.SetReal(64.315);
		OBJECTDATA(node2,load)->currentA.SetImag(-49.78);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1315.1;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2123.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2123.6);
		OBJECTDATA(node2,load)->currentB.SetReal(-57.095);
		OBJECTDATA(node2,load)->currentB.SetImag(-21.814);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1137.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2156);
		OBJECTDATA(node2,load)->voltageC.SetImag(2156);
		OBJECTDATA(node2,load)->currentC.SetReal(10.383);
		OBJECTDATA(node2,load)->currentC.SetImag(61.845);
		
		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 4.12;
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 5.32;
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 5.66;
		OBJECTDATA(ls,line_spacing)->distance_AtoB = 2.5;
		OBJECTDATA(ls,line_spacing)->distance_BtoC = 7;
		OBJECTDATA(ls,line_spacing)->distance_AtoC = 4.5;
		
		cl = get_class_by_name("overhead_line_conductor");
		OBJECT *olc_a = gl_create_object(cl);
		OBJECTDATA(olc_a,overhead_line_conductor)->geometric_mean_radius = .0081;
		OBJECTDATA(olc_a,overhead_line_conductor)->resistance = .592;

		OBJECT *olc_b = gl_create_object(cl);
		OBJECTDATA(olc_b,overhead_line_conductor)->geometric_mean_radius = .0081;
		OBJECTDATA(olc_b,overhead_line_conductor)->resistance = .592;

		OBJECT *olc_c = gl_create_object(cl);
		OBJECTDATA(olc_c,overhead_line_conductor)->geometric_mean_radius = .0081;
		OBJECTDATA(olc_c,overhead_line_conductor)->resistance = .592;

		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,overhead_line_conductor)->geometric_mean_radius = .0081;
		OBJECTDATA(olc_n,overhead_line_conductor)->resistance = .592;
		

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = olc_a;
		lc->phaseB_conductor = olc_b;
		lc->phaseC_conductor = olc_c;
		lc->phaseN_conductor = olc_n;
		
		OBJECTDATA(ohl1,overhead_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
		OBJECTDATA(node1,node)->voltageA = 2449.9;
		OBJECTDATA(node1,node)->voltageA.SetImag(-106.54);
		OBJECTDATA(node1,node)->voltageB = -1315.8;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2128.8);
		OBJECTDATA(node1,node)->voltageC = -1140.3;
		OBJECTDATA(node1,node)->voltageC.SetImag(2160.1);

		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(overhead_line_tests);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	CPPUNIT_TEST(test_overhead_line_a);
	CPPUNIT_TEST(test_overhead_line_b);
	CPPUNIT_TEST(test_overhead_line_c);
	CPPUNIT_TEST(test_overhead_line_ab);
	CPPUNIT_TEST(test_overhead_line_ac);
	CPPUNIT_TEST(test_overhead_line_bc);
	CPPUNIT_TEST(test_overhead_line_abc_delta);
	CPPUNIT_TEST(test_overhead_line_abcn);
	CPPUNIT_TEST_SUITE_END();

	
};
#endif
#endif
