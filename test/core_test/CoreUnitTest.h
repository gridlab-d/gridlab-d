// CoreUnitTest.h: interface for the CoreUnitTest class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COREUNITTEST_H__CE7CC8F7_2938_4F57_B16F_ECC40B9604BE__INCLUDED_)
#define AFX_COREUNITTEST_H__CE7CC8F7_2938_4F57_B16F_ECC40B9604BE__INCLUDED_

#include "globals.h"
#include "cmdarg.h"
#include "module.h"
#include "output.h"
#include "save.h"
#include "find.h"
#include "test.h"
#include "aggregate.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CoreUnitTest : public CppUnit::TestFixture  
{
private:
    OBJECT *obj[6];
	MODULE *network;
	CLASS *node, *link;
	MODULE *tape;
	CLASS *player, *recorder, *collector;

public:
	CoreUnitTest();
	virtual ~CoreUnitTest();
	
	/*
	 * Setup all the objects that will be in use for this test.
	 *
	 */
    void setUp()
    {
        network = module_load("network",argc,argv);
        node = class_get_class_from_classname("node");
        link = class_get_class_from_classname("link");
        tape = module_load("tape",argc,argv);
        player = class_get_class_from_classname("player");
        recorder = class_get_class_from_classname("recorder");
        collector = class_get_class_from_classname("collector");
        
    }
    
	/*
	 * Currently, no teardown activities, though some of the end-test activites
	 * might be appropriate here.
	 */
    void tearDown()
    {
        
    }
    
	/*
	 * All the creation tests that were in test.c
	 */
    void testModuleLoad();
    void testNodeLoad();
    void testLinkLoad();
    void testTapeLoad();
    void testPlayerLoad();
    void testRecorderLoad();
    void testCollectorLoad();
    void testModuleImport();
    void testPlayerCreate();
    void testRecorderCreate();
    void testTapeCollectorCreate();

	// These macro's create the suite() function.
	CPPUNIT_TEST_SUITE( CoreUnitTest );
	// Use CPPUNIT_TEST to create a test for a specific test function as above.
	CPPUNIT_TEST(testModuleLoad);
	CPPUNIT_TEST(testNodeLoad);
	CPPUNIT_TEST(testLinkLoad);
	CPPUNIT_TEST(testTapeLoad);
	CPPUNIT_TEST(testPlayerLoad);
	CPPUNIT_TEST(testRecorderLoad);
	CPPUNIT_TEST(testCollectorLoad);
	CPPUNIT_TEST(testModuleImport);
	CPPUNIT_TEST(testPlayerCreate);
	CPPUNIT_TEST(testRecorderCreate);
	CPPUNIT_TEST(testTapeCollectorCreate);

	//Finishes the creation of the suite() function
	CPPUNIT_TEST_SUITE_END();

};

#endif // !defined(AFX_COREUNITTEST_H__CE7CC8F7_2938_4F57_B16F_ECC40B9604BE__INCLUDED_)
