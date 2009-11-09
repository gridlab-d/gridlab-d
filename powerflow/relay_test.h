// $Id: relay_test.h 1182 2008-12-22 22:08:36Z dchassin $
// 	Copyright (C) 2008 Battelle Memorial Institute

#ifndef _RELAY_TEST_H
#define _RELAY_TEST_H

#include <stdlib.h>
#include "relay.h"

#ifndef _NO_CPPUNIT

#ifdef VALIDATE_TOLERANCE
#undef VALIDATE_TOLERANCE
#endif
#define VALIDATE_TOLERANCE 0.04

class relay_tests : public powerflow_test_helper
{
	void test_three_phase_ganged_switch(bool status, set phases)
	{
		relay *test_relay = create_object<relay>("relay");
		test_relay->status = status;
		test_relay->reclose_time = 5;
		test_relay->phases = phases;

		const complex bs_V2_in[3] = {complex(2442.6, -109.21),
		                             complex(-1315.1, -2123.6),
		                             complex(-1137.7, 2156)};
		const complex bs_I2_in[3] = {complex(64.32, -49.78),
		                             complex(-57.1, -21.81),
		                             complex(10.38, 61.85)};
		const complex bs_I1_out[3] = {
				status && (phases & PHASE_A) ? bs_I2_in[0] : 0.0,
				status && (phases & PHASE_B) ? bs_I2_in[1] : 0.0,
				status && (phases & PHASE_C) ? bs_I2_in[2] : 0.0};

		const complex fs_V1_in[3] = {complex(2442.6, -109.21),
		                             complex(-1315.1, -2123.6),
		                             complex(-1137.7, 2156)};
		const complex fs_I2_in[3] = {complex(64.32, -49.78),
		                             complex(-57.1, -21.81),
		                             complex(10.38, 61.85)};
		const complex fs_V2_out[3] = {
				status && (phases & PHASE_A) ? bs_V2_in[0] : 0.0,
				status && (phases & PHASE_B) ? bs_V2_in[1] : 0.0,
				status && (phases & PHASE_C) ? bs_V2_in[2] : 0.0};

		test_powerflow_link(test_relay, VALIDATE_TOLERANCE, PHASE_ABC, PHASE_ABC,
				bs_V2_in, bs_I2_in, bs_I1_out, fs_V1_in, fs_I2_in, fs_V2_out);
	}

	void test_three_phase_ganged_switch_ABC_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_ABC);
	}

	void test_three_phase_ganged_switch_ABC_open() {
		test_three_phase_ganged_switch(LS_OPEN, PHASE_ABC);
	}

	void test_three_phase_ganged_switch_A_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_A);
	}

	void test_three_phase_ganged_switch_A_open() {
		test_three_phase_ganged_switch(LS_OPEN, PHASE_A);
	}

	void test_three_phase_ganged_switch_B_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_B);
	}

	void test_three_phase_ganged_switch_B_open() {
		test_three_phase_ganged_switch(LS_OPEN, PHASE_B);
	}

	void test_three_phase_ganged_switch_C_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_C);
	}

	void test_three_phase_ganged_switch_C_open() {
		test_three_phase_ganged_switch(LS_OPEN, PHASE_C);
	}
    
	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(relay_tests);

	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	//CPPUNIT_TEST();
	CPPUNIT_TEST(test_three_phase_ganged_switch_ABC_closed);
	CPPUNIT_TEST(test_three_phase_ganged_switch_ABC_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_A_closed);
	CPPUNIT_TEST(test_three_phase_ganged_switch_A_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_B_closed);
	CPPUNIT_TEST(test_three_phase_ganged_switch_B_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_C_closed);
	CPPUNIT_TEST(test_three_phase_ganged_switch_C_open);
	CPPUNIT_TEST_SUITE_END();
};

#endif
#endif  /* _RELAY_TEST_H */
