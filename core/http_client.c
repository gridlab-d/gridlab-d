/* $Id: http_client.c 4738 2014-07-03 00:55:39Z dchassin $
 */

#include <stdio.h>


#include "output.h"
#include "http_client.h"
#include "server.h"


HTTP* hopen(char *url, int maxlen)
{
	struct sockaddr_in serv_addr;
	struct hostent *server;
	char protocol[64];
	char hostname[1024];
	char filespec[1024];
	HTTP *http;
	char request[1024];
	int len;

#ifdef WIN32
	/* init sockets */
	WORD wsaVersion = MAKEWORD(2,2);
	WSADATA wsaData;
	if ( WSAStartup(wsaVersion,&wsaData)!=0 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): unable to initialize Windows sockets", url, maxlen);
		return NULL;
	}
#endif

	/* extract URL parts */
	if ( sscanf(url,"%63[^:]://%1023[^/]%1023s", protocol,hostname, filespec)!=3 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): badly formed URL", url, maxlen);
		return NULL;
	}

	/* setup handle */
	http = (HTTP*)malloc(sizeof(HTTP));
	if ( http==NULL )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): memory allocation failure", url, maxlen);
		return NULL;
	}

	/* initialize http */
	http->buf = NULL;
	http->len = 0;
	http->pos = 0;

	/* setup socket */
	http->sd = socket(AF_INET, SOCK_STREAM, 0);
	if ( http->sd < 0 )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): unable to create socket", url, maxlen);
		goto Error;
	}

	/* find server */
	server = gethostbyname(hostname);
	if ( server==NULL )
	{
		output_error("hopen(char *url='%s', int maxlen=%d): server not found", url, maxlen);
		goto Error;
	}

	/* extract server address */
	memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	memcpy((char*)&serv_addr.sin_addr.s_addr,(char*)server->h_addr,server->h_length);
	serv_addr.sin_port = htons(80);

	/* open connection */
	if ( connect(http->sd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0 )
	{
		output_error("hopen(char *url='%s', maxlen=%d): unable to connect to server", url, maxlen);
		goto Error;
	}

	/* format/send request */
	len = sprintf(request,"GET %s HTTP/1.1\r\nHost: %s:80\r\nUser-Agent: GridLAB-D/%d.%d\r\nConnection: close\r\n\r\n",filespec,hostname,REV_MAJOR,REV_MINOR);
	output_debug("sending HTTP message \n%s", request);
	if ( send(http->sd,request,len,0)<len )
	{
		output_error("hopen(char *url='%s', maxlen=%d): unable to send request", url, maxlen);
		goto Error;
	}

	/* read response header */
	http->buf = (char*)malloc(maxlen+1);
	http->len = 0;
	do {
		len = recv(http->sd,http->buf+http->len,maxlen-http->len,0x00);
		if ( len==0 )
		{
			output_debug("hopen(char *url='%s', maxlen=%d): connection closed", url, maxlen);
		}
		else if ( len<0 )
		{
			output_error("hopen(char *url='%s', maxlen=%d): unable to read response", url, maxlen);
			goto Error;
		}
		else
		{
			http->len += len;
			http->buf[http->len] = '\0';
		}
	} while ( len>0 );

	if ( http->len>0 )
		output_debug("received HTTP message \n%s", http->buf);
	else
		output_debug("no HTTP response received");
	return http;
Error:
#ifdef WIN32
		output_error("hopen(char *url='%s', int maxlen=%d): WSA error code %d", url, maxlen, WSAGetLastError());
#endif
	return NULL;
}
int hclose(HTTP*http)
{
	if ( http )
	{
		if ( http->sd ) 
#ifdef WIN32
			closesocket(http->sd);
#else
			close(http->sd);
#endif
		if ( http->buf!=NULL ) free(http->buf);
		free(http);
	}
	return 1;
}
size_t hfilelength(HTTP*http)
{
	return http->len;
}
int heof(HTTP*http)
{
	return http->pos>=http->len;
}
size_t hread(char *buffer, size_t size, HTTP* http)
{
	size_t len = http->len - http->pos;
	if ( http->pos >= http->len )
		return 0;
	if ( len > size )
		len = size;
	memcpy(buffer,http->buf+http->pos,len);
	http->pos += len;
	return len;
}


/* URL access */
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

HTTPRESULT *http_read(char *url, int maxlen)
{
	HTTPRESULT *result = http_new_result();
	
	if ( strncmp(url,"http://",7)==0 )
	{
		HTTP *fp = hopen(url,maxlen);
		if ( fp==NULL )
		{
			output_error("http_read('%s'): unable to access url", url);
			result->status = errno;
		}
		else
		{
			size_t len = (int)hfilelength(fp);
			size_t hlen = len;
			char *buffer = (char*)malloc(len+1);
			char *data = NULL;
			if ( hread(buffer,len,fp)<=0 )
			{
				output_error("http_read('%s'): unable to read file", url);
				result->status = errno;
			}
			else 
			{
				int dlen;
				buffer[len]='\0';
				data = strstr(buffer,"\r\n\r\n");
				if ( data!=NULL )
				{
					hlen = data - buffer;
					*data++ = '\0';
					result->body.size = len - hlen - 1;
					while ( isspace(*data) ) { data++; result->body.size--; }
//					if ( sscanf(data,"%x",&dlen)==1 )
//					{
//						data = strchr(data,'\n');
//						while ( isspace(*data) ) {data++;}
//						result->body.size = dlen;
//						result->body.data = malloc(dlen+1);
//					}
//					else
//					{
						result->body.data = malloc(result->body.size+1);
//					}
					memcpy(result->body.data,data,result->body.size);
					result->body.data[result->body.size] = '\0';
				}
				else
				{
					result->body.data = "";
					result->body.size = 0;
				}
				result->header.size = hlen;
				result->header.data = malloc(hlen+1);
				strcpy(result->header.data,buffer);
				result->status = 0;
			}
		}
		hclose(fp);
	}

	else if ( strncmp(url,"file://",7)==0 )
	{
		FILE *fp = fopen(url+7,"rt");
		if ( fp==NULL )
		{
			output_error("http_read('%s'): unable to access file", url);
			result->status = errno;
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
		fclose(fp);
	}
	return result;
}
