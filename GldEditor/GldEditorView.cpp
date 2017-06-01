// GldEditorView.cpp : implementation of the CGldEditorView class
//

#include "stdafx.h"
#include <time.h>
#include "GldEditor.h"

#include "GldEditorDoc.h"
#include "GldEditorView.h"

#include "convert.h"
#include "module.h"
#include "exec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CGldEditorView

IMPLEMENT_DYNCREATE(CGldEditorView, CListView)

BEGIN_MESSAGE_MAP(CGldEditorView, CListView)
	ON_WM_STYLECHANGED()
END_MESSAGE_MAP()

// CGldEditorView construction/destruction

CGldEditorView::CGldEditorView()
{
	// TODO: add construction code here

}

CGldEditorView::~CGldEditorView()
{
}

BOOL CGldEditorView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs
	cs.style|=LVS_REPORT;

	return CListView::PreCreateWindow(cs);
}

void CGldEditorView::OnInitialUpdate()
{
	CListView::OnInitialUpdate();

	ListView_SetExtendedListViewStyleEx(GetListCtrl().m_hWnd,
		LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES ,
		LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES  );
}


// CGldEditorView diagnostics

#ifdef _DEBUG
void CGldEditorView::AssertValid() const
{
	CListView::AssertValid();
}

void CGldEditorView::Dump(CDumpContext& dc) const
{
	CListView::Dump(dc);
}

CGldEditorDoc* CGldEditorView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CGldEditorDoc)));
	return (CGldEditorDoc*)m_pDocument;
}
#endif //_DEBUG


// CGldEditorView message handlers
void CGldEditorView::OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct)
{
	//TODO: add code to react to the user changing the view style of your window	
	CListView::OnStyleChanged(nStyleType,lpStyleStruct);	
}

