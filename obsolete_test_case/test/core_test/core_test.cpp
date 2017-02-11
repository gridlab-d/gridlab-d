// core_test.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "core_test.h"
#include "core_testDlg.h"

// +++++++++++++++++++++++++
    // CppUnit: MFC TestRunner
    #include <cppunit/ui/mfc/TestRunner.h>

    // CppUnit: TestFactoryRegistry to retreive the top test
    // suite that contains all registered tests.
    #include <cppunit/extensions/TestFactoryRegistry.h>
// +++++++++++++++++++++++++

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// +++++++++++++++++++++++++
    // CppUnit: MFC TestRunner
    static AFX_EXTENSION_MODULE extTestRunner;
// +++++++++++++++++++++++++

/////////////////////////////////////////////////////////////////////////////
// CCore_testApp

BEGIN_MESSAGE_MAP(CCore_testApp, CWinApp)
	//{{AFX_MSG_MAP(CCore_testApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCore_testApp construction

CCore_testApp::CCore_testApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCore_testApp object

CCore_testApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCore_testApp initialization

BOOL CCore_testApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Declare & Run MFC TestRunner
    CppUnit::MfcUi::TestRunner runner;
    runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
    runner.run();

	/*
	CCore_testDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}
	*/

	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
