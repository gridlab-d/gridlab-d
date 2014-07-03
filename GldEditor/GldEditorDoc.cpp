// GldEditorDoc.cpp : implementation of the CGldEditorDoc class
//

#include "stdafx.h"
#include <direct.h>
#include "GldEditor.h"

#include "GldEditorDoc.h"
#include "GldEditorView.h"

#include "load.h"
#include "exec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CGldEditorDoc *CurrentDocument = NULL;
int view_stdout(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf_s(buffer,format,ptr);
	va_end(ptr);
	ASSERT(CurrentDocument!=NULL);
	CurrentDocument->OutputStdout(CString(buffer));
	return 0;
}
int view_stderr(char *format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf_s(buffer,format,ptr);
	va_end(ptr);
	ASSERT(CurrentDocument!=NULL);
	CurrentDocument->OutputStderr(CString(buffer));
	return 0;
}

// CGldEditorDoc

IMPLEMENT_DYNCREATE(CGldEditorDoc, CDocument)

BEGIN_MESSAGE_MAP(CGldEditorDoc, CDocument)
END_MESSAGE_MAP()


// CGldEditorDoc construction/destruction

CGldEditorDoc::CGldEditorDoc()
{
	output_set_stdout(view_stdout);
	output_set_stderr(view_stderr);
	CurrentDocument = this;
	m_OutputGroup = NULL;

	OutputStdout(_T("GLD Editor Version 1.0"));
	OutputStdout(_T("Copyright (C) 2008, Battelle Memorial Institute"));
	OutputStdout(_T(""));
#ifdef _DEBUG
	OutputStdout(_T("*** DEVELOPMENT VERSION ***"));
	OutputStdout(_T(""));
#else
	OutputStdout(_T("Welcome to GridLAB-D"));
	OutputStdout(_T(""));
#endif
}

CGldEditorDoc::~CGldEditorDoc()
{
}

BOOL CGldEditorDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	return TRUE;
}




// CGldEditorDoc serialization

void CGldEditorDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}


// CGldEditorDoc diagnostics

#ifdef _DEBUG
void CGldEditorDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CGldEditorDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CGldEditorDoc commands
BOOL CGldEditorDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
	CString str(lpszPathName);
	char cstr[1024];
	strcpy_s(cstr,str);

	StartGroup(_T("loadall(char *filename='%s');"), cstr);
	STATUS result = loadall(cstr);
	if (result==FAILED)
	{
		OutputStderr("FAILED: %s", strerror(errno));
		UpdateAllViews(NULL);
	}
	else
		OutputStdout("SUCCESS");
	EndGroup(result==FAILED);
	if (result==SUCCESS)
	{
		random_init();
		global_starttime = realtime_now();
		_getcwd(global_workdir,1024);
		t_setup_ranks();
	}
	return 1;
}

void CGldEditorDoc::OutputStdout(LPCTSTR format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf_s(buffer,format,ptr);
	va_end(ptr);

	m_OutputBuffer.AddTail(CString(" ")+CString(buffer));
}

void CGldEditorDoc::OutputStderr(LPCTSTR format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf_s(buffer,format,ptr);
	va_end(ptr);

	m_OutputBuffer.AddTail(CString("!")+CString(buffer));
}

void CGldEditorDoc::StartGroup(LPCTSTR format, ...)
{
	char buffer[1024];
	va_list ptr;
	va_start(ptr,format);
	vsprintf_s(buffer,format,ptr);
	va_end(ptr);

	m_OutputBuffer.AddTail(CString("+")+CString(buffer));
}

void CGldEditorDoc::EndGroup(BOOL status)
{
	CString &last = m_OutputBuffer.GetTail();
	POSITION pos = m_OutputBuffer.GetTailPosition();
	m_OutputBuffer.SetAt(pos,CString(status?"=":"-")+m_OutputBuffer.GetAt(pos).Mid(1));
}
