// MasterUnitTest.h: interface for the MasterUnitTest class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MASTERUNITTEST_H__6AF00409_91AB_4AF9_9A75_7D45654E826E__INCLUDED_)
#define AFX_MASTERUNITTEST_H__6AF00409_91AB_4AF9_9A75_7D45654E826E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class MasterUnitTest : public CppUnit::TestFixture
{
public:
	MasterUnitTest();
	virtual ~MasterUnitTest();

	CPPUNIT_TEST_SUITE( MasterUnitTest );
	// Module tests go here.  The CPPUNIT_TEST_SUITE creates a suite() function
	CPPUNIT_TEST(CoreUnitTest::suite());
	CPPUNIT_TEST_SUITE_END();

};

#endif // !defined(AFX_MASTERUNITTEST_H__6AF00409_91AB_4AF9_9A75_7D45654E826E__INCLUDED_)
