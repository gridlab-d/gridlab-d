// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
// Selim Ciraci

#ifndef _ABSCONNECTION_H
#define _ABSCONNECTION_H

#include <string>
#include <sstream>

using namespace std;

enum ENGINE_SOCK_TYPE{
	UDP=0,
	TCP
};

class absconnection
{
	protected:
		bool connectionOK;
		const string ip;
		const int port;
		string protocol;
	public:
		absconnection(const string &toIp,const int &toPort);
		virtual ~absconnection();
		virtual int recv(char * buffer,const int &size) =0;
		virtual int send(const char *buffer,const int &size) =0;
		virtual void close() =0;
		bool isConnecitonEstablished(){ return connectionOK; }
		virtual int connect(int retries=0) =0;
		virtual int getErrorCode() =0;
		const char *getErrorMessage();
		//factory methods
		static absconnection *getconnectionUDP(const string &toIP,const int &toPort);
		static absconnection *getconnectionTCP(const string &toIP,const int &toPort);
		static absconnection *getconnection(ENGINE_SOCK_TYPE socktype,const string &toIP,const int &toPort);
		const string getProtocolStr();
};

#endif
