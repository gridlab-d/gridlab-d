/* $Id$ 
 * server.c
 */

#include <stdio.h>
#include <stdlib.h>

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

#include <memory.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

#define MAXSTR		1024		// maximum string length

static int shutdown_server = 0; /* flag to stop accepting incoming messages */
SOCKET sockfd = (SOCKET)0;

static void shutdown_now(void)
{
	extern int stop_now;
	stop_now = 1;
	shutdown_server = 1;
	if (sockfd!=(SOCKET)0)
#ifdef WIN32
		shutdown(sockfd,SD_BOTH);
#else
		shutdown(sockfd,SHUT_RDWR);
#endif
	sockfd = (SOCKET)0;
}

#ifndef WIN32
int GetLastError()
{
	return errno;
}
#endif

void server_request(int);	// Function to handle clients' request(s)
void http_response(SOCKET);

#include "server.h"
#include "output.h"
#include "globals.h"
#include "object.h"

#include "legal.h"

static size_t send_data(SOCKET s, char *buffer, size_t len)
{
#ifdef WIN32
	return (size_t)send(s,buffer,len,0);
#else
	return (size_t)write(s,buffer,len);
#endif	
}

static size_t recv_data(SOCKET s,char *buffer, size_t len)
{
#ifdef WIN32
	return (size_t)recv(s,buffer,len,0);
#else
	return (size_t)read(s,(void *)buffer,len);
#endif
}

static void *server_routine(void *arg)
{
	static int status = 0;
	sockfd = (SOCKET)arg;
	// repeat forever..
	while (!shutdown_server)
	{
		struct sockaddr_in cli_addr;
		SOCKET newsockfd;

		int clilen = sizeof(cli_addr);

		/* accept client request and get client address */
		newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
		if ((int)newsockfd<0 && errno!=EINTR)
		{
			status = GetLastError();
			output_error("server accept error on fd=%d: code %d", sockfd, status);
			goto Done;
		}
		else if ((int)newsockfd > 0)
		{
#ifdef WIN32
			output_verbose("accepting incoming connection from %d.%d.%d.%d on port %d", cli_addr.sin_addr.S_un.S_un_b.s_b1,cli_addr.sin_addr.S_un.S_un_b.s_b2,cli_addr.sin_addr.S_un.S_un_b.s_b3,cli_addr.sin_addr.S_un.S_un_b.s_b4,cli_addr.sin_port);
#else
			output_verbose("accepting incoming connection from on port %d",cli_addr.sin_port);
#endif
			http_response(newsockfd);
			if (global_server_quit_on_close)
				shutdown_now();
		}
#ifdef NEVER
		{
			handleRequest(newsockfd);
#ifdef WIN32
			closesocket(newsockfd);
#else
			close(newsockfd);
#endif
		}
#endif
	}
	output_verbose("server shutdown");
Done:
	return (void*)&status;
}

STATUS server_startup(int argc, char *argv[])
{
	int portNumber = global_server_portnum;
	SOCKET sockfd;
	struct sockaddr_in serv_addr;
	pthread_t thread;

#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		output_error("socket library initialization failed");
		return FAILED;	
	}
