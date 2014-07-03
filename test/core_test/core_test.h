// core_test.h : main header file for the CORE_TEST application
//

#if !defined(AFX_CORE_TEST_H__A375087A_9154_4A53_B4AC_E1E8DF3456E2__INCLUDED_)
#define AFX_CORE_TEST_H__A375087A_9154_4A53_B4AC_E1E8DF3456E2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCore_testApp:
// See core_test.cpp for the implementation of this class
//

class CCore_testApp : public CWinApp
{
public:
	CCore_testApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCore_testApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCore_testApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CORE_TEST_H__A375087A_9154_4A53_B4AC_E1E8DF3456E2__INCLUDED_)