void CGldEditorView::LoadObject(OBJECT *obj)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Name = list.InsertColumn(nCol++,"Name",LVCFMT_LEFT,W(150),nCol);
	int Type = list.InsertColumn(nCol++,"Type",LVCFMT_LEFT,W(100),nCol);
	int Class = list.InsertColumn(nCol++,"Class",LVCFMT_LEFT,W(150),nCol);
	int Access = list.InsertColumn(nCol++,"Access",LVCFMT_LEFT,W(100),nCol);
	int Data = list.InsertColumn(nCol++,"Data",LVCFMT_RIGHT,W(100),nCol);
	int Description = list.InsertColumn(nCol++,"Description",LVCFMT_LEFT,wr.Width()-nWid,nCol);
	#undef W

	int nItem;
	char buffer[1024]="";
	CLASS *oclass;
	PROPERTY *prop;

	nItem = list.InsertItem(list.GetItemCount(),"clock");
	list.SetItemText(nItem,Type,"TIMESTAMP");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PROTECTED");
	list.SetItemText(nItem,Data,convert_from_timestamp(obj->clock,buffer,sizeof(buffer))?buffer:"");

	nItem = list.InsertItem(list.GetItemCount(),"name");
	list.SetItemText(nItem,Type,"OBJECTNAME");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,object_name(obj));

	nItem = list.InsertItem(list.GetItemCount(),"id");
	list.SetItemText(nItem,Type,"OBJECTNUM");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"REFERENCE");
	sprintf(buffer,"%d",obj->id);
	list.SetItemText(nItem,Data,buffer);

	nItem = list.InsertItem(list.GetItemCount(),"class");
	list.SetItemText(nItem,Type,"CLASSNAME");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"REFERENCE");
	list.SetItemText(nItem,Data,obj->oclass->name);

	nItem = list.InsertItem(list.GetItemCount(),"size");
	list.SetItemText(nItem,Type,"OBJECTRANK");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"REFERENCE");
	sprintf(buffer,"%d",obj->oclass->size);
	list.SetItemText(nItem,Data,buffer);

	nItem = list.InsertItem(list.GetItemCount(),"parent");
	list.SetItemText(nItem,Type,"OBJECTNAME");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,object_name(obj->parent));

	nItem = list.InsertItem(list.GetItemCount(),"rank");
	list.SetItemText(nItem,Type,"OBJECTRANK");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	sprintf(buffer,"%d",obj->rank);
	list.SetItemText(nItem,Data,buffer);

	nItem = list.InsertItem(list.GetItemCount(),"in_svc");
	list.SetItemText(nItem,Type,"TIMESTAMP");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,convert_from_timestamp(obj->in_svc,buffer,sizeof(buffer))?buffer:"");

	nItem = list.InsertItem(list.GetItemCount(),"out_svc");
	list.SetItemText(nItem,Type,"TIMESTAMP");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,convert_from_timestamp(obj->out_svc,buffer,sizeof(buffer))?buffer:"");

	nItem = list.InsertItem(list.GetItemCount(),"latitude");
	list.SetItemText(nItem,Type,"double");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,convert_from_latitude(obj->latitude,buffer,sizeof(buffer))?buffer:"");

	nItem = list.InsertItem(list.GetItemCount(),"longitude");
	list.SetItemText(nItem,Type,"double");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PUBLIC");
	list.SetItemText(nItem,Data,convert_from_latitude(obj->longitude,buffer,sizeof(buffer))?buffer:"");

	nItem = list.InsertItem(list.GetItemCount(),"flags");
	list.SetItemText(nItem,Type,"set");
	list.SetItemText(nItem,Class,"OBJECTHDR");
	list.SetItemText(nItem,Access,"PROTECTED");
	list.SetItemText(nItem,Data,convert_from_set(buffer,sizeof(buffer),&(obj->flags),object_flag_property())?buffer:"");

	for (oclass=obj->oclass; oclass!=NULL; oclass=oclass->parent)
	{
		list.InsertItem(list.GetItemCount(),"");
		for (prop=class_get_first_property(oclass); prop!=NULL; prop=class_get_next_property(prop))
		{
			nItem = list.InsertItem(list.GetItemCount(),prop->name);
			list.SetItemText(nItem,Type,class_get_property_typename(prop->ptype));
			list.SetItemText(nItem,Class,oclass->name);
			list.SetItemText(nItem,Access,convert_from_enumeration(buffer,sizeof(buffer),&(prop->access),object_access_property())?buffer:"");
			list.SetItemText(nItem,Data,object_get_value_by_name(obj,prop->name,buffer,sizeof(buffer))?buffer:"(error)");
			list.SetItemText(nItem,Description,prop->description);
		}
	};
}

void CGldEditorView::LoadClass(CLASS *oclass)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Name = list.InsertColumn(nCol++,"Name",LVCFMT_LEFT,W(150),nCol);
	int Type = list.InsertColumn(nCol++,"Type",LVCFMT_LEFT,W(100),nCol);
	int Class = list.InsertColumn(nCol++,"Class",LVCFMT_LEFT,W(150),nCol);
	int Access = list.InsertColumn(nCol++,"Access",LVCFMT_LEFT,W(100),nCol);
	int Data = list.InsertColumn(nCol++,"Data",LVCFMT_LEFT,wr.Width()-nWid,nCol);
	#undef W

	char buffer[1024]="";
	int nItem;

	nItem = list.InsertItem(list.GetItemCount(),"inherit");
	list.SetItemText(nItem,Type,"CLASS");
	list.SetItemText(nItem,Class,"");
	list.SetItemText(nItem,Access,"PROTECTED");
	list.SetItemText(nItem,Data,oclass->parent?oclass->parent->name:"");

	nItem = list.InsertItem(list.GetItemCount(),"passconfig");
	list.SetItemText(nItem,Type,"set");
	list.SetItemText(nItem,Class,"");
	list.SetItemText(nItem,Access,"PROTECTED");
	strcpy(buffer,"");
	if (oclass->passconfig&PC_PRETOPDOWN) strcat(buffer,"PRETOPDOWN");
	if (oclass->passconfig&PC_BOTTOMUP) strcat(buffer,buffer[0]=='\0'?"":"|"),strcat(buffer,"BOTTOMUP");
	if (oclass->passconfig&PC_POSTTOPDOWN) strcat(buffer,buffer[0]=='\0'?"":"|"),strcat(buffer,"POSTTOPDOWN");
	list.SetItemText(nItem,Data,buffer);

	nItem = list.InsertItem(list.GetItemCount(),"module");
	list.SetItemText(nItem,Type,"MODULE");
	list.SetItemText(nItem,Class,"");
	list.SetItemText(nItem,Access,"PROTECTED");
	list.SetItemText(nItem,Data,oclass->module->name);

	list.InsertItem(list.GetItemCount(),"");

	PROPERTY *prop;
	for (prop=class_get_first_property(oclass); prop!=NULL; prop=class_get_next_property(prop))
	{
		nItem = list.InsertItem(list.GetItemCount(),prop->name);

		list.SetItemText(nItem,Type,class_get_property_typename(prop->ptype));

		list.SetItemText(nItem,Class,oclass->name);

		convert_from_enumeration(buffer,sizeof(buffer),&(prop->access),object_access_property());
		list.SetItemText(nItem,Access,buffer);

		list.SetItemText(nItem,Data,"");
	}
}

