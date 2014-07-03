// GldEditorDoc.h : interface of the CGldEditorDoc class
//

#pragma once

class CGldEditorDoc : public CDocument
{
protected: // create from serialization only
	CGldEditorDoc();
	DECLARE_DYNCREATE(CGldEditorDoc)

// Attributes
public:
	CStringList m_OutputBuffer;
	POSITION m_OutputGroup;

// Operations
public:
	void OutputStdout(LPCTSTR format, ...);
	void OutputStderr(LPCTSTR format, ...);
	void StartGroup(LPCTSTR format, ...);
	void EndGroup(BOOL status);

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);

// Implementation
public:
	virtual ~CGldEditorDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};


