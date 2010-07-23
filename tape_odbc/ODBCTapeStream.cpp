/*	$Id$
	Copyright (C) 2008 Battelle Memorial Institute

 *
 *	author: Matt Hauer, matthew.hauer@pnl.gov, 5/30/07 - ***
 */

#include "ODBCTapeStream.h"

list<tapepair *> ODBCTapeStream::tslist;
ODBCTapeStream::ODBCTapeStream(){
	dbconn=0;
	Reset();
}

ODBCTapeStream::ODBCTapeStream(char *fname, char *flags){
	dbconn=0;
	char host[256], uid[256], pwd[64], objname[1024];
	if(sscanf(fname, "%256[^;];%256[^;];%64[^;];%1024[^;]", host, uid, pwd, objname)==4){
//		printf("Four-point open: %s, ***, ***, %s\n", host, objname);
		Open(host, objname, uid, pwd, flags);
	}
	else if(sscanf(fname, "%256[^;]:%1024[^;]", host, objname)==2){
//		printf("Two-point open: %s, %s\n", host, objname);
		Open(host, objname, flags);
	}

}

ODBCTapeStream::ODBCTapeStream(char *servername, char *objectname, char *filemode){
	dbconn=0;
	Open(servername, objectname, filemode);
}

ODBCTapeStream::ODBCTapeStream(char *host, char *uid, char *pwd, char *objname, char *flags){
	dbconn=0;
	Open(host, uid, pwd, objname, flags);
}

ODBCTapeStream::~ODBCTapeStream(){
//	if(lines) delete lines;
}

//	for the gamblers. -MH
int ODBCTapeStream::Open(char *servername, char *objname, char *filemode){
	return Open(servername, objname, "", "", filemode);
}

int ODBCTapeStream::Open(char *servername, char *objname, char *uid, char *pwd, char *_mode){
	Reset();
	static char buffer[256];
	memset(buffer, 0, 256);
	SetFileMode(_mode);
	dbconn=ODBCConnMgr::GetMgr()->ConnectToHost(servername, uid, pwd);
	if(0 == dbconn){
		state=TSO_DONE;
		return 0;
	}
	strncpy(objectname, objname, 63);
	if(filemode[0]=='r'){
		sprintf(buffer, "SELECT EVENT_TIME, EVENT_VAL FROM EVENT_TABLE WHERE EVENT_OBJECT_NAME='%s' ORDER BY EVENT_LINE", objname);
		try{
			PreparedStatement *feeder=dbconn->GetConn()->prepareStatement(buffer, 1, 0);
			lines = feeder->executeQuery();
			if(lines->last()){
				line_max=lines->getRow();
				lines->first();
				state=TSO_OPEN;
				line_cur=1;
				return line_max;
			} //	else empty set
			state=TSO_DONE;
			return 0;
		} catch(SQLException& e) {
			cout << "Exception caught: "<<e.getMessage()<<endl;
		}
		return 2;
	}
	if(filemode[0]=='w' && filemode[1] != '+'){
		try{
			//	remove any existing header entry, those are pk'ed
			sprintf(buffer, "DELETE FROM HEADER_TABLE WHERE HEADER_OBJECT_NAME='%s';", objectname);
			PreparedStatement *feeder = dbconn->GetConn()->prepareStatement(buffer);
			//	remove any existing events that might collide later
			sprintf(buffer, "DELETE FROM EVENT_TABLE WHERE EVENT_OBJECT_NAME='%s';", objname);
			feeder=dbconn->GetConn()->prepareStatement(buffer);
			feeder->executeUpdate();
			state=TSO_OPEN;
			return 1;
		} catch(SQLException& e) {
			cout << "Exception caught: "<<e.getMessage()<<endl;
		}
		return 2;
	}
	//	guess we're not 'r', 'r+', or 'w'
	state=TSO_OPEN;
	return 1;
}

char *ODBCTapeStream::ReadLine(){
	static char buffer[1024];
	return ReadLine(buffer, 1024);
}

char *ODBCTapeStream::ReadLine(char *buffer, unsigned int size){
	memset(buffer, 0, size);	//	prevents returning garbage
	if(state != TSO_OPEN) return NULL;
	sprintf(buffer, "%s,%s", lines->getString(1).c_str(), lines->getString(2).c_str());//timedate,value
	++line_cur;
	if(!lines->next())
		state = TSO_DONE;
	return buffer;
}

