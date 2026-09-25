// MasterUnitTest.cpp: implementation of the MasterUnitTest class.
//
//////////////////////////////////////////////////////////////////////

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/ui/text/TestRunner.h>

#include "CoreUnitTest.h"
#include "MasterUnitTest.h"
#include "master_test.h"
#include "stdafx.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

MasterUnitTest::MasterUnitTest() {}

MasterUnitTest::~MasterUnitTest() {}
