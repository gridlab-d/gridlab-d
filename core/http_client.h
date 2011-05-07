/* $Id$
 */

#ifndef _HTTPCLIENT_H
#define _HTTPCLIENT_H

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

HTTPRESULT *http_read(char *url); 
void http_delete_result(HTTPRESULT *result);
HTTPRESULT *http_new_result(void);

#endif