/*	stubbed 'til we figure out how to store shapes in the DB	*/
int ODBCTapeStream::ReadShape(char *objname, float *scale){
	//	retreive lines for objectname
	//	-build query
	//	kinda like "SELECT (min, hour, day, month, wkday, value) IN (shape_table) WHERE (shape_obj_name IS $objectname) SORT (asc, shape_part_num)"
	//	-run query
	//	-parse results

	/*	nasty stuff from open_shaper here	*/

	return 1;	//	success?
}

/*	catches the abstract TS::Write(char *), even if we need to somehow format
 *	our input for the database here, rather than either writing it raw or
 *	formating it server-side.
 */
int ODBCTapeStream::Write(char *inbuffer){
	return 0;
}

//	writes an event to the object table
int ODBCTapeStream::Write(char *timestamp, char *value){
	double event_val=atof(value);		//	assuming fp num
	odbc::PreparedStatement *feeder=dbconn->GetConn()->prepareStatement("INSERT INTO OBJECT_TABLE VALUES (?, ?, ?, ?)");
	feeder->setString(1, objectname);
	feeder->setInt(2, ++line_cur);
	feeder->setString(3, timestamp);
	feeder->setString(4, value);
	int rows=feeder->executeUpdate();
	return 0;
}

//	writes a file header to the header table.
//	char*7 + float*2
void ODBCTapeStream::PrintHeader(char *timestr, char *uname, char *hostname, char *targstr,
	char *prop, char *trigger, long interval, long limit){
	PreparedStatement *feeder = dbconn->GetConn()->prepareStatement("INSERT INTO HEADER_TABLE VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);");
	feeder->setString(1, objectname);
	feeder->setString(2, timestr);
	feeder->setString(3, uname);
	feeder->setString(4, hostname);
	feeder->setString(5, targstr);
	feeder->setString(6, prop);
	feeder->setString(7, trigger);
	feeder->setLong(8, interval);
	feeder->setLong(9, limit);
	int affectedRows=feeder->executeUpdate();
}

void ODBCTapeStream::Close(){
	Reset();
}

void ODBCTapeStream::HardClose(){	//	handle dc'ed us
	//	do non-ODBCConnHandle stuff 'cus we got called from there
	line_cur=0;
	line_max=0;
//	if(lines) delete [] lines;
}

int ODBCTapeStream::Rewind(){
	line_cur=1;
	return 0;
}

void ODBCTapeStream::Reset(){
	memset(objectname, 0, 64);
	line_cur=0;
	line_max=0;
//	if(lines) delete [] lines;
	if(dbconn)
		dbconn->DisconnectTape(this);
}

//
//	STATIC METHODS
//

TapeStream *ODBCTapeStream::OpenStream(void *my, char *fname, char *flags){
	char host[256], uid[256], pwd[64], objname[1024];
	tapepair *pair=0;
	//	try host:uid:pwd:obj
	if(sscanf(fname, "%256[^:]:%256[^:]:%64[^:]:%1024[^:]", host, uid, pwd, objname)==4)
		pair=new tapepair(new ODBCTapeStream(host, uid, pwd, objname, flags), my);
	//	try raw host:obj
	else if(sscanf(fname, "%256[^:]:%1024[^:]", host, objname)==2)
		pair=new tapepair(new ODBCTapeStream(host, objname, flags), my);
	if(0==pair) return 0;	//	this shouldn't happen ~ bad fname format
	tslist.push_back(pair);
	return pair->tape;
}

void ODBCTapeStream::CloseStream(void *my){
	list<tapepair *>::iterator itr=tslist.begin();
	do{
		if((*itr)->name == my){
			(*itr)->tape->Close();
			delete (*itr)->tape;
			tslist.erase(itr);
			return;
		}
		++itr;
	} while(itr != tslist.end());
}

void ODBCTapeStream::CloseAllStream(){
	while(!tslist.empty()){
		tslist.back()->tape->Close();
		delete tslist.back()->tape;
		tslist.pop_back();
	}
}

//	end of ODBCTapeStream.cpp
