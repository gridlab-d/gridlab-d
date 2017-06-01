// OutputView.cpp : implementation of the COutputView class
//

#include "stdafx.h"
#include "GldEditor.h"

#include "GldEditorDoc.h"
#include "OutputView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// COutputView

IMPLEMENT_DYNCREATE(COutputView, CTreeView)

BEGIN_MESSAGE_MAP(COutputView, CTreeView)
	ON_WM_STYLECHANGED()
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CTreeView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CTreeView::OnFilePrintPreview)
END_MESSAGE_MAP()

// COutputView construction/destruction

COutputView::COutputView()
{
	// TODO: add construction code here
}

COutputView::~COutputView()
{
}

BOOL COutputView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	return CTreeView::PreCreateWindow(cs);
}

void COutputView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();
	
	LONG ws = GetWindowLong(m_hWnd,GWL_STYLE);
	SetWindowLong(m_hWnd,GWL_STYLE,ws|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);
}

void COutputView::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
{
	CGldEditorDoc *pDoc = GetDocument();
	ASSERT(pDoc!=NULL);

	CTreeCtrl &tree = GetTreeCtrl();
	UINT nItems = tree.GetCount();

	CStringList &buffer = pDoc->m_OutputBuffer;
	POSITION pos = buffer.FindIndex(nItems);

	HTREEITEM group = NULL;
	while (pos!=NULL)
	{
		CString item = buffer.GetNext(pos);
		CString flag = item.Left(1);
		CString text = item.Mid(1);
		text.Replace("\n"," ");
		if (flag=="+" && pos!=NULL)
			group = tree.InsertItem(text);
		else if (flag=="-" || flag=="=")
		{
			ASSERT(group!=NULL);
			HTREEITEM hItem = tree.InsertItem(text,group);
			if (flag=="=")
			{
				tree.EnsureVisible(hItem);
				tree.SetItemState(group,TVIS_BOLD,TVIS_BOLD);
			}
			group = tree.GetParentItem(group);
		}
		else 
		{
			ASSERT(flag==" " || flag=="!");
			HTREEITEM hItem = tree.InsertItem(text,group?group:TVI_ROOT);
			if (flag=="!" && text.TrimLeft().Left(3)!="...")
			{
				tree.EnsureVisible(hItem);
				tree.SetItemState(hItem,TVIS_BOLD,TVIS_BOLD);
				tree.SetItemState(group,TVIS_BOLD,TVIS_BOLD);
			}
		}
	}
	if (group)
		tree.Expand(group,TVE_EXPAND);
}

// COutputView diagnostics

#ifdef _DEBUG
void COutputView::AssertValid() const
{
	CTreeView::AssertValid();
}

void COutputView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CGldEditorDoc* COutputView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGldEditorDoc)));
	return (CGldEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// COutputView message handlers
