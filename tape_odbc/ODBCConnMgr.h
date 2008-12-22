/*	$id$
	Copyright (C) 2008 Battelle Memorial Institute

 *	A singleton class used to track each of the databases that the
 *		ODBCTapeStream objects are connecting to.  Provides both
 *		dictionary and observer functionality for ODBC connections.
 *	The dictionary ops allow re-use of an existing connection.
 *	The observer routines will hypothetically clean the objects and
 *		gracefully kill the app if a database connection is lost when
 *		I/O is needed.
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 6/4/07 - ***
 */

#ifndef _ODBCCONNMGR_H_
#define _ODBCCONNMGR_H_

#include <list>

#include "ODBCConnHandle.h"

using namespace std;

class ODBCConnHandle;

class ODBCConnMgr{
public:
	~ODBCConnMgr();

	ODBCConnHandle *ConnectToHost(char *, char *, char *);
	int DisconnectFromHost(char *);
	static ODBCConnMgr *GetMgr();
private:
	ODBCConnMgr();

	list<ODBCConnHandle *>	handles;
	static ODBCConnMgr *mgr;
};

#endif
