// $Id$

#include <cstdio>
#include <cstdlib>

#include "gridlabd.h"
#include "socket.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

Socket::Socket(int pf, int type, int prot)
{
	sd = ::socket(pf,type,prot);
	memset(&info,0,sizeof(info)); 
}

int Socket::init(void)
{
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		gl_error("socket library initialization failed: %s",strerror());
		return 0;	
	}
#endif
	return 1;
}

int Socket::get_saddr(char *buffer, size_t len) 
{
	static char static_buffer[16];
	if (buffer==NULL) buffer = static_buffer;
	unsigned int addr = get_laddr();
	unsigned char a = (addr>>24)&0xff;
	unsigned char b = (addr>>16)&0xff;
	unsigned char c = (addr>>8)&0xff;
	unsigned char d = (addr)&0xff;
	return snprintf(buffer,len-1,"%d.%d.%d.%d",a,b,c,d);
}

bool Socket::set_addr(char *s)
{
	int a,b,c,d;
	if ( sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)==4 )
	{
		set_addr(a<<24|b<<16|c<<8|d);
		return true;
	}
	else
		return false;
}

const char *Socket::strerror()
{
#ifdef _WIN32
	int err = WSAGetLastError();
	switch (err) {
	case WSAEINTR : return "interrupted function call";
	case WSAEACCES : return "permission denied";
	case WSAEFAULT : return "bad address";
	case WSAEMFILE : return "too many open files";
	case WSAEINVAL : return "invalid argument";
	case WSAEWOULDBLOCK : return "resource temporarily unavailable";
	case WSAEINPROGRESS : return "operation now in progress";
	case WSAEALREADY : return "operation already in progress";
	case WSAENOTSOCK : return "socket operation on nonsocket";
	case WSAEDESTADDRREQ : return "destination address required";
	case WSAEMSGSIZE : return "message too long";
	case WSAEPROTOTYPE : return "protocol wrong type for socket";
	case WSAENOPROTOOPT : return "bad protocol option";
	case WSAEPROTONOSUPPORT : return "protocol not supported";
	case WSAESOCKTNOSUPPORT : return "socket type not supported";
	case WSAEPFNOSUPPORT : return "protocol family not supported";
	case WSAEAFNOSUPPORT : return "address family not supported";
	case WSAEADDRINUSE : return "address already in use";
	case WSAEADDRNOTAVAIL : return "cannot assign requested address";
	case WSAENETDOWN : return "network is down";
	case WSAENETUNREACH : return "network is unreachable";
	case WSAENETRESET : return "connection reset by peer";
	case WSAECONNABORTED: return "software caused connection abort";
	case WSAECONNRESET: return "connection reset by peer";
	case WSAENOBUFS : return "no buffer space available";
	case WSAEISCONN : return "socket is already connected";
	case WSAENOTCONN : return "socket is not connected";
	case WSAESHUTDOWN : return "cannot send after socket shutdown";
	case WSAETIMEDOUT : return "connection timed out";
	case WSAECONNREFUSED : return "connection refused";
	case WSAEHOSTDOWN : return "host is down";
	case WSAEHOSTUNREACH : return "no route to host";
	case WSAEPROCLIM : return "too many processes";
	case WSASYSNOTREADY : return "network subsystem is unavailable";
	case WSAVERNOTSUPPORTED : return "winsock.dll version out of range";
	case WSANOTINITIALISED : return "successful WSAStartup not yet performed";
	case WSAEDISCON : return "graceful shutdown in progress";
	case WSA_INVALID_HANDLE : return "object handle is invalid";
	case WSA_INVALID_PARAMETER : return "one or more parameters are invalid";
	case WSA_IO_INCOMPLETE : return "overlapped I/O event object not in signaled state";
	case WSA_NOT_ENOUGH_MEMORY : return "insufficient memory available";
	case WSA_OPERATION_ABORTED : return "overlapped operation aborted";
	default : return "undefined error type";
	}
#else
	return ::strerror(errno);
#endif
}
