/* $Id: server.c 4738 2014-07-03 00:55:39Z dchassin $ 
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

#include "server.h"
#include "output.h"
#include "globals.h"
#include "object.h"
#include "exec.h"
#include "timestamp.h"
#include "load.h"

#include "legal.h"

#include "gui.h"

#define MAXSTR		1024		// maximum string length

static int shutdown_server = 0; /**< flag to stop accepting incoming connections */
SOCKET sockfd = (SOCKET)0; /**< socket on which incomming connections are accepted */

/** Callback function to shut server down
 
    This process halts both the server and the simulator.
	
	@return Nothing
 **/
static void shutdown_now(void)
{
	output_verbose("server shutdown on exit in progress...");
	exec_setexitcode(XC_SVRKLL);
	shutdown_server = 1;
	if (sockfd!=(SOCKET)0)
#ifdef WIN32
		shutdown(sockfd,SD_BOTH);
#else
		shutdown(sockfd,SHUT_RDWR);
#endif
	sockfd = (SOCKET)0;
	gui_wait_status(GUIACT_HALT);
	output_verbose("server shutdown on exit done");
}

#ifndef WIN32
/** Retrieve the last error code (Linux only)
	@returns errno code
 **/
int GetLastError()
{
	return errno;
}
#endif

void server_request(int);	// Function to handle clients' request(s)
void *http_response(void *ptr);

/** Send the data to the client
	@returns the number of bytes sent if successful, -1 if failed (errno is set).
 **/
static size_t send_data(SOCKET s, char *buffer, size_t len)
{
#ifdef WIN32
	return (size_t)send(s,buffer,(int)len,0);
#else
	return (size_t)write(s,buffer,len);
#endif	
}

/** Receive data from the client (blocking)
	@returns the number of bytes received if successful, -1 if failed (errno is set).
 **/
static size_t recv_data(SOCKET s,char *buffer, size_t len)
{
#ifdef WIN32
	return (size_t)recv(s,buffer,(int)len,0);
#else
	return (size_t)read(s,(void *)buffer,len);
#endif
}

/** Main server wait loop 
    @returns a pointer to the status flag
 **/
static unsigned int n_threads = 0;
static pthread_t thread_id;
static void *server_routine(void *arg)
{
	static int status = 0;
	static int started = 0;
	if (started)
	{
		output_error("server routine is already running");
		return NULL;
	}
	started = 1;
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

			if ( pthread_create(&thread_id,NULL, http_response,(void*)newsockfd)!=0 )
				output_error("unable to start http response thread");
			if (global_server_quit_on_close)
				shutdown_now();
			else
				gui_wait_status(0);
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
	started = 0;
	return (void*)&status;
}

/** Start accepting incoming connections on the designated server socket
	@returns SUCCESS/FAILED status code
 **/
static pthread_t thread;
STATUS server_startup(int argc, char *argv[])
{
	static int started = 0;
	int portNumber = global_server_portnum;
	SOCKET sockfd;
	struct sockaddr_in serv_addr;
#ifdef WIN32
	static WSADATA wsaData;
#endif

	if (started)
		return SUCCESS;

#ifdef WIN32
	output_debug("starting WS2");
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		output_error("socket library initialization failed: %s",strerror(GetLastError()));
		return FAILED;	
	}
#endif

	/* create a new socket */
	if ((sockfd = socket(AF_INET,SOCK_STREAM,IPPROTO_TCP)) == INVALID_SOCKET)
	{
		output_error("can't create stream socket: %s",strerror(GetLastError()));
		return FAILED;
	}
	atexit(shutdown_now);

	memset(&serv_addr,0,sizeof(serv_addr));

Retry:
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNumber);

	/* bind socket to server address */
	if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		if (portNumber<global_server_portnum+1000)
		{
			portNumber++;
			output_warning("server port not available, trying port %d...", portNumber);
			goto Retry;
		}
