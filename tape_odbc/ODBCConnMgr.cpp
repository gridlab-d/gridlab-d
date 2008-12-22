/*	$Id$
	Copyright (C) 2008 Battelle Memorial Institute

 *
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 6/4/07 - ***
 */


#include "ODBCConnMgr.h"

ODBCConnMgr *ODBCConnMgr::mgr=0;

ODBCConnMgr::ODBCConnMgr(){
	mgr=this;
	handles.clear();
}

ODBCConnMgr::~ODBCConnMgr(){
	mgr=0;
	while(!handles.empty()){
		delete handles.front();
		handles.pop_front();
	}
}

ODBCConnMgr *ODBCConnMgr::GetMgr(){
	if(mgr == 0) return new ODBCConnMgr();
	return mgr;
}

ODBCConnHandle *ODBCConnMgr::ConnectToHost(char *hostname, char *uid, char *pwd){
	if(!handles.empty()){
		list<ODBCConnHandle *>::iterator itr=handles.begin();
		do{
			if(0 == (*itr)->CheckName(hostname)) return *itr;
		} while(itr!=handles.end());
	}
	//	guess we aren't connected to that host.
//	then again, let's give it a shot.  -MH
//	if((uid == null) || (pwd == null)) return null;	//	no uid or pwd, auto-fail.
	odbc::Connection *conn=odbc::DriverManager::getConnection(hostname, uid, pwd);
	if(conn != 0){
		handles.push_back(new ODBCConnHandle(conn, hostname));
	} else {
		printf("WARNING:\tODBCConnMgr::ConnectToHost: unable to connect to host %s with uid %s!\n", hostname, uid);
		return 0;
	}
	return handles.back();
}

int ODBCConnMgr::DisconnectFromHost(char *hostname){
	if(!handles.empty()){
		list<ODBCConnHandle *>::iterator itr=handles.begin();
		do{
			if(0 == (*itr)->CheckName(hostname)){
				(*itr)->Disconnect();
				handles.erase(itr);
				return 1;
			}
		} while(itr!=handles.end());
	}
	printf("WARNING:\tODBCConnMgr::DisconnectFromHost(%s): we're not connected to that host!\n", hostname);
	return 0;
}

//	end of ODBCConnMgr.cpp
