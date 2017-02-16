// core_testDlg.h : header file
//

#if !defined(AFX_CORE_TESTDLG_H__8E8F2A3F_3CDA_4B03_841F_D16CED434FC1__INCLUDED_)
#define AFX_CORE_TESTDLG_H__8E8F2A3F_3CDA_4B03_841F_D16CED434FC1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCore_testDlg dialog

class CCore_testDlg : public CDialog
{
// Construction
public:
	CCore_testDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCore_testDlg)
	enum { IDD = IDD_CORE_TEST_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCore_testDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CCore_testDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CORE_TESTDLG_H__8E8F2A3F_3CDA_4B03_841F_D16CED434FC1__INCLUDED_)