#ifdef WIN32
		output_error("can't bind to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
		output_error("can't bind address: %s",strerror(GetLastError()));
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
	global_server_portnum = portNumber;

	/* start the server thread */
	if (pthread_create(&thread,NULL,server_routine,(void*)sockfd))
	{
		output_error("server thread startup failed: %s",strerror(GetLastError()));
		return FAILED;
	}

	started = 1;
	return SUCCESS;
}
STATUS server_join(void)
{
	void *result;
	if (pthread_join(thread,&result)==0)
		return (STATUS) result;	
	else
	{
		output_error("server thread join failed: %s", strerror(GetLastError()));
		return FAILED;
	}
}

/********************************************************
 HTTPCNX routines
 */

typedef struct s_httpcnx {
	char query[1024];
	char *buffer;
	size_t len;
	size_t max;
	char *status;
	char *type;
	SOCKET s;
} HTTPCNX;

/** Create an HTTPCNX connection handle
    @returns HTTPCNX connection handle pointer on success, NULL on failure
 **/
static HTTPCNX *http_create(SOCKET s)
{
	HTTPCNX *http = (HTTPCNX*)malloc(sizeof(HTTPCNX));
	memset(http,0,sizeof(HTTPCNX));
	http->s = s;
	http->max = 4096;
	http->buffer = malloc(http->max);
	return http;
}

/** Reset an HTTPCNX connection handle

	This function clears the contents of HTTPCNX connection block so that it can be reused to handle a new message.
    @returns Nothing
 **/
