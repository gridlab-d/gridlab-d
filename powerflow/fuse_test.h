/** $Id: fuse_test.h 4738 2014-07-03 00:55:39Z dchassin $
	@file fusetest.h
	@author Chunlian Jin
*/

#ifndef _FUSE_TEST_H
#define _FUSE_TEST_H

#include <stdlib.h>
#include "fuse.h"

#ifndef _NO_CPPUNIT

#ifdef VALIDATE_TOLERANCE
#undef VALIDATE_TOLERANCE
#endif
#define VALIDATE_TOLERANCE 0.06

class fuse_tests : public powerflow_test_helper
{
	void test_three_phase_ganged_switch(bool status, set phases)
	{
		fuse *test_fuse = create_object<fuse>("fuse");
		test_fuse->status = status;
		test_fuse->phases = phases;

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

		const complex fs_V1_in[3] = {complex(2442.7, -109.3),
		                             complex(-1315.2, -2123.6),
		                             complex(-1137.7, 2156.1)};
		const complex fs_I2_in[3] = {complex(64.32, -49.78),
		                             complex(-57.1, -21.81),
		                             complex(10.38, 61.85)};
		const complex fs_V2_out[3] = {
				status && (phases & PHASE_A) ? bs_V2_in[0] : 0.0,
				status && (phases & PHASE_B) ? bs_V2_in[1] : 0.0,
				status && (phases & PHASE_C) ? bs_V2_in[2] : 0.0};

		test_powerflow_link(test_fuse, VALIDATE_TOLERANCE, PHASE_ABC, PHASE_ABC,
				bs_V2_in, bs_I2_in, bs_I1_out, fs_V1_in, fs_I2_in, fs_V2_out);
	}

	void test_three_phase_ganged_switch_ABC_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_ABC);
	}

	//void test_three_phase_ganged_switch_ABC_open() {
	//	test_three_phase_ganged_switch(link_object::LS_OPEN, PHASE_ABC);
	//}

	void test_three_phase_ganged_switch_A_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_A);
	}

	//void test_three_phase_ganged_switch_A_open() {
	//	test_three_phase_ganged_switch(link_object::LS_OPEN, PHASE_A);
	//}

	void test_three_phase_ganged_switch_B_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_B);
	}

	//void test_three_phase_ganged_switch_B_open() {
	//	test_three_phase_ganged_switch(link_object::LS_OPEN, PHASE_B);
	//}

	void test_three_phase_ganged_switch_C_closed() {
		test_three_phase_ganged_switch(LS_CLOSED, PHASE_C);
	}

	//void test_three_phase_ganged_switch_C_open() {
	//	test_three_phase_ganged_switch(link_object::LS_OPEN, PHASE_C);
	//}

	/*
	 * This section creates the suite() method that will be used by the
	 * CPPUnit testrunner to execute the tests that we have registered.
	 * This section needs to be in the .h file
	 */
	CPPUNIT_TEST_SUITE(fuse_tests);

	/*
	 * For each test method defined above, we should have a separate
	 * CPPUNIT_TEST() line.
	 */
	//CPPUNIT_TEST();
	CPPUNIT_TEST(test_three_phase_ganged_switch_ABC_closed);
	//CPPUNIT_TEST(test_three_phase_ganged_switch_ABC_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_A_closed);
	//CPPUNIT_TEST(test_three_phase_ganged_switch_A_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_B_closed);
	//CPPUNIT_TEST(test_three_phase_ganged_switch_B_open);
	CPPUNIT_TEST(test_three_phase_ganged_switch_C_closed);
	//CPPUNIT_TEST(test_three_phase_ganged_switch_C_open);
	CPPUNIT_TEST_SUITE_END();
};

#endif
#endif  /* _RELAY_TEST_H */