void CGldEditorView::LoadModule(MODULE *mod)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Name = list.InsertColumn(nCol++,"Property",LVCFMT_LEFT,W(150),nCol);
	int Data = list.InsertColumn(nCol++,"Value(s)",LVCFMT_LEFT,wr.Width()-nWid,nCol);
	#undef W

	char buffer[1024];
	int nItem;
	
	nItem = list.InsertItem(list.GetItemCount(),"Name");
	list.SetItemText(nItem,Data,mod->name);

	nItem = list.InsertItem(list.GetItemCount(),"Path");
	module_get_path(buffer,sizeof(buffer),mod);
	list.SetItemText(nItem,Data,buffer);

	nItem = list.InsertItem(list.GetItemCount(),"Version");
	sprintf(buffer,"%d.%02d", mod->major, mod->minor);
	list.SetItemText(nItem,Data,buffer);

	CLASS *oclass;
	for (oclass=mod->oclass; oclass!=NULL && oclass->module==mod; oclass=oclass->next)
	{	//if (oclass!=NULL && oclass->module==mod)
			nItem = list.InsertItem(list.GetItemCount(),oclass==mod->oclass?"Classes":"");
		list.SetItemText(nItem,Data,oclass->name);
	}

	nItem = list.InsertItem(list.GetItemCount(),"Implementations");
	if (mod->cmdargs!=NULL) {list.SetItemText(nItem,Data,"cmdargs "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->getvar!=NULL) {list.SetItemText(nItem,Data,"getvar "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->setvar!=NULL) {list.SetItemText(nItem,Data,"setvar "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->import_file!=NULL) {list.SetItemText(nItem,Data,"import_file "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->export_file!=NULL) {list.SetItemText(nItem,Data,"export_file "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->check!=NULL) {list.SetItemText(nItem,Data,"check "); nItem=list.InsertItem(list.GetItemCount(),"");}
	if (mod->kmldump!=NULL) {list.SetItemText(nItem,Data,"kmldump "); nItem=list.InsertItem(list.GetItemCount(),"");}
#ifndef _NO_CPPUNIT
	if (mod->module_test!=NULL) {list.SetItemText(nItem,Data,"module_test ");}
#endif
	PROPERTY *prop;
	nItem = list.InsertItem(list.GetItemCount(),"Globals");
	for (prop=mod->globals; prop!=NULL; prop=prop->next)
	{
		if (prop!=mod->globals)
			nItem = list.InsertItem(list.GetItemCount(),"");
		list.SetItemText(nItem,Data,prop->name);
	}
	GLOBALVAR *global=NULL;
	while ((global=global_getnext(global))!=NULL)
	{
		char modname[1024], varname[1024];
		if (sscanf(global->name,"%[^:]::%s",modname,varname)==2 && strcmp(modname,mod->name)==0)
		{
			if (prop!=mod->globals)
				nItem = list.InsertItem(list.GetItemCount(),"");
			list.SetItemText(nItem,Data,global->name);
		}
	}
}

void CGldEditorView::LoadGlobals(void)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Name = list.InsertColumn(nCol++,"Name",LVCFMT_LEFT,W(150),nCol);
	int Module = list.InsertColumn(nCol++,"Module",LVCFMT_LEFT,W(150),nCol);
	int Type = list.InsertColumn(nCol++,"Type",LVCFMT_LEFT,W(100),nCol);
	int Access = list.InsertColumn(nCol++,"Access",LVCFMT_LEFT,W(150),nCol);
	int Data = list.InsertColumn(nCol++,"Data",LVCFMT_LEFT,wr.Width()-nWid,nCol);
	#undef W

	GLOBALVAR *var = NULL;
	while ((var=global_getnext(var))!=NULL)
	{
		char modname[1024], varname[1024];
		int nItem = list.InsertItem(list.GetItemCount(),var->name);
		char buffer[1024]="";

		if (sscanf(var->name,"%[^:]::%s", modname, varname)==2)
		{
			list.SetItemText(nItem,Name,varname);
			list.SetItemText(nItem,Module,modname);
		}

		list.SetItemText(nItem,Type,class_get_property_typename(var->prop->ptype));

		convert_from_enumeration(buffer,sizeof(buffer),&(var->prop->access),object_access_property());
		list.SetItemText(nItem,Access,buffer);

		global_getvar(var->name,buffer,sizeof(buffer));
		list.SetItemText(nItem,Data,buffer);
	}
}

void CGldEditorView::LoadSolver(void)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Pass = list.InsertColumn(nCol++,"Pass",LVCFMT_LEFT,W(100),nCol);
	int Rank = list.InsertColumn(nCol++,"Rank",LVCFMT_LEFT,W(50),nCol);
	int Object = list.InsertColumn(nCol++,"Object",LVCFMT_LEFT,W(150),nCol);
	int ProcId = list.InsertColumn(nCol++,"ProcId",LVCFMT_LEFT,W(50),nCol);
	int Status = list.InsertColumn(nCol++,"Status",LVCFMT_LEFT,W(200),nCol);
	#undef W

	int nItem = list.InsertItem(list.GetItemCount(),"");
	struct {
		char *name;
		PASSCONFIG id;
	} passmap[] = {
		{"PRETOPDOWN",PC_PRETOPDOWN},
		{"BOTTOMUP",PC_BOTTOMUP},
		{"POSTTOPDOWN",PC_POSTTOPDOWN},
	};
	char buffer[1024];
	for (int pass=0; pass<sizeof(passmap)/sizeof(passmap[0]); pass++)
	{
		#define PASSINIT(p) (p % 2 ? ranks[p]->first_used : ranks[p]->last_used)
		#define PASSCMP(i, p) (p % 2 ? i <= ranks[p]->last_used : i >= ranks[p]->first_used)
		#define PASSINC(p) (p % 2 ? 1 : -1)
		list.SetItemText(nItem,0,passmap[pass].name);
		INDEX **ranks = exec_getranks();
		if (ranks!=NULL && ranks[pass]!=NULL)
		{
			int i;

			/* process object in order of rank using index */
			for (i = PASSINIT(pass); PASSCMP(i, pass); i += PASSINC(pass))
			{
				LISTITEM *item;
				/* skip empty lists */
				if (ranks[pass]->ordinal[i] == NULL) 
					continue;

				sprintf(buffer,"%d",i);
				list.SetItemText(nItem,Rank,buffer);
							
				for (item=ranks[pass]->ordinal[i]->first; item!=NULL; item=item->next)
				{
					OBJECT *obj = (OBJECT*)item->data;
					list.SetItemText(nItem,Object,obj->name);
					
					sprintf(buffer,"%d",obj->tp_affinity);
					list.SetItemText(nItem,ProcId,buffer);

					list.SetItemText(nItem,Status,convert_from_set(buffer,sizeof(buffer),&(obj->flags),object_flag_property())?buffer:"(na)");

					nItem = list.InsertItem(list.GetItemCount(),"");
				}
			}
		}
	}
	list.DeleteItem(nItem);
}

void CGldEditorView::LoadFile(char *filename)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Line = list.InsertColumn(nCol++,"Line",LVCFMT_LEFT,W(50),nCol);
	int Text = list.InsertColumn(nCol++,"Text",LVCFMT_LEFT,wr.Width()-nWid,nCol);
	#undef W

	char *file = find_file(filename,NULL,4);
	if (file==NULL)
	{
		int nItem = list.InsertItem(list.GetItemCount(),"ERROR");
		list.SetItemText(nItem,Text,"File not found");
		return;
	}

	int nItem = list.InsertItem(list.GetItemCount(),"File");
	list.SetItemText(nItem,Text,file);

	FILE *fp = fopen(file,"r");
	char buffer[1024];
	int line=0;
	while (fp!=NULL && !ferror(fp) && !feof(fp) && fgets(buffer,sizeof(buffer),fp))
	{
		line++;
		char linenum[64];
		sprintf(linenum,"%d",list.GetItemCount());
		nItem = list.InsertItem(list.GetItemCount(),linenum);
		CString text(buffer);
		text.Replace("\n","");
		list.SetItemText(nItem,Text,text);
		if (list.GetItemCount()>100)
		{
			nItem = list.InsertItem(list.GetItemCount(),"MORE");
			sprintf(buffer,"- %d", line);
			list.SetItemText(nItem,Text,buffer);
			break;
		}
	}
	if (fp==NULL || ferror(fp))
	{
		int nItem = list.InsertItem(list.GetItemCount(),"ERROR");
		list.SetItemText(nItem,Text,strerror(errno));
	}
}

void CGldEditorView::LoadScheduleBlock(SCHEDULE *sch, unsigned int block)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int Cal = list.InsertColumn(nCol++,"Month",LVCFMT_LEFT,W(150),nCol);
	int dowCol[8];
	dowCol[0] = list.InsertColumn(nCol++,"Sun",LVCFMT_LEFT,W(50),nCol);
	dowCol[1] = list.InsertColumn(nCol++,"Mon",LVCFMT_LEFT,W(50),nCol);
	dowCol[2] = list.InsertColumn(nCol++,"Tue",LVCFMT_LEFT,W(50),nCol);
	dowCol[3] = list.InsertColumn(nCol++,"Wed",LVCFMT_LEFT,W(50),nCol);
	dowCol[4] = list.InsertColumn(nCol++,"Thu",LVCFMT_LEFT,W(50),nCol);
	dowCol[5] = list.InsertColumn(nCol++,"Fri",LVCFMT_LEFT,W(50),nCol);
	dowCol[6] = list.InsertColumn(nCol++,"Sat",LVCFMT_LEFT,W(50),nCol);
	dowCol[7] = list.InsertColumn(nCol++,"Hol",LVCFMT_LEFT,W(50),nCol);
	#undef W

	char *months[] = {"January","February","March","April","May","June","July","August","September","October","November","December"};
	int days[] = {31,28,31,30,31,30,31,31,30,31,30,31}; 
	int year=2000;
	for (int month=1; month<=12; month++)
	{
		char buffer[64];
		sprintf(buffer,"%s",months[month-1]);
		int nItem = list.InsertItem(list.GetItemCount(),buffer);
		int hItem[24];
		for (int hour=0; hour<24; hour++)
		{
			char buffer[64];
			sprintf(buffer,"    %d:00",hour);
			hItem[hour] = list.InsertItem(list.GetItemCount(),buffer);
		}
		for (int day=0; day<7; day++)
		{
			for (int hour=0; hour<24; hour++)
			{
				DATETIME dt = {year,month,day+1,hour,0,0};
				TIMESTAMP ts = mkdatetime(&dt);
				local_datetime(ts,&dt);
				SCHEDULEINDEX ref = schedule_index(sch,ts);
				double value = schedule_value(sch,ref);
				char buffer[64];
				sprintf(buffer,"%g%c",value,schedule_dtnext(sch,ref)<60?'*':' ');
				list.SetItemText(hItem[hour],dowCol[dt.weekday],buffer);
			}
		}
	}
}

