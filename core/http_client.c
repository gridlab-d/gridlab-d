/* $Id$
 */

#include <stdio.h>

#ifdef WIN32
	#include <winsock2.h>
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#define SOCKET int
	#define INVALID_SOCKET (-1)
#endif

#include "output.h"
#include "http_client.h"
#include "server.h"

HTTPRESULT *http_new_result(void)
{
	HTTPRESULT *result = malloc(sizeof(HTTPRESULT));
	result->body.data = NULL;
	result->body.size = 0;
	result->header.data = NULL;
	result->header.size = 0;
	result->status = 0;
	return result;
}

void http_delete_result(HTTPRESULT *result)
{
	if ( result->body.size>0 )
		free(result->body.data);
	if ( result->header.size>0 )
		free(result->header.data);
	free(result);
}

HTTPRESULT *http_read(char *url)
{
	HTTPRESULT *result = http_new_result();
	
	if ( strncmp(url,"http://",7)==0 )
	{
		output_warning("http_read('%s'): http access not implemented", url);
	}

	else if ( strncmp(url,"file://",7)==0 )
	{
		FILE *fp = fopen(url+7,"rt");
		if ( fp==NULL )
		{
			output_error("http_read('%s'): unable to access file", url);
		}
		else
		{
			result->body.size = filelength(fileno(fp))+1;
			result->body.data = malloc(result->body.size);
			memset(result->body.data,0,result->body.size);
			if ( fread(result->body.data,1,result->body.size,fp)<=0 )
			{
				output_error("http_read('%s'): unable to read file", url);
				result->status = errno;
			}
			else
				result->status = 0;
		}
	}
	return result;
}
