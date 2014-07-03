// $id$
//	Copyright (C) 2008 Battelle Memorial Institute
#ifndef METER_TEST_H
#define METER_TEST_H

#include "line.h"
#include "node.h"
#include "meter.h"
#include "triplex_meter.h"

#ifndef _NO_CPPUNIT

class meter_tests : public test_helper
{
public:
	void setUp(){
	}

	void tearDown(){
		local_callbacks->remove_objects();
	}
	
	void test_meter_singlephase(){
		OBJECT *node1,*node2,*node3; // Node objects
		OBJECT *ohl1,*ohl2; // line object
		// Test values
		
		complex* node1_phA_I_test = new complex(6.0638,-3.0419);
		complex* node3_phA_V_test = new complex(115.27,-0.29);
		complex* node3_phB_V_test = new complex(122.37,-2.5);
		complex* node3_phC_V_test = new complex(0.0,0.0);
//        complex* meter_power_test = new complex();
        double meter_power_test = 43076.9024312;

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();
		
		cl = get_class_by_name("triplex_node");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,triplex_node)->create();

        cl = get_class_by_name("triplex_meter");
		node3 = gl_create_object(cl);
		OBJECTDATA(node3,meter)->create();

		cl = get_class_by_name("transformer");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,transformer)->create();

		cl = get_class_by_name("triplex_line");
		ohl2 = gl_create_object(cl);
		OBJECTDATA(ohl2,triplex_line)->create();

		OBJECTDATA(ohl1,transformer)->from = node1;
		OBJECTDATA(ohl1,transformer)->to = node2;

		OBJECTDATA(ohl1,transformer)->phases = PHASE_A|PHASE_S;

		OBJECTDATA(node1,node)->voltageA = complex(7200,0);
		OBJECTDATA(node1,node)->voltageB = complex(-3600,-6235.4);
		OBJECTDATA(node1,node)->voltageC = complex(-3600,6235.4);

		OBJECTDATA(node2,triplex_node)->voltageA = complex(120.0,0.0);
		OBJECTDATA(node2,triplex_node)->voltageB = complex(120.0,0.0);
		OBJECTDATA(node2,triplex_node)->voltageC = complex(0.0,0.0);

		OBJECTDATA(node3,triplex_meter)->voltageA = complex(120.0,0.0);
		OBJECTDATA(node3,triplex_meter)->voltageB = complex(120.0,0.0);
		OBJECTDATA(node3,triplex_meter)->voltageC = complex(0.0,0.0);
		// Set up the transformer variables
		cl = get_class_by_name("transformer_configuration");
		OBJECT *trans_config = gl_create_object(cl);
		OBJECTDATA(trans_config,transformer_configuration)->V_primary = 7.2 * 1000; 
		OBJECTDATA(trans_config,transformer_configuration)->V_secondary = 0.120 * 1000; 
 		OBJECTDATA(trans_config,transformer_configuration)->kVA_rating = 50;
		OBJECTDATA(trans_config,transformer_configuration)->phaseA_kVA_rating = 50;
		//OBJECTDATA(trans_config,transformer_configuration)->R_pu = .011;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetReal(0.011);
		//OBJECTDATA(trans_config,transformer_configuration)->X_pu = 0.018;
		OBJECTDATA(trans_config,transformer_configuration)->impedance.SetImag(0.018);
		OBJECTDATA(trans_config,transformer_configuration)->connect_type = transformer_configuration::SINGLE_PHASE_CENTER_TAPPED;//delta-delta
		OBJECTDATA(ohl1,transformer)->configuration = trans_config;

