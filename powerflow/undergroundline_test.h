// $id$
//	Copyright (C) 2008 Battelle Memorial Institute
#ifndef _UNDERGROUNDLINE_TEST_H
#define _UNDERGROUNDLINE_TEST_H

#include "line.h"

#ifndef _NO_CPPUNIT

class undergroundline_tests : public test_helper
{
public:
	void setUp(){
	}

	void tearDown(){
		local_callbacks->remove_objects();
	}

	/**
	 * Test for a single A phase underground line with a concentric neutral.
	 */
	void test_underground_line_a_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phA_I_test = new complex(204.5,-18.431);
		complex* node2_phA_V_test = new complex(2345.9,-230.99);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		//OBJECTDATA(ohl1,underground_line)->phase = OBJECTDATA(node1,node)->phase = OBJECTDATA(node2,load)->phase = PHASEA;
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_A;
		OBJECTDATA(node1,node)->phases = PHASE_A;
		OBJECTDATA(node2,load)->phases = PHASE_A;
		
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2349.8;
		OBJECTDATA(node1,node)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->currentA.SetReal(204.5);
		OBJECTDATA(node2,load)->currentA.SetImag(-18.431);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_a = gl_create_object(cl);
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 0.0;//no extra neutral
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(underground_line_conductor));
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = ulc_a;
		lc->phaseB_conductor = 0;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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
		OBJECTDATA(node1,node)->voltageA.SetImag(-220.05);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test->Re() ? rV_2 / node2_phA_V_test->Re() : node2_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phA_V_test->Im() ? iV_2 / node2_phA_V_test->Im() : node2_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_underground_line_b_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phB_I_test = new complex(39.716,-57.144);
		complex* node2_phB_V_test = new complex(-1360.5,2140);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_B;
		OBJECTDATA(node1,node)->phases = PHASE_B;
		OBJECTDATA(node2,load)->phases = PHASE_B;
		
		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1362.2;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->currentB.SetReal(39.716);
		OBJECTDATA(node2,load)->currentB.SetImag(-57.144);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_b = gl_create_object(cl);
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_b,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 0.0;//no extra neutral
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(underground_line_conductor));
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseB_conductor = ulc_b;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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

		
		OBJECTDATA(node1,node)->voltageB = -1352.9;
		OBJECTDATA(node1,node)->voltageB.SetImag(2136.8);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageB.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test->Re() ? rV_2 / node2_phB_V_test->Re() : node2_phB_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test->Im() ? iV_2 / node2_phB_V_test->Im() : node2_phB_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}

	void test_underground_line_c_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phC_I_test = new complex(-46.035,115.21);
		complex* node2_phC_V_test = new complex(-1018.6,2102.2);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_C;
		OBJECTDATA(node2,load)->phases = PHASE_C;
		
		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1028.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->currentC.SetReal(-46.035);
		OBJECTDATA(node2,load)->currentC.SetImag(115.21);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_c = gl_create_object(cl);
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_c,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_CtoN = 0.0;//no extra neutral
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(underground_line_conductor));
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseB_conductor = 0;
		lc->phaseC_conductor = ulc_c;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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

		
		OBJECTDATA(node1,node)->voltageC = -1030.4;
		OBJECTDATA(node1,node)->voltageC.SetImag(2110.9);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		double iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

	}



	void test_underground_line_ab_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phA_I_test = new complex(204.5,-18.431);
		complex* node2_phA_V_test = new complex(2347.4,-225.7);
		complex* node1_phB_I_test = new complex(39.716,-57.144);
		complex* node2_phB_V_test = new complex(-1367.1,-2134.6);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_A|PHASE_B;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_B;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_B;
		
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2349.8;
		OBJECTDATA(node1,node)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->currentA.SetReal(204.5);
		OBJECTDATA(node2,load)->currentA.SetImag(-18.431);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1362.2;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->currentB.SetReal(39.716);
		OBJECTDATA(node2,load)->currentB.SetImag(-57.144);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_a = gl_create_object(cl);
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_strands = 13;

		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_b = gl_create_object(cl);
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_b,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 0.0;//no extra neutral
		OBJECTDATA(ls,line_spacing)->distance_AtoB = 0.5;
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl);
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = ulc_a;
		lc->phaseB_conductor = ulc_b;
		lc->phaseC_conductor = 0;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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
		OBJECTDATA(node1,node)->voltageA.SetImag(-220.05);
		OBJECTDATA(node1,node)->voltageB = -1352.9;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.8);
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

	void test_underground_line_bc_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phB_I_test = new complex(39.716,-57.144);
		complex* node2_phB_V_test = new complex(-1356.1,-2138);
		complex* node1_phC_I_test = new complex(-46.035,115.21);
		complex* node2_phC_V_test = new complex(-1023.4,2105.1);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_B|PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_B|PHASE_C;
		OBJECTDATA(node2,load)->phases = PHASE_B|PHASE_C;
		
		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1362.2;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->currentB.SetReal(39.716);
		OBJECTDATA(node2,load)->currentB.SetImag(-57.144);
		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1028.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->currentC.SetReal(-46.035);
		OBJECTDATA(node2,load)->currentC.SetImag(115.21);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_b = gl_create_object(cl);
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_b,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_strands = 13;

		OBJECT *ulc_c = gl_create_object(cl);
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_c,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_BtoN = 0.0;//no extra neutral
		OBJECTDATA(ls,line_spacing)->distance_BtoC = 0.5;
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(underground_line_conductor));
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = 0;
		lc->phaseB_conductor = ulc_b;
		lc->phaseC_conductor = ulc_c;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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

		
		OBJECTDATA(node1,node)->voltageB = -1352.88;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.8);
		OBJECTDATA(node1,node)->voltageC = -1030.46;
		OBJECTDATA(node1,node)->voltageC.SetImag(2110.809);
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

	void test_underground_line_ca_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phA_I_test = new complex(204.5,-18.431);
		complex* node2_phA_V_test = new complex(2351.4,-232.45);
		complex* node1_phC_I_test = new complex(-46.035,115.21);
		complex* node2_phC_V_test = new complex(-1028.6,2102.9);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_A|PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_C;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_C;

		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2349.8;
		OBJECTDATA(node1,node)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->currentA.SetReal(204.5);
		OBJECTDATA(node2,load)->currentA.SetImag(-18.431);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1028.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->currentC.SetReal(-46.035);
		OBJECTDATA(node2,load)->currentC.SetImag(115.21);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_a = gl_create_object(cl);
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_strands = 13;

		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_c = gl_create_object(cl);
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_c,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 0.0;//no extra neutral
		OBJECTDATA(ls,line_spacing)->distance_AtoC = 1;
			//(OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter - 
			//										  OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter)/24.0;
		
		

		/*
		OBJECT *olc_n = gl_create_object(cl,sizeof(underground_line_conductor));
		OBJECTDATA(olc_n,underground_line_conductor)->geometric_mean_radius = .00446;
		OBJECTDATA(olc_n,underground_line_conductor)->resistance = 1.12;
		*/

		// Do I need to set up the other conductors and set them to INF?
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = ulc_a;
		lc->phaseB_conductor = 0;
		lc->phaseC_conductor = ulc_c;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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
		OBJECTDATA(node1,node)->voltageA.SetImag(-220.05);
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

	void test_underground_line_abc_cn(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *ohl1; // underground line object
		// Test values
		complex* node1_phA_I_test = new complex(204.5,-18.431);
		complex* node2_phA_V_test = new complex(2351,-220.05);
		complex* node1_phB_I_test = new complex(39.716,-57.144);
		complex* node2_phB_V_test = new complex(-1362.6,-2136.8);
		complex* node1_phC_I_test = new complex(-46.035,115.21);
		complex* node2_phC_V_test = new complex(-1029,2106.5);

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node2,load)->phases = PHASE_A|PHASE_B|PHASE_C;
		
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2349.8;
		OBJECTDATA(node1,node)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->voltageA.SetImag(-228.75);
		OBJECTDATA(node2,load)->currentA.SetReal(204.5);
		OBJECTDATA(node2,load)->currentA.SetImag(-18.431);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1362.2;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(-2136.6);
		OBJECTDATA(node2,load)->currentB.SetReal(39.716);
		OBJECTDATA(node2,load)->currentB.SetImag(-57.144);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1028.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(2106.4);
		OBJECTDATA(node2,load)->currentC.SetReal(-46.035);
		OBJECTDATA(node2,load)->currentC.SetImag(115.21);
		
		// Phase A line conductor
		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_a = gl_create_object(cl);
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_a,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_a,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_a,underground_line_conductor)->neutral_strands = 13;

		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_b = gl_create_object(cl);
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_b,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_b,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_b,underground_line_conductor)->neutral_strands = 13;

		cl = get_class_by_name("underground_line_conductor");
		OBJECT *ulc_c = gl_create_object(cl);
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_diameter = 0.567;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_gmr = 0.0171;
		OBJECTDATA(ulc_c,underground_line_conductor)->conductor_resistance = 0.41;
		OBJECTDATA(ulc_c,underground_line_conductor)->outer_diameter = 1.29;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_gmr = 0.00208;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_diameter = 0.0641;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_resistance = 14.8722;
		OBJECTDATA(ulc_c,underground_line_conductor)->neutral_strands = 13;


		cl = get_class_by_name("line_spacing");
		OBJECT *ls = gl_create_object(cl);
		OBJECTDATA(ls,line_spacing)->distance_AtoN = 0.0;//no extra neutral
		OBJECTDATA(ls,line_spacing)->distance_AtoB = 0.5;
		OBJECTDATA(ls,line_spacing)->distance_AtoC = 1;
		OBJECTDATA(ls,line_spacing)->distance_BtoC = 0.5;
		
		cl = get_class_by_name("line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		line_configuration *lc = OBJECTDATA(line_config,line_configuration);
		lc->line_spacing = ls;
		lc->phaseA_conductor = ulc_a;
		lc->phaseB_conductor = ulc_b;
		lc->phaseC_conductor = ulc_c;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl1,underground_line)->configuration = line_config;
		
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
		OBJECTDATA(node1,node)->voltageA.SetImag(-220.05);
		OBJECTDATA(node1,node)->voltageB = -1352.9;
		OBJECTDATA(node1,node)->voltageB.SetImag(-2136.8);
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
	CPPUNIT_TEST_SUITE(undergroundline_tests);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */

	
	CPPUNIT_TEST(test_underground_line_a_cn);
	CPPUNIT_TEST(test_underground_line_b_cn);
	CPPUNIT_TEST(test_underground_line_c_cn);
	CPPUNIT_TEST(test_underground_line_bc_cn);
	CPPUNIT_TEST(test_underground_line_ab_cn);
	CPPUNIT_TEST(test_underground_line_ca_cn);
	CPPUNIT_TEST(test_underground_line_abc_cn);
	
	CPPUNIT_TEST_SUITE_END();

};


#endif
#endif