void CGldEditorView::LoadSchedule(SCHEDULE *sch)
{
	CListCtrl &list = GetListCtrl();
	
	int nColumns = list.GetHeaderCtrl()?list.GetHeaderCtrl()->GetItemCount():0;
	for (int i=0; i<nColumns; i++)
		list.DeleteColumn(0);

	CRect wr;
	list.GetClientRect(&wr);
	int nCol=0;
	int nWid=0;
	#define W(X) (nWid+=X,X)
	int year = list.InsertColumn(nCol++,"Year",LVCFMT_LEFT,W(45),nCol);
	int month = list.InsertColumn(nCol++,"Month",LVCFMT_LEFT,W(50),nCol);
	int weekday = list.InsertColumn(nCol++,"Weekday",LVCFMT_LEFT,W(60),nCol);
	int day = list.InsertColumn(nCol++,"Day",LVCFMT_RIGHT,W(35),nCol);
	int hour = list.InsertColumn(nCol++,"Time",LVCFMT_RIGHT,W(50),nCol);
	int value = list.InsertColumn(nCol++,"Value",LVCFMT_RIGHT,W(50),nCol);
	#undef W

	TIMESTAMP tstart = global_clock; // TODO: this should be global_starttime but it's already been set to wall clock by this time
	TIMESTAMP tstop = global_stoptime;
	if (tstart==0)
	{
		time_t now = time(NULL);
		struct tm* ts = localtime(&now);
		DATETIME dt = {ts->tm_year,1,1,0,0,0,0,0};
		tstart = mkdatetime(&dt);
	}
	if (tstop==TS_NEVER || tstop<tstart)
	{
		DATETIME dt;
		local_datetime(tstart,&dt);
		dt.year++;
		tstop = mkdatetime(&dt);
	}

	TIMESTAMP t = tstart;
	DATETIME last = {0,0,0,-1,0,0}; // -1 forces first time to be displayed even when it's midnight
	int max = 100;
	double lastvalue=-1; // -1 forces first time to be displayed even when it's zero value
	while (t<tstop )// && max-->0)
	{
		SCHEDULEINDEX ndx = schedule_index(sch,t);
	
		DATETIME next;
		local_datetime(t,&next);
		double nextvalue = schedule_value(sch,ndx);
		if (nextvalue!=lastvalue)
		{
			int hItem = list.InsertItem(list.GetItemCount(),"");
			CString buf;

			if (last.year!=next.year)
			{
				buf.Format("%d",next.year);
				list.SetItemText(hItem,year,buf);
				last.year = next.year;
			}

			if (last.month!=next.month)
			{
				char *mon[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
				list.SetItemText(hItem,month,mon[next.month-1]);
				last.month = next.month;
			}

			if (last.day!=next.day)
			{
				buf.Format("%d",next.day);
				list.SetItemText(hItem,day,buf);
				last.day = next.day;
				char *wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
				list.SetItemText(hItem,weekday,wd[next.weekday]);
			}

			if (last.hour!=next.hour || last.minute!=next.minute)
			{
				buf.Format("%d:%02d",next.hour,next.minute);
				list.SetItemText(hItem,hour,buf);
				last.hour = next.hour;
				last.minute = next.minute;
			}

			buf.Format("%g", nextvalue);
			list.SetItemText(hItem,value,buf);
			lastvalue = nextvalue;
		}
		t+=schedule_dtnext(sch,ndx)*60; // TODO increment by dtnext
	}
}