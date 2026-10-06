#ifndef _GLD_SOCKETS_H_
#define _GLD_SOCKETS_H_

#ifdef _WIN32

#include <winsock2.h>
// #include <Ws2def.h>
#else

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define SOCKET int
#define INVALID_SOCKET (-1)
#define closesocket close
#endif

#include <errno.h>
#include <memory.h>
#include <string.h>
// #include <pthread.h>

#endif

// EOF