///////////////////////////////////////////////////////////////////////////////

		OBJECTDATA(ohl2,triplex_line)->from = node2;
		OBJECTDATA(ohl2,triplex_line)->to = node3;
		OBJECTDATA(ohl2,triplex_line)->length = 100;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl2,triplex_line)->phases = OBJECTDATA(node2,node)->phases = OBJECTDATA(node3,load)->phases = PHASE_A;
		
		// Phase A line conductor
		cl = get_class_by_name("triplex_line_conductor");
		OBJECT *ulc_a = gl_create_object(cl);
		OBJECTDATA(ulc_a,triplex_line_conductor)->geometric_mean_radius = 0.0111;
		OBJECTDATA(ulc_a,triplex_line_conductor)->resistance = 0.97;

		cl = get_class_by_name("triplex_line_configuration");
		OBJECT *line_config = gl_create_object(cl);
		triplex_line_configuration *lc = OBJECTDATA(line_config,triplex_line_configuration);
		lc->ins_thickness=0.08;
		lc->diameter=0.368;
		lc->phaseA_conductor = ulc_a;
		lc->phaseB_conductor = ulc_a;
		lc->phaseC_conductor = ulc_a;
		lc->phaseN_conductor = 0;
		
		OBJECTDATA(ohl2,triplex_line)->configuration = line_config;
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		OBJECTDATA(node3,node)->currentA = complex(164.25,-80.09);
		OBJECTDATA(node3,node)->currentB = complex(-199.58,102.43);
		OBJECTDATA(node3,node)->currentC = complex(35.32,-22.34);

		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1
#ifdef _TESTING
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentA.Re(), OBJECTDATA(node1,node)->currentA.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentB.Re(), OBJECTDATA(node1,node)->currentB.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentC.Re(), OBJECTDATA(node1,node)->currentC.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->currentA.Re(), OBJECTDATA(node2,triplex_node)->currentA.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->currentB.Re(), OBJECTDATA(node2,triplex_node)->currentB.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->currentC.Re(), OBJECTDATA(node2,triplex_node)->currentC.Im());
#endif        

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		
	    OBJECTDATA(node1,node)->voltageA = complex(7200,0);
		OBJECTDATA(node1,node)->voltageB = complex(-3600,-6235.4);
		OBJECTDATA(node1,node)->voltageC = complex(-3600,6235.4);
		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values
		double rV_3 = OBJECTDATA(node2,triplex_node)->voltageA.Re();
		double iV_3 = OBJECTDATA(node2,triplex_node)->voltageA.Im();
#ifdef _TESTING
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->voltageA.Re(), OBJECTDATA(node2,triplex_node)->voltageA.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->voltageB.Re(), OBJECTDATA(node2,triplex_node)->voltageB.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node2,triplex_node)->voltageC.Re(), OBJECTDATA(node2,triplex_node)->voltageC.Im());
#endif

		double rV_2 = OBJECTDATA(node3,meter)->voltageA.Re();
		double iV_2 = OBJECTDATA(node3,meter)->voltageA.Im();
#ifdef _TESTING
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node3,meter)->voltageA.Re(), OBJECTDATA(node3,meter)->voltageA.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node3,meter)->voltageB.Re(), OBJECTDATA(node3,meter)->voltageB.Im());
     	printf("  %10.6f+%+10.6f\n", OBJECTDATA(node3,meter)->voltageC.Re(), OBJECTDATA(node3,meter)->voltageC.Im());
#endif

		validate_re = 1 - (rV_2 <= node3_phA_V_test->Re() ? rV_2 / node3_phA_V_test->Re() : node3_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node3_phA_V_test->Im() ? iV_2 / node3_phA_V_test->Im() : node3_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

//		local_callbacks->sync_all(PC_PRETOPDOWN);

		double mP_3 = OBJECTDATA(node3,meter)->measured_power.Mag()*1000;
		validate_re = 1 - (mP_3 <= meter_power_test ? mP_3 / meter_power_test : meter_power_test /mP_3);
				
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);

	}

	void test_meter_polyphase(){
		OBJECT *node1,*node2,*node3; // Node objects
		OBJECT *ohl1,*ohl2; // line object
		// Test values
		
		complex* node1_phA_I_test = new complex(204.5,-18.431);
		complex* node1_phB_I_test = new complex(39.716,-57.144);
		complex* node1_phC_I_test = new complex(-46.035,115.21);	

		complex* node3_phA_V_test = new complex(2334.5449,-237.6837);
		complex* node3_phB_V_test = new complex(-1372.186896,-2138.1349);
		complex* node3_phC_V_test = new complex(-1027.44395,2102.13);


//        complex* meter_power_test = new complex();
        double meter_power_test1 = 855101.786;

        double meter_power_test2 = 855418.845;

		CLASS *cl = get_class_by_name("node");
		node1 = gl_create_object(cl);
		OBJECTDATA(node1,node)->create();

        cl = get_class_by_name("meter");
		node2 = gl_create_object(cl);
		OBJECTDATA(node2,meter)->create();
		OBJECTDATA(node2,meter)->nominal_voltage = 4200;//??????????????
/*
        cl = get_class_by_name("node");
		node2 = gl_create_object(cl,sizeof(node));
		OBJECTDATA(node2,node)->create();
*/
		cl = get_class_by_name("load");
		node3 = gl_create_object(cl);
		OBJECTDATA(node3,load)->create();

		cl = get_class_by_name("underground_line");
		ohl1 = gl_create_object(cl);
		OBJECTDATA(ohl1,underground_line)->create();

		OBJECTDATA(ohl1,underground_line)->from = node1;
		OBJECTDATA(ohl1,underground_line)->to = node2;
		OBJECTDATA(ohl1,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl1,underground_line)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node1,node)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node2,meter)->phases = PHASE_A|PHASE_B|PHASE_C;

	    cl = get_class_by_name("underground_line");
		ohl2 = gl_create_object(cl);
		OBJECTDATA(ohl2,underground_line)->create();

		OBJECTDATA(ohl2,underground_line)->from = node2;
		OBJECTDATA(ohl2,underground_line)->to = node3;
		OBJECTDATA(ohl2,underground_line)->length = 500;

		// Next we set up the parameters needed for the test
		OBJECTDATA(ohl2,underground_line)->phases = PHASE_A|PHASE_B|PHASE_C;
