// CoreUnitTest.cpp: implementation of the CoreUnitTest class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "core_test.h"
#include <cppunit/ui/text/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#include "CoreUnitTest.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CoreUnitTest::CoreUnitTest()
{

}

CoreUnitTest::~CoreUnitTest()
{

}

void CoreUnitTest::testModuleLoad()
{
    CPPUNIT_ASSERT(network != NULL);
}

void CoreUnitTest::testNodeLoad()
{
    CPPUNIT_ASSERT(node != NULL);
}

void CoreUnitTest::testLinkLoad()
{
    CPPUNIT_ASSERT(link != NULL);
}

void CoreUnitTest::testTapeLoad()
{
    CPPUNIT_ASSERT(tape != NULL);
}

void CoreUnitTest::testPlayerLoad()
{
    CPPUNIT_ASSERT(player != NULL);
}

void CoreUnitTest::testRecorderLoad()
{
    CPPUNIT_ASSERT(recorder != NULL);
}

void CoreUnitTest::testCollectorLoad()
{
    CPPUNIT_ASSERT(collector != NULL);
}

void CoreUnitTest::testModuleImport()
{
    CPPUNIT_ASSERT(module_import(network,"../test/pnnl2bus.cdf")<=0);
}

void CoreUnitTest::testPlayerCreate()
{
    CPPUNIT_ASSERT((*player->create)(&obj[3],object_get_first())==FAILED);
    
    // Not sure what this is needed for, or if I should have a separate test for it.
    object_set_value_by_name(obj[3],"loop","3600"); /* 18000 is about 12y at 1h steps */
    object_set_value_by_name(obj[3],"property","S");
}

void CoreUnitTest::testRecorderCreate()
{
    CPPUNIT_ASSERT((*recorder->create)(&obj[4],object_get_first())==FAILED);
    
    // Not sure what this is needed for, or if I should have a separate test for it.
    object_set_value_by_name(obj[4],"property","V,S");
    object_set_value_by_name(obj[4],"interval","0");
    object_set_value_by_name(obj[4],"limit","1000");
}

void CoreUnitTest::testTapeCollectorCreate()
{
    CPPUNIT_ASSERT((*collector->create)(&obj[5],NULL)==FAILED);
    
    // Not sure what this is needed for, or if I should have a separate test for it.
    object_set_value_by_name(obj[5],"property","count(V.mag),min(V.mag),avg(V.mag),std(V.mag),max(V.mag),min(V.ang),avg(V.ang),std(V.ang),max(V.ang)");
    object_set_value_by_name(obj[5],"interval","0");
    object_set_value_by_name(obj[5],"limit","1000");
    object_set_value_by_name(obj[5],"group","class=node;");
}