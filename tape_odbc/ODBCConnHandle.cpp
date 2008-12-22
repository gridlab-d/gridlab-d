/*	$id$
	Copyright (C) 2008 Battelle Memorial Institute

 *
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 6/4/07 - ***
 */

#include "ODBCConnHandle.h"

ODBCConnHandle::ODBCConnHandle(){
	Reset();
}

ODBCConnHandle::ODBCConnHandle(Connection *inconn, char *hostname){
	Reset();
	conn=inconn;
	strncpy(servername, hostname, 128);
}

ODBCConnHandle::~ODBCConnHandle(){
	Disconnect();	//	clears list
	delete conn;
}

void ODBCConnHandle::Reset(){
	tapect=0;
	conn=0;
	if(!tapelist.empty()){
		printf("WARNING:\tODBCConnHandle::Reset: we're reseting a non-empty handle?\n");
	}
	tapelist.clear();
	memset(servername, 0, 128);
}

int ODBCConnHandle::CheckName(char *hostname){
	return strcmp(servername, hostname);
}

void ODBCConnHandle::DisconnectTape(ODBCTapeStream *ts){
	if(tapelist.empty()) return;	//	nothing to remove
	tapelist.remove(ts);
	int tapelistsize=(int)(tapelist.size());
	if(tapelistsize > tapect)
		printf("WARNING:\tODBCConnHandle::DisconnectTape: we disconnected a tape and ended up with more tapes than we started with?\n");
	tapect=tapelistsize;			//	shouldn't assume single instence removed
}

void ODBCConnHandle::Disconnect(){	//	as in, everything
	while(!tapelist.empty()){
		tapelist.front()->HardClose();
		tapelist.pop_front();
	}
}

int ODBCConnHandle::RegisterStream(ODBCTapeStream *ts){
	tapelist.push_back(ts);
	++tapect;
	return 1;	//	success
}

//	end of ODBCConnHandle.cpp
