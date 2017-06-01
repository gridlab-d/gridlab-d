// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#include "GldEditor.h"

#include "GldEditorDoc.h"
#include "GldEditorView.h"
#include "MainFrm.h"
#include "LeftView.h"

#include "object.h"
#include "class.h"
#include "module.h"
#include "globals.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, &CLeftView::OnTvnSelchanged)
END_MESSAGE_MAP()


// CLeftView construction/destruction

CLeftView::CLeftView()
{
	// TODO: add construction code here
}

CLeftView::~CLeftView()
{
}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying the CREATESTRUCT cs

	return CTreeView::PreCreateWindow(cs);
}

void CLeftView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	LONG ws = GetWindowLong(m_hWnd,GWL_STYLE);
	SetWindowLong(m_hWnd,GWL_STYLE,ws|TVS_HASLINES|TVS_LINESATROOT|TVS_HASBUTTONS);
}

void CLeftView::OnUpdate(CView *pSender, LPARAM lHint, CObject *pHint)
{
	CGldEditorDoc *pDoc = GetDocument();
	ASSERT(pDoc!=NULL);

	CTreeCtrl &tree = GetTreeCtrl();
	while (tree.GetCount()>0)
	{
		HTREEITEM hItem = tree.GetNextItem(TVI_ROOT,TVGN_ROOT);
		HTREEITEM hSubItem = hItem;
		while ((hSubItem=tree.GetChildItem(hSubItem))!=NULL)
			hItem = hSubItem;
		CTreeRef *pRef = (CTreeRef*)tree.GetItemData(hItem);
		if (pRef)
			delete pRef;
		tree.DeleteItem(hItem);
	}

	CTreeRef *pRef;
	HTREEITEM modules = tree.InsertItem("Model");
	for (MODULE *mod=module_get_first(); mod!=NULL; mod=mod->next)
	{
		HTREEITEM hItem1 = tree.InsertItem(mod->name,modules);
		pRef = new CTreeRef("module",(DWORD_PTR)mod);
		tree.SetItemData(hItem1,(DWORD_PTR)pRef);
		for (CLASS *oclass=class_get_first_class(); oclass!=NULL; oclass=oclass->next)
		{
			if (oclass->module==mod)
			{
				HTREEITEM hItem2 = tree.InsertItem(oclass->name,hItem1);
				pRef = new CTreeRef("class",(DWORD_PTR)oclass);
				tree.SetItemData(hItem2,(DWORD_PTR)pRef);
				for (OBJECT *obj = object_get_first(); obj!=NULL; obj=obj->next)
				{
					if (obj->oclass==oclass)
					{
						HTREEITEM hItem3 = tree.InsertItem(object_name(obj),hItem2);
						tree.SetItemState(hItem2,TVIS_BOLD,TVIS_BOLD);
						tree.SetItemState(hItem1,TVIS_BOLD,TVIS_BOLD);
						tree.SetItemState(modules,TVIS_BOLD,TVIS_BOLD);
						pRef = new CTreeRef("object",(DWORD_PTR)obj);
						tree.SetItemData(hItem3,(DWORD_PTR)pRef);
					}
				}
			}
		}
	}

	HTREEITEM schedules = tree.InsertItem("Schedules");
	SCHEDULE *sch = NULL;
	while ((sch=schedule_getnext(sch))!=NULL)
	{
		HTREEITEM hItem1 = tree.InsertItem(sch->name,schedules);
		pRef = new CTreeRef("schedule",(DWORD_PTR)sch);
		tree.SetItemData(hItem1,(DWORD_PTR)pRef);
		tree.SetItemState(schedules,TVIS_BOLD,TVIS_BOLD);
		int block;
		for (block=0; block<sch->block; block++)
		{
			HTREEITEM hItem2 = tree.InsertItem(sch->blockname[block],hItem1);
			char refname[1024];
			sprintf(refname,"schedule:%d",block);
			pRef = new CTreeRef(refname,(DWORD_PTR)sch);
			tree.SetItemData(hItem2,(DWORD_PTR)pRef);
			tree.SetItemState(hItem1,TVIS_BOLD,TVIS_BOLD);
		}
	}

	HTREEITEM files = tree.InsertItem("Files");
	OBJECT *obj;
	for (obj=object_get_first(); obj!=NULL; obj=obj->next)
	{
		PROPERTY *prop;
		for (prop=class_get_first_property(obj->oclass); prop!=NULL; prop=class_get_next_property(prop))
		{
			if (strstr(prop->name,"file")!=NULL)
			{
				char *file = (char*)object_get_addr(obj,prop->name);
				if (file!=NULL)
				{	
					HTREEITEM hItem = tree.InsertItem(file,files);
					pRef = new CTreeRef("file",(DWORD_PTR)file);
					tree.SetItemData(hItem,(DWORD_PTR)pRef);
					tree.SetItemState(files,TVIS_BOLD,TVIS_BOLD);
				}
			}
		}
	}
	HTREEITEM globals = tree.InsertItem("System");
	pRef = new CTreeRef("globals",NULL);
	tree.SetItemData(globals,(DWORD_PTR)pRef);

	HTREEITEM solver = tree.InsertItem("Solver");
	pRef = new CTreeRef("solver",NULL);
	tree.SetItemData(solver,(DWORD_PTR)pRef);
}

// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
	CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);
}

CGldEditorDoc* CLeftView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGldEditorDoc)));
	return (CGldEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CLeftView message handlers

void CLeftView::OnTvnSelchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	CGldEditorView *pEditor = ((CMainFrame*)(theApp.GetMainWnd()))->GetEditorPane();
	CListCtrl &editor = pEditor->GetListCtrl();
	editor.DeleteAllItems();

	CTreeCtrl &tree = GetTreeCtrl();
	HTREEITEM hItem = ((TVITEM)(pNMTreeView->itemNew)).hItem;
	HTREEITEM hParent = tree.GetParentItem(hItem);
	CTreeRef *pRef = (CTreeRef*)tree.GetItemData(hItem);
	if (pRef!=NULL)
	{
		CString type = pRef->GetType();
		if (type=="object")
			pEditor->LoadObject((OBJECT*)pRef->GetData());
		else if (type=="class")
			pEditor->LoadClass((CLASS*)pRef->GetData());
		else if (type=="module")
			pEditor->LoadModule((MODULE*)(pRef->GetData()));
		else if (type=="globals")
			pEditor->LoadGlobals();
		else if (type=="solver")
			pEditor->LoadSolver();
		else if (type=="file")
			pEditor->LoadFile((char*)(pRef->GetData()));
		else if (type.Left(8)=="schedule")
		{
			if (strchr(type,':'))
				pEditor->LoadScheduleBlock((SCHEDULE*)(pRef->GetData()),atoi(type.Mid(9)));
			else
				pEditor->LoadSchedule((SCHEDULE*)(pRef->GetData()));
		}
		else /* add other types here */
		{
		}
	}

	*pResult = 0;
}