static void http_reset(HTTPCNX *http)
{
	http->status = NULL;
	http->type = NULL;
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
#define HTTP_VERSIONNOTSUPPORTED "505 HTTPCNX Version not supported"

/** Set the HTTPCNX response status code **/
static void http_status(HTTPCNX *http, char *status)
{
	http->status = status;
}
/** Set the HTTPCNX response message type **/
static void http_type(HTTPCNX *http, char *type)
{
	http->type = type;
}
/** Send the HTTPCNX response **/
static void http_send(HTTPCNX *http)
{
	char header[4096];
	int len=0;
	len += sprintf(header+len, "HTTP/1.1 %s", http->status?http->status:HTTP_INTERNALSERVERERROR);
	output_verbose("%s (len=%d, mime=%s)",header,http->len,http->type?http->type:"none");
	len += sprintf(header+len, "\nContent-Length: %d\n", http->len);
	if (http->type && http->type[0]!='\0')
		len += sprintf(header+len, "Content-Type: %s\n", http->type);
	len += sprintf(header+len, "Cache-Control: no-cache\n");
	len += sprintf(header+len, "Cache-Control: no-store\n");
	len += sprintf(header+len, "Expires: -1\n");
	len += sprintf(header+len,"\n");
	send_data(http->s,header,len);
	if (http->len>0)
		send_data(http->s,http->buffer,http->len);
	http->len = 0;
}
/** Write the contents of the HTTPCNX message buffer **/
static void http_write(HTTPCNX *http, char *data, size_t len)
{
	if (http->len+len>=http->max)
	{
		/* extend buffer */
		void *old = http->buffer;
		if (http->len+len < http->max*2)
		{
			http->max *= 2;
		}
		else
		{
			http->max = http->len+len+1;
		}
		http->buffer = malloc(http->max);
		memcpy(http->buffer,old,http->len);
		free(old);
		old = NULL;
	}
	memcpy(http->buffer+http->len,data,len);
	http->len += len;
}
/** Close the HTTPCNX connection after sending content **/
static void http_close(HTTPCNX *http)
{
	if (http->len>0)
		http_send(http);
#ifdef WIN32
	closesocket(http->s);
#else
	close(http->s);
#endif
}
/** Set the response MIME type **/
static void http_mime(HTTPCNX *http, char *path)
{
	size_t len = strlen(path);
	static struct s_map {
		char *ext;
		char *mime;
	} map[] = {
		{".png","image/png"},
		{".js","text/javascript"},
	};
	int n;
	for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
	{
		if (strcmp(path+len-strlen(map[n].ext),map[n].ext)==0)
		{
			http->type = map[n].mime;
			return;
		}
	}
	output_warning("mime type for '%s' is not known", path);
	http->type = NULL;
	return;
}

/** Format HTTPCNX message content **/
static int http_format(HTTPCNX *http, char *format, ...)
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

/** Upquote message buffer content **/
char *http_unquote(char *buffer)
{
	char *eob = buffer+strlen(buffer)-1;
	if (buffer[0]=='"') buffer++;
	if (*eob=='"') *eob='\0';
	return buffer;
}

/** Get the value of a hex character
	@returns the value corresponding to the hex code
 **/
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

/** Decode the contents of a buffer (in place) **/
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

/** Process an incoming XML data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_xml_request(HTTPCNX *http,char *uri)
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

/** Process an incoming GUI request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_gui_request(HTTPCNX *http,char *uri)
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
/** Obtain the size of a file (non-Windows only)
	@returns the number of bytes in the file or -1 on failure
 **/
int filelength(int fd)
{
	struct stat fs;
	if (fstat(fd,&fs))
		return -1;
	else
		return fs.st_size;
}
#endif

/** Copy the content of a file to the client
	@returns the number of bytes sent
 **/
int http_copy(HTTPCNX *http, char *context, char *source)
{
	char *buffer;
	size_t len;
	FILE *fp = fopen(source,"rb");
	if (fp==NULL)
	{
		output_error("unable to find %s output '%s': %s", context, source, strerror(errno));
		return 0;
	}
	len = filelength(fileno(fp));
	if (len<0)
	{
		output_error("%s output '%s' not accessible", context, source);
		fclose(fp);
		return 0;
	}
	if (len==0)
	{
		output_warning("%s output '%s' is empty", context, source);
		fclose(fp);
		return 1;
	}
	buffer = (char*)malloc(len);
	if (buffer==NULL)
	{
		output_error("%s output buffer for '%s' not available", context, source);
		fclose(fp);
		return 0;
	}
	if (fread(buffer,1,len,fp)<=0)
	{
		output_error("%s output '%s' read failed", context, source);
		free(buffer);
		fclose(fp);
		return 0;
	}
	http_write(http,buffer,len);
	http_mime(http,source);
	free(buffer);
	fclose(fp);
	return 1;
}

/** Process an incoming output data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
 
int http_output_request(HTTPCNX *http,char *uri)
{
	char fullpath[1024];
	strcpy(fullpath,global_workdir);
	if (*(fullpath+strlen(fullpath)-1)!='/' || *(fullpath+strlen(fullpath)-1)!='\\' )
		strcat(fullpath,"/");
	strcat(fullpath,uri);
	return http_copy(http,"file",fullpath);
}

/** Process an incoming Java request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_java(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *jar = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("Java runtime request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (jar==NULL || strcmp(jar,".jar")!=0)
	{
		output_error("Java runtime request does not specify is a Java runtime filename with extension .jar");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
	sprintf(command,"java -jar %s",script);

	/* temporary cut off of plt extension to build output file */
	*jar = '\0'; sprintf(output,"%s.%s",uri,ext); *jar='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run java runtime on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("Java runtime return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"Java",output);
}

/** Process an incoming Perl data request
	@returns non-zero on success, 0 on failure (errno set)
 **/