//		OBJECTDATA(node2,meter)->phases = PHASE_A|PHASE_B|PHASE_C;
		OBJECTDATA(node3,load)->phases = PHASE_A|PHASE_B|PHASE_C;
//		OBJECTDATA(node2,node)->phases = PHASE_A|PHASE_B|PHASE_C;

		
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
		OBJECTDATA(ohl2,underground_line)->configuration = line_config;

		OBJECTDATA(node1,node)->voltageA = complex(2349.8,-228.75);
		OBJECTDATA(node1,node)->voltageB = complex(-1362,-2136.6);
		OBJECTDATA(node1,node)->voltageC = complex(-1028.7,2106.4);
	
		OBJECTDATA(node2,meter)->voltageA = complex(2349.8,-228.75);
		OBJECTDATA(node2,meter)->voltageB = complex(-1362,-2136.6);
		OBJECTDATA(node2,meter)->voltageC = complex(-1028.7,2106.4);
/*
		OBJECTDATA(node2,node)->voltageA = complex(2349.8,-228.75);
		OBJECTDATA(node2,node)->voltageB = complex(-1362,-2136.6);
		OBJECTDATA(node2,node)->voltageC = complex(-1028.7,2106.4);
*/
		OBJECTDATA(node3,load)->voltageA = complex(2349.8,-228.75);
		OBJECTDATA(node3,load)->voltageB = complex(-1362,-2136.6);
		OBJECTDATA(node3,load)->voltageC = complex(-1028.7,2106.4);


///////////////////////////////////////////////////////////////////////////////
		
		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		OBJECTDATA(node3,load)->currentA = complex(204.5,-18.431);
		OBJECTDATA(node3,load)->currentB = complex(39.716,-57.144);
		OBJECTDATA(node3,load)->currentC = complex(-46.035,115.21);
		local_callbacks->sync_all(PC_BOTTOMUP);

		// Check the bottom-up pass values
		double rI_1 = OBJECTDATA(node1,node)->currentA.Re(); // Real part of the current at node 1
		double iI_1 = OBJECTDATA(node1,node)->currentA.Im(); // Imaginary part of the current at node 1
		printf(" \nI1: %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentA.Re(), OBJECTDATA(node1,node)->currentA.Im());
     	printf(" I1: %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentB.Re(), OBJECTDATA(node1,node)->currentB.Im());
     	printf(" I1: %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->currentC.Re(), OBJECTDATA(node1,node)->currentC.Im());

		printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->currentA.Re(), OBJECTDATA(node2,meter)->currentA.Im());
     	printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->currentB.Re(), OBJECTDATA(node2,meter)->currentB.Im());
     	printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->currentC.Re(), OBJECTDATA(node2,meter)->currentC.Im());
