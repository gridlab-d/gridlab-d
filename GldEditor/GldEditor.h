// GldEditor.h : main header file for the GldEditor application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CGldEditorApp:
// See GldEditor.cpp for the implementation of this class
//

class CGldEditorApp : public CWinApp
{
public:
	CGldEditorApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation
	afx_msg void OnAppAbout();
	afx_msg void OnFileNewFrame();
	afx_msg void OnFileNew();
	DECLARE_MESSAGE_MAP()
};

extern CGldEditorApp theApp;