// ControlView.h : interface of the CLeftView class
//


#pragma once

class CGldEditorDoc;

class CControlView : public CTreeView
{
protected: // create from serialization only
	CControlView();
	DECLARE_DYNCREATE(CControlView)

// Attributes
public:
	CGldEditorDoc* GetDocument();
	HTREEITEM m_hItemCommand, m_hItemOutput, m_hItemSearch, m_hItemDebugger;

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
	virtual ~CControlView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CGldEditorDoc* CControlView::GetDocument()
   { return reinterpret_cast<CGldEditorDoc*>(m_pDocument); }
#endif

