// ControlView.cpp : implementation of the CControlView class
//

#include "stdafx.h"
#include "GldEditor.h"

#include "GldEditorDoc.h"
#include "ControlView.h"

#include "object.h"
#include "class.h"
#include "module.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CControlView

IMPLEMENT_DYNCREATE(CControlView, CTreeView)

BEGIN_MESSAGE_MAP(CControlView, CTreeView)
END_MESSAGE_MAP()


// CControlView construction/destruction

CControlView::CControlView()
{
	// TODO: add construction code here
}

CControlView::~CControlView()
{
}

BOOL CControlView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs

	return CTreeView::PreCreateWindow(cs);
}

void CControlView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();
	CTreeCtrl &tree = GetTreeCtrl();
	
	LONG ws = GetWindowLong(m_hWnd,GWL_STYLE);
	SetWindowLong(m_hWnd,GWL_STYLE,ws|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);

	tree.DeleteAllItems();
	m_hItemCommand = tree.InsertItem("Commands");
	m_hItemOutput = tree.InsertItem("Output");
	m_hItemSearch = tree.InsertItem("Search");
	m_hItemDebugger = tree.InsertItem("Debugger");

	tree.SetItemState(m_hItemCommand,TVIS_BOLD,TVIS_BOLD);
}

void CControlView::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
{
	CTreeCtrl &tree = GetTreeCtrl();
	CGldEditorDoc *pDoc = GetDocument();
	ASSERT(pDoc!=NULL);
}

// CControlView diagnostics

#ifdef _DEBUG
void CControlView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CControlView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CGldEditorDoc* CControlView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGldEditorDoc)));
	return (CGldEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CControlView message handlers