/*
		printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->currentA.Re(), OBJECTDATA(node2,node)->currentA.Im());
     	printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->currentB.Re(), OBJECTDATA(node2,node)->currentB.Im());
     	printf(" I2: %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->currentC.Re(), OBJECTDATA(node2,node)->currentC.Im());
*/

		printf(" I3: %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->currentA.Re(), OBJECTDATA(node3,load)->currentA.Im());
     	printf(" I3: %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->currentB.Re(), OBJECTDATA(node3,load)->currentB.Im());
     	printf(" I3: %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->currentC.Re(), OBJECTDATA(node3,load)->currentC.Im());

//		local_callbacks->sync_all(PC_PRETOPDOWN);
		printf(" Power: %10.6f\n", OBJECTDATA(node2,meter)->power);

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test->Re() ? rI_1 / node1_phA_I_test->Re() : node1_phA_I_test->Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test->Im() ? iI_1 / node1_phA_I_test->Im() : node1_phA_I_test->Im() /iI_1);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

//		local_callbacks->sync_all(PC_PRETOPDOWN);
		double mP_1=OBJECTDATA(node2,meter)->measured_power.Mag()*1000;
		validate_re = 1 - (mP_1 <= meter_power_test1 ? mP_1 / meter_power_test1 : meter_power_test1 /mP_1);
//		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);

		
	    OBJECTDATA(node1,node)->voltageA = complex(2367.6,-220.050);
		OBJECTDATA(node1,node)->voltageB = complex(-1352.9,-2136.8);
		OBJECTDATA(node1,node)->voltageC = complex(-1030.4,2110.9);	

		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values
		double rV_3 = OBJECTDATA(node3,load)->voltageA.Re();
		double iV_3 = OBJECTDATA(node3,load)->voltageA.Im();
		printf(" V1: %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->voltageA.Re(), OBJECTDATA(node1,node)->voltageA.Im());
     	printf(" V1:  %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->voltageB.Re(), OBJECTDATA(node1,node)->voltageB.Im());
     	printf(" V1:  %10.6f+%+10.6f\n", OBJECTDATA(node1,node)->voltageC.Re(), OBJECTDATA(node1,node)->voltageC.Im());

		printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->voltageA.Re(), OBJECTDATA(node2,meter)->voltageA.Im());
     	printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->voltageB.Re(), OBJECTDATA(node2,meter)->voltageB.Im());
     	printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,meter)->voltageC.Re(), OBJECTDATA(node2,meter)->voltageC.Im());
/*
        printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->voltageA.Re(), OBJECTDATA(node2,node)->voltageA.Im());
     	printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->voltageB.Re(), OBJECTDATA(node2,node)->voltageB.Im());
     	printf(" V2:  %10.6f+%+10.6f\n", OBJECTDATA(node2,node)->voltageC.Re(), OBJECTDATA(node2,node)->voltageC.Im());
*/
		printf(" V3:  %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->voltageA.Re(), OBJECTDATA(node3,load)->voltageA.Im());
     	printf(" V3:  %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->voltageB.Re(), OBJECTDATA(node3,load)->voltageB.Im());
     	printf(" V3: %10.6f+%+10.6f\n", OBJECTDATA(node3,load)->voltageC.Re(), OBJECTDATA(node3,load)->voltageC.Im());

//		local_callbacks->sync_all(PC_PRETOPDOWN);
		printf(" Power: %10.6f\n", OBJECTDATA(node2,meter)->power);

		double rV_2 = OBJECTDATA(node3,load)->voltageA.Re();
		double iV_2 = OBJECTDATA(node3,load)->voltageA.Im();
		validate_re = 1 - (rV_2 <= node3_phA_V_test->Re() ? rV_2 / node3_phA_V_test->Re() : node3_phA_V_test->Re() /rV_2);
		validate_im = 1 - (iV_2 <= node3_phA_V_test->Im() ? iV_2 / node3_phA_V_test->Im() : node3_phA_V_test->Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

//		local_callbacks->sync_all(PC_PRETOPDOWN);

		mP_1=OBJECTDATA(node2,meter)->measured_power.Mag()*1000;
		validate_re = 1 - (mP_1 <= meter_power_test2 ? mP_1 / meter_power_test2 : meter_power_test2 /mP_1);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);

	}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(meter_tests);
	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */

	
	CPPUNIT_TEST(test_meter_singlephase);
    CPPUNIT_TEST(test_meter_polyphase);

	
	CPPUNIT_TEST_SUITE_END();
};

#endif
#endif
