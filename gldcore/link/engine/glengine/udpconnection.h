// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
// DP Chassin

#ifndef _UDPCONNECTION_H
#define _UDPCONNECTION_H

#include "absconnection.h"

#ifdef _WIN32
	#ifdef int64
	#undef int64 // wtypes.h uses the term int64
	#endif
	#include <winsock2.h>
	#define socklen_t int
	#define snprintf _snprintf
	#ifndef int64
	#define int64 _int64
	#endif
	#define int64_t _int64
	typedef UINT_PTR SOCKET;
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#include <sys/wait.h>
	#include <netdb.h>
	#include <unistd.h>
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#define int64 long long
	#define SOCKET int
	#define vsprintf_s vsnprintf
	#define sprintf_s snprintf
#endif

class udpconnection :
	public absconnection
{	
	private:
#ifdef _WIN32
		SOCKET sd;
#else
		int sd;
#endif
		struct sockaddr_in togld;
	public:
		//supports connection to remote hosts
		udpconnection(const string &ip,const int &port);
		~udpconnection(void);
		virtual int recv(char * buffer,const int &size);
		virtual int send(const char *buffer,const int &size);
		virtual void close();
		virtual int getErrorCode();
		virtual int connect(int retries=0);
};

#endif
