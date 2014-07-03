// $id$
//	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _TRANSFORMER_TEST_H
#define _TRANSFORMER_TEST_H

#include "transformer.h"

#ifndef _NO_CPPUNIT

#ifdef VALIDATE_THRESHOLD
#undef VALIDATE_THRESHOLD
#endif
#define VALIDATE_THRESHOLD .0009


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

class transformer_tests : public test_helper 
{	
 

	void test_delta_delta_transformer(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *tformer; // Overhead line object

		// Calculated results
		complex* node1_phA_I_test = new complex(276.11,-191.18);
		complex* node1_phB_I_test = new complex(-303.37,-144.05);
		complex* node1_phC_I_test = new complex(26.947,334.92);
		
		complex* node2_phA_V_test = new complex(2250.5,-140.2);
		complex* node2_phB_V_test = new complex(-1249.9,-1883.2);
		complex* node2_phC_V_test = new complex(-1000.6,2023.4);

		// setup test objects
		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("transformer");
		tformer = gl_create_object(cl);
		OBJECTDATA(tformer,transformer)->create();

		OBJECTDATA(tformer,transformer)->from = node1;
		OBJECTDATA(tformer,transformer)->to = node2;

		SET_ADD(OBJECTDATA(tformer,transformer)->phases,PHASE_ABCN);
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2251.5;
		OBJECTDATA(node1,node)->voltageA.SetImag(-138.71);
		OBJECTDATA(node2,load)->voltageA.SetImag(-138.71);
		OBJECTDATA(node2,load)->currentA.SetReal(827.65);
		OBJECTDATA(node2,load)->currentA.SetImag(-573.09);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1248;
		OBJECTDATA(node1,node)->voltageB.SetImag(-1883.9);
		OBJECTDATA(node2,load)->voltageB.SetImag(-1883.9);
		OBJECTDATA(node2,load)->currentB.SetReal(-909.39);
		OBJECTDATA(node2,load)->currentB.SetImag(-431.81);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1001.7;
		OBJECTDATA(node1,node)->voltageC.SetImag(2022.4);
		OBJECTDATA(node2,load)->voltageC.SetImag(2022.4);
		OBJECTDATA(node2,load)->currentC.SetReal(80.776);
		OBJECTDATA(node2,load)->currentC.SetImag(1004);

		// Set up the transformer variables
		cl = get_class_by_name("transformer_configuration");
		OBJECT *trans_config = gl_create_object(cl);
		OBJECTDATA(trans_config,transformer_configuration)->V_primary = 12.47 * 1000; 
		OBJECTDATA(trans_config,transformer_configuration)->V_secondary = 4.16 * 1000; 
 		OBJECTDATA(trans_config,transformer_configuration)->kVA_rating = 6000;
		//OBJECTDATA(trans_config,transformer_configuration)->R_pu = .01;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetReal(.01);
		//OBJECTDATA(trans_config,transformer_configuration)->X_pu = .06;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetImag(.06);
		OBJECTDATA(trans_config,transformer_configuration)->connect_type = transformer_configuration::DELTA_DELTA;//delta-delta
		OBJECTDATA(tformer,transformer)->configuration = trans_config;

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		OBJECTDATA(node1,node)->voltageA = 7116.6;
		OBJECTDATA(node1,node)->voltageA.SetImag(-40.59);
		OBJECTDATA(node1,node)->voltageB = -3599.8;
		OBJECTDATA(node1,node)->voltageB.SetImag(-6154.2);
		OBJECTDATA(node1,node)->voltageC = -3512;
		OBJECTDATA(node1,node)->voltageC.SetImag(6194.4);

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
      
	void test_delta_Gwye_transformer(){

		OBJECT *node1,*node2; // Node objects
		OBJECT *tformer; // Overhead line object

		// Calculated results
		complex* node1_phA_I_test = new complex(20.26,-36.94);
		complex* node1_phB_I_test = new complex(-39.81,-0.95);
		complex* node1_phC_I_test = new complex(19.55,37.89);

		complex* node2_phA_V_test = new complex(2890.054,-6201.529);
		complex* node2_phB_V_test = new complex(-6513.450,1037.498);
		complex* node2_phC_V_test = new complex(3601.730,5403.961);

		// setup test objects
		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("transformer");
		tformer = gl_create_object(cl);
		OBJECTDATA(tformer,transformer)->create();

		OBJECTDATA(tformer,transformer)->from = node1;
		OBJECTDATA(tformer,transformer)->to = node2;

		SET_ADD(OBJECTDATA(tformer,transformer)->phases,PHASE_ABC);
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2891.6;
		OBJECTDATA(node1,node)->voltageA.SetImag(-6201.1);
		OBJECTDATA(node2,load)->voltageA.SetImag(-6201.1);
		OBJECTDATA(node2,load)->currentA.SetReal(-25.34);
		OBJECTDATA(node2,load)->currentA.SetImag(-483.44);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -6513.3;
		OBJECTDATA(node1,node)->voltageB.SetImag(1031.6);
		OBJECTDATA(node2,load)->voltageB.SetImag(1031.6);
		OBJECTDATA(node2,load)->currentB.SetReal(-413.66);
		OBJECTDATA(node2,load)->currentB.SetImag(224.6);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = 3659.1;
		OBJECTDATA(node1,node)->voltageC.SetImag(5486.7);
		OBJECTDATA(node2,load)->voltageC.SetImag(5486.7);
		OBJECTDATA(node2,load)->currentC.SetReal(349.32);
		OBJECTDATA(node2,load)->currentC.SetImag(242.78);

		// Set up the transformer variables
		cl = get_class_by_name("transformer_configuration");
		OBJECT *trans_config = gl_create_object(cl);
		OBJECTDATA(trans_config,transformer_configuration)->V_primary = 138*1000;
		OBJECTDATA(trans_config,transformer_configuration)->V_secondary = 12.47*1000;
		OBJECTDATA(trans_config,transformer_configuration)->kVA_rating = 5000;
//		OBJECTDATA(trans_config,transformer_configuration)->R_pu = 0.0074;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetReal(0.0074);
//		OBJECTDATA(trans_config,transformer_configuration)->X_pu = 0.0847;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetImag(0.0847);
		OBJECTDATA(trans_config,transformer_configuration)->connect_type = transformer_configuration::DELTA_GWYE;		
		OBJECTDATA(tformer,transformer)->configuration = trans_config;

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		OBJECTDATA(node1,node)->voltageA = 72555;
		OBJECTDATA(node1,node)->voltageA.SetImag(-40767);
		OBJECTDATA(node1,node)->voltageB = -65459;
		OBJECTDATA(node1,node)->voltageB.SetImag(-40776);
		OBJECTDATA(node1,node)->voltageC = -7139;
		OBJECTDATA(node1,node)->voltageC.SetImag(81515);

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

	void test_wye_wye_transformer(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *tformer; // Overhead line object

		// Calculated results
		complex* node1_phA_I_test = new complex(285.25,-199.1);
		complex* node1_phB_I_test = new complex(-291.28,-141.09);
		complex* node1_phC_I_test = new complex(29.262,335.54);
		
		complex* node2_phA_V_test = new complex(2242.9,-144.8);
		complex* node2_phB_V_test = new complex(-1251.3,-1892.4);
		complex* node2_phC_V_test = new complex(-1002.9,2020.8);

		// setup test objects
		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("transformer");
		tformer = gl_create_object(cl);
		OBJECTDATA(tformer,transformer)->create();

		OBJECTDATA(tformer,transformer)->from = node1;
		OBJECTDATA(tformer,transformer)->to = node2;

		SET_ADD(OBJECTDATA(tformer,transformer)->phases,PHASE_ABCN);
		OBJECTDATA(node1,node)->voltageA = OBJECTDATA(node2,load)->voltageA = 2242.9;
		OBJECTDATA(node1,node)->voltageA.SetImag(-114.8);
		OBJECTDATA(node2,load)->voltageA.SetImag(-114.8);
		OBJECTDATA(node2,load)->currentA.SetReal(855.08);
		OBJECTDATA(node2,load)->currentA.SetImag(-596.83);

		OBJECTDATA(node1,node)->voltageB = OBJECTDATA(node2,load)->voltageB = -1251.3;
		OBJECTDATA(node1,node)->voltageB.SetImag(-1892.4);
		OBJECTDATA(node2,load)->voltageB.SetImag(-1892.4);
		OBJECTDATA(node2,load)->currentB.SetReal(-873.15);
		OBJECTDATA(node2,load)->currentB.SetImag(-422.93);

		OBJECTDATA(node1,node)->voltageC = OBJECTDATA(node2,load)->voltageC = -1002.9;
		OBJECTDATA(node1,node)->voltageC.SetImag(2020.8);
		OBJECTDATA(node2,load)->voltageC.SetImag(2020.8);
		OBJECTDATA(node2,load)->currentC.SetReal(87.717);
		OBJECTDATA(node2,load)->currentC.SetImag(1005.8);

		// Set up the transformer variables
		cl = get_class_by_name("transformer_configuration");
		OBJECT *trans_config = gl_create_object(cl);
		OBJECTDATA(trans_config,transformer_configuration)->V_primary = 12.47 * 1000; 
		OBJECTDATA(trans_config,transformer_configuration)->V_secondary = 4.16 * 1000; 
 		OBJECTDATA(trans_config,transformer_configuration)->kVA_rating = 6000;
		//OBJECTDATA(trans_config,transformer_configuration)->R_pu = .01;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetReal(0.01);
		//OBJECTDATA(trans_config,transformer_configuration)->X_pu = .06;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetImag(0.06);
		OBJECTDATA(trans_config,transformer_configuration)->connect_type = transformer_configuration::WYE_WYE;
		OBJECTDATA(tformer,transformer)->configuration = trans_config;

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		//CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		OBJECTDATA(node1,node)->voltageA = 7106.9;
		OBJECTDATA(node1,node)->voltageA.SetImag(-42.069);
		OBJECTDATA(node1,node)->voltageB = -3607.1;
		OBJECTDATA(node1,node)->voltageB.SetImag(-6162);
		OBJECTDATA(node1,node)->voltageC = -3520.6;
		OBJECTDATA(node1,node)->voltageC.SetImag(6190.1);

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


	void test_center_tapped_transformer_Aphase(){
		OBJECT *node1,*node2; // Node objects
		OBJECT *tformer; // Overhead line object

		// Calculated results
		complex* node1_phA_I_test = new complex(5.9265,-3.0529);
		complex* node1_phB_I_test = new complex(0,0);
		complex* node1_phC_I_test = new complex(0,0);

		complex* node2_phA_V_test = new complex(118,-1.2627);
		complex* node2_phB_V_test = new complex(117.85,-1.2634);
		complex* node2_phC_V_test = new complex(0,0);

		// setup test objects
		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		cl = get_class_by_name("load");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,load)->create();
		cl = get_class_by_name("transformer");
		tformer = gl_create_object(cl);
		OBJECTDATA(tformer,transformer)->create();

		OBJECTDATA(tformer,transformer)->from = node1;
		OBJECTDATA(tformer,transformer)->to = node2;

		SET_ADD(OBJECTDATA(tformer,transformer)->phases,PHASE_A|PHASE_S);
		OBJECTDATA(node1,node)->voltageA = complex(7200,0);
		OBJECTDATA(node2,load)->voltageA = complex(118.01,-0.6131);
		OBJECTDATA(node2,load)->currentA = complex(160.62,-80.451);
		//////////
		OBJECTDATA(node1,node)->voltageB = complex(-3600,-6235.4);
		OBJECTDATA(node2,load)->voltageB = complex(117.85,-0.61424);
		OBJECTDATA(node2,load)->currentB = complex(-194.97,102.72);
		//OBJECTDATA(node2,load)->currentB.SetImag(-431.81);

		OBJECTDATA(node1,node)->voltageC = complex(-3600,6235.4);
		OBJECTDATA(node2,load)->voltageC = complex(0,0);
		OBJECTDATA(node2,load)->currentC = complex(-34.355,22.272);
		//OBJECTDATA(node2,load)->currentC.SetImag(1004);

		// Set up the transformer variables
		cl = get_class_by_name("transformer_configuration");
		OBJECT *trans_config = gl_create_object(cl);
		OBJECTDATA(trans_config,transformer_configuration)->V_primary = 7.2 * 1000; 
		OBJECTDATA(trans_config,transformer_configuration)->V_secondary = 0.120 * 1000; 
 		OBJECTDATA(trans_config,transformer_configuration)->kVA_rating = 50;
		OBJECTDATA(trans_config,transformer_configuration)->phaseA_kVA_rating = 50;
		//OBJECTDATA(trans_config,transformer_configuration)->R_pu = .011;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetReal(0.011);
		//OBJECTDATA(trans_config,transformer_configuration)->X_pu = .018;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetImag(0.018);
		OBJECTDATA(trans_config,transformer_configuration)->connect_type = transformer_configuration::SINGLE_PHASE_CENTER_TAPPED;//delta-delta
		OBJECTDATA(tformer,transformer)->configuration = trans_config;

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		//CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		OBJECTDATA(node1,node)->voltageA = complex(7200,0);
		OBJECTDATA(node1,node)->voltageB = complex(-3600,-6235.4);
		OBJECTDATA(node1,node)->voltageC = complex(-3600,6235.4);
		OBJECTDATA(node2,load)->currentA = complex(160.62,-80.451);
		OBJECTDATA(node2,load)->currentB = complex(-194.97,102.72);
		OBJECTDATA(node2,load)->currentC = complex(-34.355,22.272);
	
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

		/*rV_2 = OBJECTDATA(node2,load)->voltageC.Re();
		iV_2 = OBJECTDATA(node2,load)->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test->Re() ? rV_2 / node2_phC_V_test->Re() : node2_phC_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test->Im() ? iV_2 / node2_phC_V_test->Im() : node2_phC_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);*/

	}
		/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(transformer_tests);

	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	//CPPUNIT_TEST();
	CPPUNIT_TEST(test_wye_wye_transformer);
	CPPUNIT_TEST(test_delta_delta_transformer);
	CPPUNIT_TEST(test_delta_Gwye_transformer);
	CPPUNIT_TEST(test_center_tapped_transformer_Aphase);
	CPPUNIT_TEST_SUITE_END();
};

#endif

#endif
