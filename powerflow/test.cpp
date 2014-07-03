/** $Id: test.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 * @file test.cpp 
 * This file contains the module_test function, which is called from the
 * gridlab core when the --test <modulename> command line option is used.
 * Nothing in this file should be modified.  Any test classes containing
 * module tests should be placed in the test.h file.  To use this file,
 * a few steps need to be taken to allow for compilation in Visual Studio.
 * 	-# This file, and test.h should be copied into the directory containing 
 * 	the source files for the module to be tested. 
 * 	-# The project for the module needs to be told where to find the cppunit
 * 	include files.  This is done by modifying the properties using the
 * 	properties dialog, expanding the C/C++ subtree, selecting the Additional
 * 	Include Directories property and adding the location of the cppunit include
 * 	directory (usually ..\test\cppunit2\include).
 * 	-# The project needs to be told where to find the shared libraries for
 * 	cppunit.  This will require 2 modifications to the project properties.
 * 		-# Expand the linker subtree, and under Additional Dependancies, add
 * 		cppunitd.lib (for debug configuration) or cppunit.lib (for release
 * 		configuration).
 * 		-# Select the general child of the linker subtree, and add the location
 * 		to the cppunit library files to the Additional Library Directories item
 * 		(usually ..\test\cppunit2\lib)
 * At this point, it should be possible to compile your project with the test
 * files included.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"

#ifndef _NO_CPPUNIT
// Include the local copy of test.h
#include "./test.h"


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
    // It is likely that no modifications will be necessary to this function.
    // Tests should be implemented in the header file above, and created as
    // a CPPUnit test suite.
	printf("");

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
