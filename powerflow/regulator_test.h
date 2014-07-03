// $id$
// 	Copyright (C) 2008 Battelle Memorial Institute
#ifndef _REGULATOR_TEST_H
#define _REGULATOR_TEST_H

#include <stdlib.h>
#include "node.h"
#include "regulator.h"

#ifndef _NO_CPPUNIT

#ifdef VALIDATE_THRESHOLD
#undef VALIDATE_THRESHOLD
#endif
#define VALIDATE_THRESHOLD .0004


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

class regulator_tests : public test_helper 
{	

	void test_three_single_phase_wye_regulator() {

		CLASS *cl = get_class_by_name("node");
		OBJECT *obj = gl_create_object(cl);
		node *src_node = OBJECTDATA(obj,node);
		src_node->create();

		cl = get_class_by_name("load");
		obj = gl_create_object(cl);
		node *load_node = OBJECTDATA(obj,load);
		load_node->create();

		cl = get_class_by_name("regulator");
		obj = gl_create_object(cl);
		regulator *reg = OBJECTDATA(obj,regulator);
		reg->create();

		cl = get_class_by_name("regulator_configuration");
		obj = gl_create_object(cl);
		regulator_configuration *reg_config = OBJECTDATA(obj,regulator_configuration);
		reg_config->create();

		// Calculated results
		complex node1_phA_I_test(242.49, -88.26);
		complex node1_phB_I_test(-214.54, -156.86);
		complex node1_phC_I_test(21.47, 307.05);
		
		complex node2_phA_V_test(7384.6, 0.0);
		complex node2_phB_V_test(-3716.1, -6436.5);
		complex node2_phC_V_test(-3814.6, 6607.0);

		// setup test objects
		src_node->phases = PHASE_ABCN;
		src_node->voltageA = complex(7200.0, 0.0);
		src_node->voltageB = complex(-3600.0, -6235.4);
		src_node->voltageC = complex(-3600.0, 6235.4);

		load_node->phases = PHASE_ABCN;
		load_node->voltageA = complex(7384.6, 0.0);
		load_node->voltageB = complex(-3716.1, -6436.6);
		load_node->voltageC = complex(-3789.0, 6562.7);
		load_node->currentA = complex(236.43, -86.05);
		load_node->currentB = complex(-233.99, -151.95);
		load_node->currentC = complex(21.47, 307.05);

		reg_config->connect_type = regulator_configuration::WYE_WYE;
		reg_config->band_center = 120.0;
		reg_config->band_width = 2.0;
		reg_config->time_delay = 30.0;
		reg_config->raise_taps = 16;
		reg_config->lower_taps = 16;
		reg_config->CT_ratio = 600;
		reg_config->PT_ratio = 60;
		reg_config->ldc_R_V[0] = 6.0;
		reg_config->ldc_R_V[1] = 6.0;
		reg_config->ldc_R_V[2] = 6.0;
		reg_config->ldc_X_V[0] = 12.0;
		reg_config->ldc_X_V[1] = 12.0;
		reg_config->ldc_X_V[2] = 12.0;
		reg_config->CT_phase = PHASE_ABC;
		reg_config->PT_phase = PHASE_ABC;
		reg_config->regulation = 0.10;
		reg_config->tap_pos[0] = 4;
		reg_config->tap_pos[1] = 5;
		reg_config->tap_pos[2] = 9;

		reg->phases = PHASE_ABCN;
		reg->configuration = OBJECTHDR(reg_config);
		reg->from = OBJECTHDR(src_node);
		reg->to = OBJECTHDR(load_node);

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = src_node->currentA.Re(); // Real part of the current at node 1
		double iI_1 = src_node->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test.Re() ? rI_1 / node1_phA_I_test.Re() : node1_phA_I_test.Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test.Im() ? iI_1 / node1_phA_I_test.Im() : node1_phA_I_test.Im() /iI_1);

		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		src_node->voltageA = complex(7200.0, 0.0);
		src_node->voltageB = complex(-3600.0, -6235.4);
		src_node->voltageC = complex(-3600.0, 6235.4);

		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = load_node->voltageA.Re();
		double iV_2 = load_node->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test.Re() ? (node2_phA_V_test.Re() != 0 ? rV_2 / node2_phA_V_test.Re():rV_2) : (rV_2 != 0 ? node2_phA_V_test.Re() /rV_2 : node2_phA_V_test.Re()));
		validate_im = 1 - (iV_2 <= node2_phA_V_test.Im() ? (node2_phA_V_test.Im() != 0 ? iV_2 / node2_phA_V_test.Im():iV_2) : (iV_2 != 0 ? node2_phA_V_test.Im() /iV_2 : node2_phA_V_test.Im()));
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
//		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = load_node->voltageB.Re();
		iV_2 = load_node->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test.Re() ? rV_2 / node2_phB_V_test.Re() : node2_phB_V_test.Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test.Im() ? iV_2 / node2_phB_V_test.Im() : node2_phB_V_test.Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = load_node->voltageC.Re();
		iV_2 = load_node->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test.Re() ? rV_2 / node2_phC_V_test.Re() : node2_phC_V_test.Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test.Im() ? iV_2 / node2_phC_V_test.Im() : node2_phC_V_test.Im() /iV_2);
		CPPUNIT_ASSERT(validate_re < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);
	}

	void test_two_single_phase_open_delta_ABBC_regulator() {
        OBJECT *src_obj, *load_obj; // Node objects
		OBJECT *reg_obj, *config_obj; // Regulator objects
		node *src_node;
		load *load_node;
		regulator *reg;
		regulator_configuration *reg_config;

		// Calculated results
		complex node1_phA_I_test(163.32, -261.37);
		complex node1_phB_I_test(-263.44, -18.249);
		complex node1_phC_I_test(100.12, 279.62);
		
		complex node2_phA_V_test(6505.6, -3692.1);
		complex node2_phB_V_test(-6450.2, -3692.1);
		complex node2_phC_V_test(-55.367,  7384.2);

		// setup test objects
		CLASS *_class = get_class_by_name("node");
		src_obj = gl_create_object(_class);
		src_node = OBJECTDATA(src_obj, node);
		src_node->create();

		src_node->phases = PHASE_ABC;
		src_node->voltageA = complex(6505.6, -3692.1);
		src_node->voltageB = complex(-6450.2, -3692.1);
		src_node->voltageC = complex(-55.367, 7384.2);

		_class = get_class_by_name("load");
		load_obj = gl_create_object(_class);
		load_node = OBJECTDATA(load_obj, load);
		load_node->create();

		load_node->phases = PHASE_ABC;
		load_node->voltageA = complex(6505.6, -3692.1);
		load_node->voltageB = complex(-6450.2, -3692.1);
		load_node->voltageC = complex(-55.367, 7384.2);
		load_node->currentA = complex(157.17, -251.53);
		load_node->currentB = complex(-254.81, -21.059);
		load_node->currentC = complex(97.614, 272.63);

		_class = get_class_by_name("regulator_configuration");
		config_obj = gl_create_object(_class);
		reg_config = OBJECTDATA(config_obj, regulator_configuration);
		reg_config->create();

		reg_config->connect_type = regulator_configuration::OPEN_DELTA_ABBC;
		reg_config->band_center = 120.0;
		reg_config->band_width = 2.0;
		reg_config->time_delay = 30.0;
		reg_config->raise_taps = 16;
		reg_config->lower_taps = 16;
		reg_config->CT_ratio = 500;
		reg_config->PT_ratio = (int)103.92;
		reg_config->ldc_R_V[0] = 0.8;
		reg_config->ldc_R_V[1] = 0.8;
		reg_config->ldc_R_V[2] = 0.8;
		reg_config->ldc_X_V[0] = 9.9;
		reg_config->ldc_X_V[1] = 9.9;
		reg_config->ldc_X_V[2] = 9.9;
		reg_config->CT_phase = PHASE_ABC;
		reg_config->PT_phase = PHASE_ABC;
		reg_config->regulation = 0.10;
		reg_config->tap_pos[0] = 6;
		reg_config->tap_pos[1] = 4;
		reg_config->tap_pos[2] = 0;

		_class = get_class_by_name("regulator");
		reg_obj = gl_create_object(_class);
		reg = OBJECTDATA(reg_obj, regulator);
		reg->create();

		reg->phases = PHASE_ABC;
		reg->configuration = config_obj;
		reg->from = src_obj;
		reg->to = load_obj;

		// Now we call init on the objects
		CPPUNIT_ASSERT(local_callbacks->init_objects() != FAILED);

		// Rank?
		CPPUNIT_ASSERT(local_callbacks->setup_test_ranks() != FAILED);

		// Run the test
		local_callbacks->sync_all(PC_PRETOPDOWN);
		local_callbacks->sync_all(PC_BOTTOMUP);

		double rI_1 = src_node->currentA.Re(); // Real part of the current at node 1
		double iI_1 = src_node->currentA.Im(); // Imaginary part of the current at node 1

		// Get the percentage of the difference between the calculated value and the test value
		double validate_re = 1 - (rI_1 <= node1_phA_I_test.Re() ? rI_1 / node1_phA_I_test.Re() : node1_phA_I_test.Re() /rI_1);
		double validate_im = 1 - (iI_1 <= node1_phA_I_test.Im() ? iI_1 / node1_phA_I_test.Im() : node1_phA_I_test.Im() /iI_1);

		CPPUNIT_ASSERT(fabs(validate_re) < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(fabs(validate_im) < VALIDATE_THRESHOLD);

		src_node->voltageA = complex(6235,  -3599.8);
		src_node->voltageB = complex(-6235,   -3599.8);
		src_node->voltageC = complex(0,      7199.6);

		local_callbacks->sync_all(PC_POSTTOPDOWN);
		// check the top down values

		double rV_2 = load_node->voltageA.Re();
		double iV_2 = load_node->voltageA.Im();

		validate_re = 1 - (rV_2 <= node2_phA_V_test.Re() ? (node2_phA_V_test.Re() != 0 ? rV_2 / node2_phA_V_test.Re():rV_2) : (rV_2 != 0 ? node2_phA_V_test.Re() /rV_2 : node2_phA_V_test.Re()));
		validate_im = 1 - (iV_2 <= node2_phA_V_test.Im() ? (node2_phA_V_test.Im() != 0 ? iV_2 / node2_phA_V_test.Im():iV_2) : (iV_2 != 0 ? node2_phA_V_test.Im() /iV_2 : node2_phA_V_test.Im()));
		CPPUNIT_ASSERT(fabs(validate_re) < VALIDATE_THRESHOLD);
//		CPPUNIT_ASSERT(validate_im < VALIDATE_THRESHOLD);

		rV_2 = load_node->voltageB.Re();
		iV_2 = load_node->voltageB.Im();

		validate_re = 1 - (rV_2 <= node2_phB_V_test.Re() ? rV_2 / node2_phB_V_test.Re() : node2_phB_V_test.Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phB_V_test.Im() ? iV_2 / node2_phB_V_test.Im() : node2_phB_V_test.Im() /iV_2);
		CPPUNIT_ASSERT(fabs(validate_re) < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(fabs(validate_im) < VALIDATE_THRESHOLD);

		rV_2 = load_node->voltageC.Re();
		iV_2 = load_node->voltageC.Im();

		validate_re = 1 - (rV_2 <= node2_phC_V_test.Re() ? rV_2 / node2_phC_V_test.Re() : node2_phC_V_test.Re() /rV_2);
		validate_im = 1 - (iV_2 <= node2_phC_V_test.Im() ? iV_2 / node2_phC_V_test.Im() : node2_phC_V_test.Im() /iV_2);
		CPPUNIT_ASSERT(fabs(validate_re) < VALIDATE_THRESHOLD);
		CPPUNIT_ASSERT(fabs(validate_im) < VALIDATE_THRESHOLD);
    }

	void test_two_single_phase_open_delta_ABAC_regulator() {
        // No Unit Test values given
    }

	void test_two_single_phase_open_delta_ACBC_regulator() {
        // No Unit Test values given
    }
      
	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(regulator_tests);

	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	//CPPUNIT_TEST();
	CPPUNIT_TEST(test_three_single_phase_wye_regulator);
	CPPUNIT_TEST(test_two_single_phase_open_delta_ABBC_regulator);
	CPPUNIT_TEST_SUITE_END();
};

#endif
#endif  /* _REGULATOR_TEST_H */

