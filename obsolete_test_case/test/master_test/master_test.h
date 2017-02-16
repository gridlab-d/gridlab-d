// master_test.h : main header file for the MASTER_TEST application
//

#if !defined(AFX_MASTER_TEST_H__B409516D_A133_4C02_BD05_B2F4928B4054__INCLUDED_)
#define AFX_MASTER_TEST_H__B409516D_A133_4C02_BD05_B2F4928B4054__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CMaster_testApp:
// See master_test.cpp for the implementation of this class
//

class CMaster_testApp : public CWinApp
{
public:
	CMaster_testApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMaster_testApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CMaster_testApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MASTER_TEST_H__B409516D_A133_4C02_BD05_B2F4928B4054__INCLUDED_)