#endif

	/* create a new socket */
	if ((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
	{
		output_error("can't create stream socket: %s",GetLastError());
		return FAILED;
	}
	atexit(shutdown_now);

	memset(&serv_addr,0,sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNumber);

	/* bind socket to server address */
	if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
#ifdef WIN32
		output_error("can't bind to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
		output_error("can't bind address");
#endif
		return FAILED;
	}
#ifdef WIN32
	output_verbose("bind ok to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
	output_verbose("bind ok to address");
#endif	
	/* listen for connection */
	listen(sockfd,5);
	output_verbose("server listening to port %d", portNumber);

	/* start the server thread */
	if (pthread_create(&thread,NULL,server_routine,(void*)sockfd))
	{
		output_error("server thread startup failed");
		return FAILED;
	}

	return SUCCESS;
}

/********************************************************
 HTTP routines
 */

typedef struct s_http {
	char query[1024];
	char *buffer;
	size_t len;
	size_t max;
	char *status;
	char *type;
	SOCKET s;
} HTTP;

static HTTP *http_create(SOCKET s)
{
	HTTP *http = (HTTP*)malloc(sizeof(HTTP));
	memset(http,0,sizeof(HTTP));
	http->s = s;
	http->max = 4096;
	http->buffer = malloc(http->max);
	return http;
}

#define HTTP_CONTINUE "100 Continue"
#define HTTP_SWITCHPROTOCOL "101 Switching Protocols"

#define HTTP_OK "200 Ok"
#define HTTP_CREATED "201 Created"
#define HTTP_ACCEPTED "202 Accepted"
#define HTTP_NONAUTHORITATIVEINFORMATION "203 Non-Authoritative Information"
#define HTTP_NOCONTENT "204 No Content"
#define HTTP_RESETCONTENT "205 Reset Content"
#define HTTP_PARTIALCONTENT "206 Partial Content"

#define HTTP_MULTIPLECHOICES "300 Multiple Choices"
#define HTTP_MOVEDPERMANENTLY "301 Moved Permanently"
#define HTTP_FOUND "302 Found"
#define HTTP_SEEOTHER "303 See Other"
#define HTTP_NOTMODIFIED "304 Not Modified"
#define HTTP_USEPROXY "305 Use Proxy"
#define HTTP_TEMPORARYREDIRECT "307 Temporary Redirect"

#define HTTP_BADREQUEST "400 Bad Request"
#define HTTP_UNAUTHORIZED "401 Unauthorized"
#define HTTP_PAYMENTREQUIRED "402 Payment Required"
#define HTTP_FORBIDDEN "403 Forbidden"
#define HTTP_NOTFOUND "404 Not Found"
#define HTTP_METHODNOTALLOWED "405 Method Not Allowed"
#define HTTP_NOTACCEPTABLE "406 Not Acceptable"
#define HTTP_PROXYAUTHENTICATIONREQUIRED "407 Proxy Authentication Required"
#define HTTP_REQUESTTIMEOUT "408 Request Time-out"
#define HTTP_CONFLICT "409 Conflict"
#define HTTP_GONE "410 Gone"
#define HTTP_LENGTHREQUIRED "411 Length Required"
#define HTTP_PRECONDITIONFAILED "412 Precondition Failed"
#define HTTP_REQUESTENTITYTOOLARGE "413 Request Entity Too Large"
#define HTTP_REQUESTURITOOLARGE "414 Request-URI Too Large"
#define HTTP_UNSUPPORTEDMEDIATYPE "415 Unsupported Media Type"
#define HTTP_REQUESTEDRANGENOTSATISFIABLE "416 Requested range not satisfiable"
#define HTTP_EXPECTATIONFAILED "417 Expectation Failed"
#define HTTP_INTERNALSERVERERROR "500 Internal Server Error"
#define HTTP_NOTIMPLEMENTED "501 Not Implemented"
#define HTTP_BADGATEWAY "502 Bad Gateway"
#define HTTP_SERVICEUNAVAILABLE "503 Service Unavailable"
#define HTTP_GATEWAYTIMEOUT "504 Gateway Time-out"
#define HTTP_VERSIONNOTSUPPORTED "505 HTTP Version not supported"

static void http_status(HTTP *http, char *status)
{
	http->status = status;
}
static void http_type(HTTP *http, char *type)
{
	http->type = type;
}
static void http_send(HTTP *http)
{
	char header[4096];
	int len=0;
	len += sprintf(header+len, "HTTP/1.1 %s", http->status?http->status:HTTP_INTERNALSERVERERROR);
	output_verbose("%s (len=%d)",header,http->len);
	len += sprintf(header+len, "\nContent-Length: %d\n", http->len);
	if (http->type)
		len += sprintf(header+len, "Content-Type: %s\n", http->type);
	len += sprintf(header+len, "Cache-Control: no-cache\n");
	len += sprintf(header+len, "Cache-Control: no-store\n");
	len += sprintf(header+len,"\n");
	send_data(http->s,header,len);
	if (http->len>0)
		send_data(http->s,http->buffer,http->len);
	http->len = 0;
}
static void http_write(HTTP *http, char *data, int len)
{
	if (http->len+len>=http->max)
	{
		/* extend buffer */
		void *old = http->buffer;
		http->max *= 2;
		http->buffer = malloc(http->max);
		memcpy(http->buffer,old,http->len);
		free(old);
	}
	memcpy(http->buffer+http->len,data,len);
	http->len += len;
}
static void http_close(HTTP *http)
{
	if (http->len>0)
		http_send(http);
#ifdef WIN32
	closesocket(http->s);
#else
	close(http->s);
#endif
}

static void http_mime(HTTP *http, char *path)
{
	size_t len = strlen(path);
	if (strcmp(path+len-4,".png")==0)
		http->type = "image/png";
	else
		http->type = NULL;
}

static int http_format(HTTP *http, char *format, ...)
{
	int len;
	char data[65536];
	va_list ptr;

	va_start(ptr,format);
	len = vsprintf(data,format,ptr);
	va_end(ptr);

	http_write(http,data,len);

	return len;
}

char *http_unquote(char *buffer)
{
	char *eob = buffer+strlen(buffer)-1;
	if (buffer[0]=='"') buffer++;
	if (*eob=='"') *eob='\0';
	return buffer;
}

static char hex(char c)
{
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		return c-'0';
	case 'A':
	case 'B':
	case 'C':
	case 'D':
	case 'E':
	case 'F':
		return c-'A'+10;
	case 'a':
	case 'b':
	case 'c':
	case 'd':
	case 'e':
	case 'f':
		return c-'a'+10;
	default:
		return 0;
	}
}
void http_decode(char *buffer)
{
	char result[1024], *in, *out = result;
	for ( in=buffer ; *in!='\0' ; in++ )
	{
		if (*in=='%')
		{
			char hi = *++in;
			char lo = *++in;
			*out++ = hex(hi)*16 + hex(lo);			
		}
		else
			*out++ = *in;
	}
	*out='\0';
	strcpy(buffer,result);
}

int http_xml_request(HTTP *http,char *uri)
{
	char arg1[1024]="", arg2[1024]="";
	int nargs = sscanf(uri,"%1023[^/=\r\n]/%1023[^\r\n=]",arg1,arg2);
	char *value = strchr(uri,'=');
	char buffer[1024]="";
	OBJECT *obj=NULL;
	char *id;

	/* value */
	if (value) *value++;

	/* decode %.. */
	http_decode(arg1);
	http_decode(arg2);
	if (value) http_decode(value);

	/* process request */
	switch (nargs) {

	/* get global variable */
	case 1: 

		/* find the variable */
		if (global_getvar(arg1,buffer,sizeof(buffer))==NULL)
		{
			output_error("global variable '%s' not found", arg1);
			return 0;
		}

		/* assignment, if any */
		if (value) global_setvar(arg1,value);
		
		/* post the response */
		http_format(http,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
		http_format(http,"<globalvar>\n\t<name>%s</name>\n\t<value>%s</value>\n</globalvar>\n",
			arg1, http_unquote(buffer));
		http_type(http,"text/xml");
		return 1;

	/* get object property */
	case 2:

		/* find the object */
		id = strchr(arg1,':');
		if ( id==NULL )
			obj = object_find_name(arg1);
		else
			obj = object_find_by_id(atoi(id+1));
		if ( obj==NULL )
		{
			output_error("object '%s' not found", arg1);
			return 0;
		}

		/* post the current value */
		if ( !object_get_value_by_name(obj,arg2,buffer,sizeof(buffer)) )
		{
			output_error("object '%s' property '%s' not found", arg1, arg2);
			return 0;
		}

		/* assignment, if any */
		if ( value && !object_set_value_by_name(obj,arg2,value) )
		{
			output_error("cannot set object '%s' property '%s' to '%s'", arg1, arg2, value);
			return 0;
		}

		/* post the response */
		http_format(http,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n<property>\n");
		http_format(http,"\t<object>%s</object>\n", arg1);
		http_format(http,"\t<name>%s</name>\n", arg2);
		http_format(http,"\t<value>%s</value>\n", http_unquote(buffer));
		/* TODO add property type info */
		http_format(http,"</property>\n");
		http_type(http,"text/xml");
		return 1;

	default:
		return 0;
	}
	return 0;
}

int http_gui_request(HTTP *http,char *uri)
{
	gui_set_html_stream((void*)http,http_format);
	if (gui_html_output_page(uri)>=0)
	{
		http_type(http,"text/html");
		return 1;
	}
	else 
		return 0;
}

#ifndef WIN32
#include <sys/stat.h>
static int filelength(int fd)
{
	struct stat fs;
	if (fstat(fd,&fs))
		return 0;
	else
		return fs.st_size;
}
#endif

int http_output_request(HTTP *http,char *uri)
{
	char fullpath[1024];
	FILE *fp;
	char *buffer;
	int len;
	strcpy(fullpath,global_workdir);
	if (*(fullpath+strlen(fullpath)-1)!='/' || *(fullpath+strlen(fullpath)-1)!='\\' )
		strcat(fullpath,"/");
	strcat(fullpath,uri);
	fp = fopen(fullpath,"rb");
	if ( fp==NULL )
	{
		output_error("file '%s' not found", fullpath);
		return 0;
	}
	len = filelength(fileno(fp));
	if (len<=0)
	{
		output_error("file '%s' not accessible", fullpath);
		return 0;
	}
	buffer = (char*)malloc(len);
	if (buffer==NULL)
	{
		output_error("file buffer for '%s' not available", fullpath);
		return 0;
	}
	if (fread(buffer,1,len,fp)<=0)
	{
		output_error("file '%s' read failed", fullpath);
		return 0;
	}
	http_write(http,buffer,len);
	http_mime(http,uri);
	fclose(fp);
	return 1;
}

int http_action_request(HTTP *http,char *action)
{
	if (strcmp(action,"quit")==0)
	{
		http_status(http,HTTP_ACCEPTED);
		http_type(http,"text/plain");
		http_format(http,"Goodbye");
		http_send(http);
		http_close(http);
		shutdown_now();
		return 1;
	}
	else
		return 0;
}

int http_favicon(http)
{
	FILE *fp;
	char *fullpath = find_file("favicon.ico",NULL,4);
	char *buffer;
	int len;

	if ( fullpath==NULL )
	{
		output_error("file 'favicon.ico' not found", fullpath);
		return 0;
	}
	fp = fopen(fullpath,"rb");
	if ( fp==NULL )
	{
		output_error("file '%s' open failed", fullpath);
		return 0;
	}
	len = filelength(fileno(fp));
	if (len<=0)
	{
		output_error("file '%s' not accessible", fullpath);
		return 0;
	}
	buffer = (char*)malloc(len);
	if (buffer==NULL)
	{
		output_error("file buffer for '%s' not available", fullpath);
		return 0;
	}
	if (fread(buffer,1,len,fp)<=0)
	{
		output_error("file '%s' read failed", fullpath);
		return 0;
	}
	http_write(http,buffer,len);
	http_mime(http,"image/vnd.microsoft.icon");
	fclose(fp);	return 0;
}

void http_response(SOCKET fd)
{
	HTTP *http = http_create(fd);
	size_t len;
	int content_length = 0;
	char *user_agent = NULL;
	char *host = NULL;
	int keep_alive = 0;
	char *connection = NULL;
	char *accept = NULL;
	struct s_map {
		char *name;
		enum {INTEGER,STRING} type;
		void *value;
		int sz;
	} map[] = {
		{"Content-Length", INTEGER, (void*)&content_length, 0},
		{"Host", STRING, (void*)&host, 0},
		{"Keep-Alive", INTEGER, (void*)&keep_alive, 0},
		{"Connection", STRING, (void*)&connection, 0},
		{"Accept", STRING, (void*)&accept, 0},
	};

	while ( (int)(len=recv_data(fd,http->query,sizeof(http->query)))>0 )
	{
		/* first term is always the request */
		char *request = http->query;
		char method[32];
		char uri[1024];
		char version[32];
		char *p = strchr(http->query,'\r');
		int v;

		/* read the request string */
		if (sscanf(request,"%s %s %s",method,uri,version)!=3)
		{
			http_status(http,HTTP_BADREQUEST);
			http_format(http,HTTP_BADREQUEST);
			http_type(http,"text/html");
			http_send(http);
			break;
		}

		/* read the rest of the header */
		while (p!=NULL && (p=strchr(p,'\r'))!=NULL) 
		{
 			*p = '\0';
			p+=2;
			for ( v=0 ; v<sizeof(map)/sizeof(map[0]) ; v++ )
			{
				if (map[v].sz==0) map[v].sz = strlen(map[v].name);
				if (strnicmp(map[v].name,p,map[v].sz)==0 && strncmp(p+map[v].sz,": ",2)==0)
				{
					if (map[v].type==INTEGER) { *(int*)(map[v].value) = atoi(p+map[v].sz+2); break; }
					else if (map[v].type==STRING) { *(char**)map[v].value = p+map[v].sz+2; break; }
				}
			}
		}
		output_verbose("%s (host='%s', len=%d)",http->query,host?host:"???",content_length);

		/* reject anything but a GET */
		if (stricmp(method,"GET")!=0)
		{
			http_status(http,HTTP_METHODNOTALLOWED);
			http_format(http,HTTP_METHODNOTALLOWED);
			http_type(http,"text/html");
			/* technically, we should add an Allow entry to the response header */
			http_send(http);
			break;
		}

		/* handle request */
		if (strncmp(uri,"/gui/",5)==0 )
		{
			if ( http_gui_request(http,uri+5) )
				http_status(http,HTTP_OK);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else if (strncmp(uri,"/output/",8)==0 )
		{
			if ( http_output_request(http,uri+8) )
				http_status(http,HTTP_OK);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else if (strncmp(uri,"/action/",8)==0) 
		{
			if ( http_action_request(http,uri+8) )
				http_status(http,HTTP_ACCEPTED);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else if ( strcmp(uri,"/favicon.ico")==0 )
		{
			if ( http_favicon(http) )
				http_status(http,HTTP_OK);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else if (strncmp(uri,"/",1)==0 )
		{
			if ( http_xml_request(http,uri+1) )
				http_status(http,HTTP_OK);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else 
		{
			http_status(http,HTTP_NOTFOUND);
			http_format(http,HTTP_NOTFOUND);
			http_type(http,"text/html");
			http_send(http);
		}

		/* keep-alive not desired*/
		if (connection && stricmp(connection,"close")==0)
			break;
	}
	http_close(http);
	output_verbose("socket %d closed",http->s);
}