int http_run_perl(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *pl = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("Perl request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (pl==NULL || strcmp(pl,".pl")!=0)
	{
		output_error("Perl request does not specify a Perl script filename with extension .pl");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
	sprintf(command,"perl %s",script);

	/* temporary cut off of plt extension to build output file */
	*pl = '\0'; sprintf(output,"%s.%s",uri,ext); *pl='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run Perl on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("Perl return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"Perl",output);
}

/** Process an incoming Python data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_python(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *py = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("Python request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (py==NULL || strcmp(py,".py")!=0)
	{
		output_error("Python request does not specify a Python script filename with extension .py");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
	sprintf(command,"python %s",script);

	/* temporary cut off of plt extension to build output file */
	*py = '\0'; sprintf(output,"%s.%s",uri,ext); *py='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run Python on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("Python return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"Python",output);
}

/** Process an incoming R data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_r(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *r = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("R request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (r==NULL || strcmp(r,".r")!=0)
	{
		output_error("R request does not specify an R script filename with extension .r");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
#ifdef WIN32
	sprintf(command,"r CMD BATCH %s",script);
#else
	sprintf(command,"R --vanilla CMD BATCH %s",script);
#endif

	/* temporary cut off of plt extension to build output file */
	*r = '\0'; sprintf(output,"%s.%s",uri,ext); *r='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run R on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("R return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"R",output);
}

/** Process an incoming Scilab data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_scilab(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *sce = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("Scilab request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (sce==NULL || strcmp(sce,".sce")!=0)
	{
		output_error("Scilab request does not specify a Scilab script filename with extension .sce");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
	sprintf(command,"scilab %s",script);

	/* temporary cut off of plt extension to build output file */
	*sce = '\0'; sprintf(output,"%s.%s",uri,ext); *sce='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run Scilab on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("Scilab return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"Scilab",output);
}

/** Process an incoming Octave data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_octave(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *m = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("Octave request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (m==NULL || strcmp(m,".m")!=0)
	{
		output_error("Octave request does not specify an Octave script filename with extension .m");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
	sprintf(command,"octave %s",script);

	/* temporary cut off of plt extension to build output file */
	*m = '\0'; sprintf(output,"%s.%s",uri,ext); *m='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run R on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("R return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"Octave",output);
}

/** Process an incoming Gnuplot data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_run_gnuplot(HTTPCNX *http,char *uri)
{
	char script[1024];
	char command[1024];
	char output[1024];
	char *mime = strchr(uri,'?');
	char *ext = mime?strchr(mime,'/'):NULL;
	char *plt = strrchr(uri,'.');
	int rc = 0;

	/* find mime and extension */
	if (mime==NULL)
	{
		output_error("gnuplot request does not include mime type");
		return 0;
	}
	else
		*mime++ = '\0'; /* mime type actually start at next character */
	if (ext) ext++;

	/* if not a plot request */
	if (plt==NULL || strcmp(plt,".plt")!=0)
	{
		output_error("gnuplot request does not specify a plot script filename with extension .plt");
		return 0;
	}

	/* setup gnuplot command */
	sprintf(script,"%s",uri);
#ifdef WIN32
	sprintf(command,"wgnuplot %s",script);
#else
	sprintf(command,"gnuplot %s",script);
#endif
	/* temporary cut off of plt extension to build output file */
	*plt = '\0'; sprintf(output,"%s.%s",uri,ext); *plt='.';

	/* run gnuplot */
	output_verbose("%s", command);
	if ((rc=system(command))!=0)
	{
		switch (rc)
		{
		case -1: /* an error occurred */
			output_error("unable to run gnuplot on '%s': %s", uri, strerror(errno));
			break;
		default:
			output_error("gnuplot return error code %d on '%s'", rc, uri);
			break;
		}
		return 0;
	}

	/* copy output to http */
	return http_copy(http,"gnuplot",output);
}

