/* $Id: http_client.h 4738 2014-07-03 00:55:39Z dchassin $
 */

#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

#ifdef _WIN32
#ifdef int64
#undef int64 // wtypes.h uses the term int64
#endif
	#include <winsock2.h>
#ifndef int64
#define int64 __int64
#endif
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#include <netdb.h>
	#define SOCKET int
	#define INVALID_SOCKET (-1)
#endif

typedef struct s_http {
	SOCKET sd;
	size_t len;
	size_t pos;
	char *buf;
} HTTP;
typedef struct s_http_buffer
{
	char *data;
	int size;
} HTTPBUFFER;
typedef struct s_http_result
{
	HTTPBUFFER header;
	HTTPBUFFER body;
	int status;
} HTTPRESULT;

#ifdef __cplusplus
extern "C" {
#endif

HTTP* hopen(const char *url, int maxlen);
int hclose(HTTP*http);
size_t hread(char *buffer, size_t size, HTTP* http);

void *http_read(const char *url, int maxlen);
void http_delete_result(void *result);
HTTPRESULT *http_new_result(void);
int http_saveas(char *url, char *file);

#ifdef __cplusplus
}
#endif

#endif
