#ifndef _GLD_SOCKETS_H_
#define _GLD_SOCKETS_H_

#ifdef WIN32

#include <winsock2.h>
//#include <Ws2def.h>
#else

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/errno.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include <memory.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#endif

// EOF
