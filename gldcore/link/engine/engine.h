#ifndef __ENGINE_H
#define __ENGINE_H

#include "gridlabd.h"

#ifdef _WIN32
	#ifdef int64
	#undef int64 // wtypes.h uses the term int64
	#endif
	#include <winsock2.h>
	#define snprintf _snprintf
	#ifndef int64
	#define int64 __int64
	#endif
	#define socklen_t int
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#include <netdb.h>
	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#ifndef int64
	#define int64 long long
	#endif
	#define SOCKET int
#endif

// you must have gridlabd installed and ensure core is in include path

/**

Refactored for tcp support.
TCP code is not yet stable.
*/

typedef struct s_syncdata {
	unsigned int index;
	gld_property *prop;
	struct s_syncdata *next;
} SYNCDATA;

typedef enum {TCP=1, UDP} SOCKET_TYPE;

typedef struct s_socket_data{
	SOCKET_TYPE type;
	SOCKET sd;
	unsigned short port;
	unsigned long addr;
	struct sockaddr_in receiver;
	struct sockaddr_in sender;
} SOCKET_DATA;

typedef struct s_enginelink {
	SOCKET_DATA *sd;
	struct sockaddr_in receiver;
	struct sockaddr_in sender;
	char protocol[16];
	unsigned int recv_timeout;
	unsigned int cachesize;
	SYNCDATA *send;
	SYNCDATA *recv;
} ENGINELINK;

//General debug functions
unsigned long get_addr(struct sockaddr_in *data, unsigned int part=0);
unsigned short get_port(struct sockaddr_in *data);

//UDP Functions
bool setupUDPSocket(ENGINELINK *given);
bool bindUDPSocket(ENGINELINK *given);
bool acceptUDPSocket(ENGINELINK *given);
int recvUDPSocket(ENGINELINK *engine, char *buffer, int maxlen);
int sendUDPSocket(ENGINELINK *engine, char *buffer,int len);

#endif
