/*	$id$
	Copyright (C) 2008 Battelle Memorial Institute
 *	Header for ODBC-based tape streams.
 *	Should be "interesting".
 *
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 5/30/07 - ***
 */

#ifndef _CODBCTAPESTREAM_H
#define _CODBCTAPESTREAM_H

#include <list>

#include <stdio.h>
#include <string.h>
#include <iostream>

#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>

using namespace odbc;

#include "ODBCConnHandle.h"
#include "ODBCConnMgr.h"
#include "TapeStream.h"

using namespace std;

class ODBCConnHandle;

class ODBCTapeStream : public TapeStream{
public:
 	ODBCTapeStream();
	ODBCTapeStream(char *, char *);
 	ODBCTapeStream(char *, char *, char *);
 	ODBCTapeStream(char *, char *, char *, char *, char *);
 	virtual ~ODBCTapeStream();

 	int Open(char *, char *, char *);
	int Open(char *, char *, char *, char *, char *);

 //	virtual char *Read();
 	virtual char *ReadLine(char *, unsigned int);
 	virtual char *ReadLine();
 	virtual int ReadShape(char *, float *);
 	virtual int Write(char*);
 	virtual int Write(char*, char*);
 	//int Write(int, OBJECT *);
 	virtual void Close();
 	void HardClose();
 	virtual int Rewind();
	void Reset();

	virtual void PrintHeader(char *, char *, char *, char *, char *, char *, long, long);

	static TapeStream *OpenStream(void *, char *, char *);
	static void CloseStream(void *);
	static void CloseAllStream();
protected:
	int					line_cur, line_max;
	odbc::ResultSet		*lines;
	ODBCConnHandle *	dbconn;
	char				objectname[64];
	static list<tapepair *>	tslist;
};

#endif
