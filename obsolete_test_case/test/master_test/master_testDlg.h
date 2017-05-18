// master_testDlg.h : header file
//

#if !defined(AFX_MASTER_TESTDLG_H__FD9BF231_1271_4FF8_AB62_847EB0C32D5A__INCLUDED_)
#define AFX_MASTER_TESTDLG_H__FD9BF231_1271_4FF8_AB62_847EB0C32D5A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CMaster_testDlg dialog

class CMaster_testDlg : public CDialog
{
// Construction
public:
	CMaster_testDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMaster_testDlg)
	enum { IDD = IDD_MASTER_TEST_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMaster_testDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMaster_testDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MASTER_TESTDLG_H__FD9BF231_1271_4FF8_AB62_847EB0C32D5A__INCLUDED_)
