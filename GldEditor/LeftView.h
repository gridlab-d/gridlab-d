// LeftView.h : interface of the CLeftView class
//


#pragma once

class CGldEditorDoc;

class CLeftView : public CTreeView
{
protected: // create from serialization only
	CLeftView();
	DECLARE_DYNCREATE(CLeftView)

// Attributes
public:
	CGldEditorDoc* GetDocument();

// Operations
public:

// Overrides
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint);

// Implementation
public:
	virtual ~CLeftView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult);
};

class CTreeRef
{
private:
	CString m_Type;
	DWORD_PTR m_Data;
public:
	inline CTreeRef(char *type, DWORD_PTR data) {m_Type=type;m_Data=data;};
	inline CString &GetType(void) { return m_Type;};
	inline DWORD_PTR GetData(void) { return m_Data;};
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CGldEditorDoc* CLeftView::GetDocument()
   { return reinterpret_cast<CGldEditorDoc*>(m_pDocument); }
#endif

