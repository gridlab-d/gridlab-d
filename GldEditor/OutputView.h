// OutputView.h : interface of the COutputView class
//

#pragma once

class COutputView : public CTreeView
{
protected: // create from serialization only
	COutputView();
	DECLARE_DYNCREATE(COutputView)

// Attributes
public:
	CGldEditorDoc* GetDocument() const;

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
	virtual ~COutputView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in OutputView.cpp
inline CGldEditorDoc* COutputView::GetDocument() const
   { return reinterpret_cast<CGldEditorDoc*>(m_pDocument); }
#endif

