// GldEditorView.h : interface of the CGldEditorView class
//

#pragma once

#include "object.h"

class CGldEditorView : public CListView
{
protected: // create from serialization only
	CGldEditorView();
	DECLARE_DYNCREATE(CGldEditorView)

// Attributes
public:
	CGldEditorDoc* GetDocument() const;

// Operations
public:
	void LoadObject(OBJECT *obj);
	void LoadClass(CLASS *oclass);
	void LoadModule(MODULE *mod);
	void LoadGlobals(void);
	void LoadSolver(void);
	void LoadFile(char *);
	void LoadSchedule(SCHEDULE *);
	void CGldEditorView::LoadScheduleBlock(SCHEDULE *sch, unsigned int block);

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~CGldEditorView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in GldEditorView.cpp
inline CGldEditorDoc* CGldEditorView::GetDocument() const
   { return reinterpret_cast<CGldEditorDoc*>(m_pDocument); }
#endif

