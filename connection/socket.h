// $Id$
// @file socket.h
// @addtogroup connection
// @{

#ifndef _SOCKET_H
#define _SOCKET_H

#ifdef _WIN32

// conflict for use of int64 in winsock2.h
#ifdef int64
#undef int64
#endif

#include <winsock2.h>

#define int64 __int64 /**< Win32 version of 64-bit integers */
typedef int socklen_t;

#else // WIN32

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/errno.h>
#define SOCKET int
#define INVALID_SOCKET (~0)
#define SOCKET_ERROR (-1)

#endif // WIN32

class Socket {
public:
	static int init(void);
private:
	SOCKET sd;
	struct sockaddr_in info;
public:
	inline Socket(void) { sd=0; memset(&info,0,sizeof(info));};
	Socket(int pf, int type, int prot);
	inline ~Socket() {
		if ( sd!=0 )
#ifdef _WIN32
			closesocket(sd);
#else
			close(sd);
#endif
	};
public:
	inline int get_lasterror(void) {
#ifdef _WIN32
		return GetLastError();
#else
		return errno;
#endif
	};
public:
	inline SOCKET get_socket(void) { return sd; };
	struct sockaddr_in *get_info(void) { return &info; };
	int get_infosize(void) { return sizeof(info); };
	inline int get_port(void) { return ntohs(info.sin_port); };
	int get_saddr(char *buffer=NULL, size_t len=0);
	unsigned long get_laddr(void) { return ntohl(info.sin_addr.s_addr); };
public:
	inline void set_family(int a=0) { info.sin_family=a; };
	inline void set_addr(unsigned long a) { info.sin_addr.s_addr=htonl(a); };
	bool set_addr(char*s);
	inline void set_port(int t=0) { info.sin_port = t; };
public:
	inline int bind() { return ::bind(sd,(struct sockaddr*)&info,sizeof(info)); };
	inline int listen(int backlog) { return ::listen(sd,backlog); }; 
	inline bool accept(Socket &svr) { int len=get_infosize(); sd=::accept(svr.get_socket(),(struct sockaddr*)get_info(),(socklen_t*)&len); return sd!=INVALID_SOCKET; };
public:
	static const char *strerror(void);
};

#endif