/** Process an incoming runtime file request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_get_rt(HTTPCNX *http,char *uri)
{
	char fullpath[1024];
	if (!find_file(uri,NULL,R_OK,fullpath,sizeof(fullpath)))
	{
		output_error("runtime file '%s' couldn't be located in GLPATH='%s'", uri,getenv("GLPATH"));
		return 0;
	}
	return http_copy(http,"runtime",fullpath);
}

/** Process an incoming action request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_action_request(HTTPCNX *http,char *action)
{
	if (gui_post_action(action)==-1)
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

/** Process an incoming main loop control request
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_control_request(HTTPCNX *http, char *action)
{
	char buffer[1024];

	if ( strcmp(action,"resume")==0 )
	{
		exec_mls_resume(TS_NEVER);
		return 1;
	}
	else if ( sscanf(action,"pauseat=%[-0-9%:A-Za-z]",buffer)==1 )
	{
		TIMESTAMP ts;
		http_decode(buffer);
		ts = convert_to_timestamp(buffer);
		if ( ts!=TS_INVALID )
		{
			exec_mls_resume(ts);
			return 1;
		}
		else
		{
			output_error("control command '%s' has an invalid timestamp", buffer);
			return 0;
		}
	}
	else if ( strcmp(action,"shutdown")==0 )
	{
		output_message("server shutdown by client");
		exit(XC_SUCCESS);
	}
	return 0;
}

/** Process an incoming main loop control request
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_open_request(HTTPCNX *http, char *action)
{
	if ( loadall(action) )
		return 1;
	else
		return 0;
}
/** Process an incoming favicon request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_favicon(HTTPCNX *http)
{
	char fullpath[1024];
	if ( find_file("favicon.ico",NULL,R_OK,fullpath,sizeof(fullpath))==NULL )
	{
		output_error("file 'favicon.ico' not found", fullpath);
		return 0;
	}
	return http_copy(http,"icon",fullpath);
}

/** Process an incoming request
	@returns nothing
 **/
void *http_response(void *ptr)
{
	SOCKET fd = (SOCKET)ptr;
	HTTPCNX *http = http_create(fd);
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
		size_t sz;
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
		
		/* initialize the response */
		http_reset(http);

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
		if ( strcmp(uri,"/favicon.ico")==0 )
		{
			if ( http_favicon(http) )
				http_status(http,HTTP_OK);
			else
				http_status(http,HTTP_NOTFOUND);
			http_send(http);
		}
		else {
			static struct s_map {
				char *path;
				int (*request)(HTTPCNX*,char*);
				char *success;
				char *failure;
			} map[] = {
				/* this is the map of recognize request types */
				{"/control/",	http_control_request,	HTTP_ACCEPTED, HTTP_NOTFOUND},
				{"/open/",		http_open_request,		HTTP_ACCEPTED, HTTP_NOTFOUND},
				{"/xml/",		http_xml_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/gui/",		http_gui_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/output/",	http_output_request,	HTTP_OK, HTTP_NOTFOUND},
				{"/action/",	http_action_request,	HTTP_ACCEPTED,HTTP_NOTFOUND},
				{"/rt/",		http_get_rt,			HTTP_OK, HTTP_NOTFOUND},
				{"/perl/",		http_run_perl,			HTTP_OK, HTTP_NOTFOUND},
				{"/gnuplot/",	http_run_gnuplot,		HTTP_OK, HTTP_NOTFOUND},
				{"/java/",		http_run_java,			HTTP_OK, HTTP_NOTFOUND},
				{"/python/",	http_run_python,		HTTP_OK, HTTP_NOTFOUND},
				{"/r/",			http_run_r,				HTTP_OK, HTTP_NOTFOUND},
				{"/scilab/",	http_run_scilab,		HTTP_OK, HTTP_NOTFOUND},
				{"/octave/",	http_run_octave,		HTTP_OK, HTTP_NOTFOUND},
			};
			int n;
			for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
			{
				size_t len = strlen(map[n].path);
				if (strncmp(uri,map[n].path,len)==0)
				{
					if ( map[n].request(http,uri+len) )
						http_status(http,map[n].success);
					else
						http_status(http,map[n].failure);
					http_send(http);
					goto Next;
				}
			}
		}
		/* deprecated XML usage */
		if (strncmp(uri,"/",1)==0 )
		{
			if ( http_xml_request(http,uri+1) )
			{	
				output_warning("deprecate XML usage in request '%s'", uri);
				http_status(http,HTTP_OK);
			}
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
Next:
		if (connection && stricmp(connection,"close")==0)
			break;
	}
	http_close(http);
	output_verbose("socket %d closed",http->s);
	return 0;
}


