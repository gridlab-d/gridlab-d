// $Id: test.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"
#include "plc.h"

#ifndef _NO_CPPUNIT
#include "plc_test.h"




/**
 * Function called from the core to perform the unit tests for this module.  It
 * takes 3 parameters and returns an integer value that indicates whether the
 * tests were sucessful or not.
 *
 * @param callbacks struct of functions pointing back to accessor functions in the core.
 * @param argc a count of the number of arguments that were passed in on the command line.
 * @param argv pointer to an array of characters which contains the arguments passed in on the command line
 * @return either 0 or 1 depending on the outcome of the tests.  1 is returned upon successful completion, and 0 indicates a test failure.
 */
EXPORT int module_test(TEST_CALLBACKS *callbacks,int argc, char* argv[]){
	local_callbacks = callbacks;
	CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

	// Adds the test to the list of test to run
	CppUnit::TextUi::TestRunner runner;
	runner.addTest( suite );

	// Change the default outputter to a compiler error format outputter
	runner.setOutputter( new CppUnit::TextOutputter( &runner.result(),std::cerr ) );
	// Run the tests.
	bool wasSucessful = runner.run();
	if(wasSucessful == false)
		return 0;
	return 1;
}

#endif
