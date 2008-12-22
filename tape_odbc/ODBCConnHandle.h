/*	$id$
	Copyright (C) 2008 Battelle Memorial Institute

 *	A container that stores useful information around an odbc::Connection.
 *		Allows for fun things like string names and observers.
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 6/4/07 - ***
 */

#ifndef _ODBCCONNHANDLE_H_
#define _ODBCCONNHANDLE_H_

#include <list>
#include <string.h>

#include <odbc++/connection.h>
#include <odbc++/drivermanager.h>

#include "ODBCTapeStream.h"

using std::list;

class ODBCTapeStream;

class ODBCConnHandle{
public:
	ODBCConnHandle();
	ODBCConnHandle(Connection *, char *);
	~ODBCConnHandle();

	void Reset();
	int CheckName(char *);
	void DisconnectTape(ODBCTapeStream *);
	void Disconnect();
	int RegisterStream(ODBCTapeStream *);

	odbc::Connection *GetConn(){return conn;}
private:
	odbc::Connection *conn;
	char	servername[128];
	int		tapect;
	list<ODBCTapeStream *> tapelist;
};

#endif
