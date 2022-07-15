/* $Id: server.c 4738 2014-07-03 00:55:39Z dchassin $ 
 * server.c
 */

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32

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

#include <cerrno>
#include <cstring>
#include <memory.h>
#include <pthread.h>
#include <unistd.h>

#include "server.h"
#include "output.h"
#include "globals.h"
#include "object.h"
#include "exec.h"
#include "timestamp.h"
#include "load.h"
#include "find.h"

#include "legal.h"

#include "gui.h"
#include "kml.h"

#define SET_MYCONTEXT(X)
#define IN_MYCONTEXT
SET_MYCONTEXT(DMC_SERVER)

#define MAXSTR		1024		// maximum string length

static int shutdown_server = 0; /**< flag to stop accepting incoming connections */
SOCKET sockfd = (SOCKET)0; /**< socket on which incomming connections are accepted */

/** Callback function to shut server down
 
    This process halts both the server and the simulator.
	
	@return Nothing
 **/
static void shutdown_now(void)
{
	IN_MYCONTEXT output_verbose("server shutdown on exit in progress...");
	exec_setexitcode(XC_SVRKLL);
	shutdown_server = 1;
	if (sockfd!=(SOCKET)0)
#ifdef _WIN32
		shutdown(sockfd,SD_BOTH);
#else
		shutdown(sockfd,SHUT_RDWR);
#endif
	sockfd = (SOCKET)0;
	gui_wait_status(GUIACT_HALT);
	IN_MYCONTEXT output_verbose("server shutdown on exit done");
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
#ifdef _WIN32
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
#ifdef _WIN32
	return (size_t)recv(s,buffer,(int)len,0);
#else
	return (size_t)read(s,(void *)buffer,len);
#endif
}

int client_allowed(char *saddr)
{
	return strncmp(saddr,global_client_allowed,strlen(global_client_allowed))==0;
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
	sockfd = *reinterpret_cast<SOCKET*>(arg);
	// repeat forever..
	static int active = 0;
	void *result = NULL;
	while (!shutdown_server)
	{
		struct sockaddr_in cli_addr;
		SOCKET newsockfd;

#ifdef WIN32
		int clilen;
#else
		unsigned int clilen;
#endif

		clilen = sizeof(cli_addr);

		/* accept client request and get client address */
		newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
		if ((int)newsockfd<0 && errno!=EINTR)
		{
			status = GetLastError();
			output_warning("server accept failed on socket %d: code %d", sockfd, status);
			//goto Done;
		}
		else if ((int)newsockfd > 0)
		{
			char *saddr = inet_ntoa(cli_addr.sin_addr);
			if ( !client_allowed(saddr) )
			{
				output_error("denying connection from %s on port %d",saddr, cli_addr.sin_port);
				close(newsockfd);
				continue;
			}
			IN_MYCONTEXT output_verbose("accepting connection from %s on port %d",saddr, cli_addr.sin_port);
			if ( active )
				pthread_join(thread_id,&result);
			if ( pthread_create(&thread_id,NULL, http_response,reinterpret_cast<int*>(newsockfd))!=0 )
				output_error("unable to start http response thread");
			if (global_server_quit_on_close)
				shutdown_now();
			else
				gui_wait_status(static_cast<GUIACTIONSTATUS>(0));
			active = 1;
		}
	}
	IN_MYCONTEXT output_verbose("server shutdown");
Done:
	started = 0;
	return (void*)&status;
}

/** Start accepting incoming connections on the designated server socket
	@returns SUCCESS/FAILED status code
 **/
#define DEFAULT_PORTNUM 6267
static pthread_t startup_thread;
STATUS server_startup(int argc, char *argv[])
{
	static int started = 0;
	int enable = 1;
	int portNumber = global_server_portnum==0 ? DEFAULT_PORTNUM : global_server_portnum;
	SOCKET sockfd;
	struct sockaddr_in serv_addr;
#ifdef _WIN32
	static WSADATA wsaData;
#endif
	void *result = NULL;

	if (started)
		return SUCCESS;

#ifdef _WIN32
	IN_MYCONTEXT output_debug("starting WS2");
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
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&enable), sizeof(enable)) < 0)
		output_error("setsockopt(SO_REUSEADDR) failed: %s", strerror(GetLastError()));
	memset(&serv_addr,0,sizeof(serv_addr));

Retry:
	serv_addr.sin_family = AF_INET;
	if ( strlen(global_server_inaddr)>0 )
	{
		serv_addr.sin_addr.s_addr = inet_addr(global_server_inaddr);
		if ( !serv_addr.sin_addr.s_addr )
		{
			output_error("invalid server_inaddr argument supplied : %s", global_server_inaddr);
			return FAILED;
		}
	}
	else
	{
		serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	serv_addr.sin_port = htons(portNumber);

	/* bind socket to server address */
	if ( bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0 )
	{
		if ( global_server_portnum == 0 && portNumber < (DEFAULT_PORTNUM+1000) )
		{
			portNumber++;
			output_warning("server port not available, trying port %d...", portNumber);
			goto Retry;
		}
#ifdef _WIN32
		output_error("can't bind to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
		output_error("can't bind address: %s",strerror(GetLastError()));
#endif
		return FAILED;
	}
#ifdef _WIN32
	IN_MYCONTEXT output_verbose("bind ok to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
#else
	IN_MYCONTEXT output_verbose("bind ok to address");
#endif	
	/* listen for connection */
	if ( listen(sockfd,5) < 0 )
	{
		output_error("socket listen failed: sockfd=%d, error='%s'", sockfd, strerror(GetLastError()));
		return FAILED;
	}
	IN_MYCONTEXT output_verbose("server listening to port %d", portNumber);
	global_server_portnum = portNumber;

	/* join the old thread and wait if it hasn't finished yet */
	if ( started ) {
		pthread_join(startup_thread,&result);
	}

	/* start the new thread */
	if (pthread_create(&startup_thread,NULL,server_routine, reinterpret_cast<int*>(sockfd)))
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
	if (pthread_join(startup_thread,&result)==0)
		return *static_cast<STATUS*>(result);
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
	size_t max_size;
	char *status;
	char *type;
	SOCKET s;
	bool cooked;
} HTTPCNX;

/** Create an HTTPCNX connection handle
    @returns HTTPCNX connection handle pointer on success, NULL on failure
 **/
static HTTPCNX *http_create(SOCKET s)
{
	HTTPCNX *http = (HTTPCNX*)malloc(sizeof(HTTPCNX));
	memset(http,0,sizeof(HTTPCNX));
	http->s = s;
	http->max_size = 65536;
	http->buffer = static_cast<char *>(malloc(http->max_size));
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
static void http_status(HTTPCNX *http, const char *status)
{
	http->status = const_cast<char*>(status);
}
/** Set the HTTPCNX response message type **/
static void http_type(HTTPCNX *http, const char *type)
{
	http->type = const_cast<char*>(type);
}
/** Send the HTTPCNX response **/
static void http_send(HTTPCNX *http)
{
	char header[4096];
	int len=0;
	len += sprintf(header+len, "HTTP/1.1 %s", http->status?http->status:HTTP_INTERNALSERVERERROR);
	IN_MYCONTEXT output_verbose("%s (len=%d, mime=%s)",header,http->len,http->type?http->type:"none");
	len += sprintf(header+len, "\nContent-Length: %ld\n", http->len);
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
/** Cook the contents of the HTTPCNX message buffer, if limit==0 returns only bytes needed to store result */
static size_t http_rewrite(char *out, char *in, size_t len, size_t limit)
{
	char name[64], *n;
	size_t count = 0;
	enum {RAW, COOKED} state = RAW;
	size_t i;
	for ( i = 0 ; i < len ; i++ )
	{
		if ( state==RAW )
		{
			if ( in[i]=='<' && in[i+1]=='<' && in[i+2]=='<' )
			{
				i += 2;
				state = COOKED;
				memset(name,0,sizeof(name));
				n = name;
			}
			else
			{	if ( count<limit )
					out[count] = in[i];
				count++;
			}
		}
		else if ( state==COOKED )
		{
			if ( in[i]=='>' && in[i+1]=='>' && in[i+2]=='>' )
			{
				char buffer[1024];
				if ( global_getvar(name,buffer,sizeof(buffer))==NULL )
				{
					output_error("http_rewrite(): '%s' not found", name);
				}
				else
				{
					int vlen = strlen(buffer);
					if ( count+vlen < limit )
						strcpy(out+count,buffer);
					count += strlen(buffer);
				}
				i += 2;
				state = RAW;
			}
			else
				*n++ = in[i];
		}
	}
	return count;
}

/** Write the contents of the HTTPCNX message buffer **/
static void http_write(HTTPCNX *http, const char *data, size_t len)
{
	char *tmp = NULL;
	if ( http->cooked )
	{
		size_t need = http_rewrite(tmp, const_cast<char*>(data),len,0);
		tmp = (char*)malloc(need*2+1);
		len = http_rewrite(tmp, const_cast<char*>(data),len,need*2);
	}
	if (http->len+len>=http->max_size)
	{
		/* extend buffer */
		void *old = http->buffer;
		if (http->len+len < http->max_size*2)
		{
			http->max_size *= 2;
		}
		else
		{
			http->max_size = http->len+len+1;
		}
		http->buffer = static_cast<char *>(malloc(http->max_size));
		memcpy(http->buffer,old,http->len);
		free(old);
	}
	memcpy(http->buffer+http->len,tmp?tmp:data,len);
	if ( tmp )
		free(tmp);
	http->len += len;
}

/** Close the HTTPCNX connection after sending content **/
static void http_close(HTTPCNX *http)
{
	if (http->len>0)
		http_send(http);
#ifdef _WIN32
	closesocket(http->s);
#else
	close(http->s);
#endif
	free(http->buffer);
}
/** Set the response MIME type **/
static void http_mime(HTTPCNX *http, char *path)
{
	size_t len = strlen(path);
	static struct s_map {
		const char *ext;
		const char *mime;
	} map[] = {
		{".png","image/png"},
		{".js","text/javascript"},
		{".kml","application/vnd.google-earth.kml+xml"},
		{".htm","text/html"},
		{".ico","image/x-icon"},
		{".txt","text/plain"},
		{".log","text/plain"},
		{".glm","text/plain"},
		{".php","text/plain"},
	};
	int n;
	for ( n=0 ; n<sizeof(map)/sizeof(map[0]) ; n++ )
	{
		if (strcmp(path+len-strlen(map[n].ext),map[n].ext)==0)
		{
			http->type = const_cast<char*>(map[n].mime);
			return;
		}
	}
	output_warning("mime type for '%s' is not known", path);
	http->type = NULL;
	return;
}

/** Format HTTPCNX message content **/
static int http_format(HTTPCNX *http, const char *format, ...)
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

int get_value_with_unit(OBJECT *obj, char *arg1, char *arg2, char *buffer, size_t len)
{
	char *uname = strchr(arg2,'[');
	if ( uname!=NULL )
	{
		UNIT *unit;
		PROPERTY *prop;
		double rvalue;
		gld::complex cvalue;
		char *spec = NULL;
		int prec = 4;
		char fmt[64];

		/* find the end of the unit definition */
		char *p = strchr(uname,']');
		if ( p!=NULL ) *p='\0';
		else {
			output_error("object '%s' property '%s' unit spec in incomplete or invalid", arg1, arg2);
			return 0;
		}
		*uname++ = '\0';

		/* find the format specs */
		spec = strchr(uname,',');
		if ( spec!=NULL )
			*spec++ = '\0';
		else
			spec = const_cast<char*>("4g");

		/* check spec for conformance */
		if ( strchr("0123456789",spec[0])==NULL || strchr("aAfFgGeE",spec[1])==NULL )
		{
			output_error("object '%s' property '%s' unit format '%s' is invalid (must be [0-9][aAeEfFgG])", arg1, arg2, spec);
			return 0;
		}

		/* get the unit */
		unit = unit_find(uname);
		if ( unit==NULL )
		{
			output_error("object '%s' property '%s' unit '%s' not found", arg1, arg2, uname);
			return 0;
		}

		/* get the property */
		prop = object_get_property(obj,arg2,NULL);
		if ( prop==NULL )
		{
			output_error("object '%s' property '%s' not found", arg1, arg2);
			return 0;
		}
		if ( prop->unit==NULL )
		{
			output_error("class '%s' property '%s' has no units", obj->oclass->name, prop->name);
			return 0;
		}

		/* handle complex numbers */
		if ( prop->ptype==PT_complex )
		{
			cvalue = *object_get_complex_quick(obj,prop);
			if ( !unit_convert_complex(prop->unit,unit,&cvalue) )
			{
				output_error("object '%s' property '%s' conversion from '%s' to '%s' failed", arg1, arg2, prop->unit->name, unit);
				return 0;
			}
			switch ( spec[2]=='\0' ? cvalue.Notation() : spec[2] ) {
			case I: // i-notation
				sprintf(fmt,"%%.%c%c%%+.%c%ci %%s",spec[0],spec[1],spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Re(),cvalue.Im(),uname);
				break;
			case J: // j-notation
				sprintf(fmt,"%%.%c%c%%+.%c%cj %%s",spec[0],spec[1],spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Re(),cvalue.Im(),uname);
				break;
			case A: // degrees
				sprintf(fmt,"%%.%c%c%%+.%c%cd %%s",spec[0],spec[1],spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Mag(),cvalue.Arg()*180/PI,uname);
				break;
			case R: // radians
				sprintf(fmt,"%%.%c%c%%+.%c%cr %%s",spec[0],spec[1],spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Mag(),cvalue.Arg(),uname);
				break;
			case 'M': // magnitude only
				sprintf(fmt,"%%.%c%c %%s",spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Mag(),uname);
				break;
			case 'D': // angle only in degrees
				sprintf(fmt,"%%.%c%c deg",spec[0],spec[1]);
				snprintf(buffer,len,fmt,cvalue.Arg()*180/PI,uname);
				break;
			case 'R': // angle only in radians
				sprintf(fmt,"%%.%c%c rad",spec[0],spec[1]);
				sprintf(buffer,fmt,cvalue.Arg(),uname);
				break;
			case 'X': // real part only
				sprintf(fmt,"%%.%c%c %%s",spec[0],spec[1]);
				sprintf(buffer,fmt,cvalue.Re(),uname);
				break;
			case 'Y': // imaginary part only
				sprintf(fmt,"%%.%c%c %%s",spec[0],spec[1]);
				sprintf(buffer,fmt,cvalue.Im(),uname);
				break;
			default:
				output_error("object '%s' property '%s' complex angle notation '%c' is not valid", arg1, arg2, spec[2]=='\0' ? cvalue.Notation() : spec[3]);
				return 0;
			}
		}
		else /* handle doubles */
		{
			sprintf(fmt,"%%.%c%c %%s",spec[0],spec[1]);
			rvalue = *object_get_double_quick(obj,prop);
			if ( !unit_convert_ex(prop->unit,unit,&rvalue) )
			{
				output_error("object '%s' property '%s' conversion from '%s' to '%s' failed", arg1, arg2, prop->unit->name, unit);
				return 0;
			}
			sprintf(buffer,fmt,rvalue,uname);
		}
	}
	else if ( !object_get_value_by_name(obj,arg2,buffer,len) )
	{
		output_error("object '%s' property '%s' not found", arg1, arg2);
		return 0;
	}
	return strlen(buffer);
}
/** Process an incoming raw data request
 * @returns non-zero on success, 0 on failure (errno set)
 */
int http_raw_request(HTTPCNX *http, char *uri)
{
	char arg1[1024]="", arg2[1024]="";
	int nargs = sscanf(uri,"%1023[^/=\r\n]/%1023[^\r\n=]",arg1,arg2);
	char *value = strchr(uri,'=');
	char buffer[1024]="";
	OBJECT *obj=NULL;
	char *id;

	/* value */
	if (value) value++;

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
		http_format(http,"%s", http_unquote(buffer));
		http_type(http,"text/plain");
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
		if ( !get_value_with_unit(obj,arg1,arg2,buffer,sizeof(buffer)) )
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
		http_format(http,"%s", http_unquote(buffer));
		http_type(http,"text/plain");
		return 1;

	default:
		return 0;
	}
	return 0;
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
	if (value) value++;

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

		if ( strcmp(arg2,"*")==0 )
		{
			PROPERTY *prop;
			char buffer[1024];
			http_format(http,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
			http_format(http,"<properties>\n");
#define PROPERTY(N,F,V) http_format(http,"\t<property>\n\t\t<name>N##</name>\n\t\t<value>F##</value>\n\t</property>\n", V)
			PROPERTY("id","%d",obj->id);
			PROPERTY("class","%s",obj->oclass->name);
			if ( obj->name ) PROPERTY("name","%s",object_name(obj,buffer,sizeof(buffer)));
			if ( strlen(obj->groupid)>0 ) PROPERTY("groupid","%s",obj->groupid.get_string());
			if ( obj->parent ) PROPERTY("parent","%s",object_name(obj->parent,buffer,sizeof(buffer)));
			PROPERTY("rank","%d",obj->rank);
			PROPERTY("clock","%lld",obj->clock);
			if ( obj->valid_to < TS_NEVER ) PROPERTY("valid_to","%lld",obj->valid_to);
			if ( obj->schedule_skew ) PROPERTY("schedule_skew","%lld",obj->schedule_skew);
			if ( !isnan(obj->latitude) ) PROPERTY("latitude","%.6f",obj->latitude);
			if ( !isnan(obj->longitude) ) PROPERTY("longitude","%.6f",obj->longitude);
			if ( obj->in_svc > TS_ZERO ) {
				PROPERTY("in_svc","%lld",obj->in_svc);
				if ( obj->in_svc_micro > 0 ) PROPERTY("in_svc_micro","%f",obj->in_svc_micro);
			}
			if ( obj->out_svc< TS_NEVER )
			{
				PROPERTY("out_svc","%lld",obj->out_svc);
				if ( obj->out_svc_micro > 0 ) PROPERTY("out_svc_micro","%f",obj->out_svc_micro);
			}
			if ( obj->heartbeat > 0 ) PROPERTY("heartbeat","%lld",obj->heartbeat);
			if ( obj->flags > 0 ) PROPERTY("flags","0x%lx",obj->flags);

			for ( prop=obj->oclass->pmap; prop!=NULL; prop=(prop->next?prop->next:(prop->oclass->parent?prop->oclass->parent->pmap:NULL)) )
			{
				http_format(http,"\t<property>\n\t\t<name>%s</name>\n",prop->name);
				http_format(http,"\t\t<value>%s</value>\n\t</property>\n",object_get_value_by_name(obj,prop->name,buffer,sizeof(buffer))>0?buffer:"");
			}
#undef PROPERTY
			http_format(http,"</properties>\n");
		}
		else
		{	
			PROPERTY *prop = object_get_property(obj, arg2, NULL);
			PROPERTYSPEC *spec = prop ? property_getspec(prop->ptype) : NULL;
			/* get the unit (if any) */
			if ( !get_value_with_unit(obj,arg1,arg2,buffer,sizeof(buffer)) )
				return 0;

			/* assignment, if any */
			if ( value && !object_set_value_by_name(obj,arg2,value) )
			{
				output_error("cannot set object '%s' property '%s' to '%s'", arg1, arg2, value);
				return 0;
			}

			/* post the response */
			http_format(http,"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n");
			http_format(http,"<property>\n\t<object>%s</object>\n", arg1);
			http_format(http,"\t<name>%s</name>\n", arg2);
			http_format(http,"\t<value>%s</value>\n", http_unquote(buffer));
			if ( spec!=NULL ) http_format(http,"\t<type>%s</type>\n", spec->name);
			http_format(http,"</property>\n");
		}
		http_type(http,"text/xml");
		return 1;

	default:
		return 0;
	}
	return 0;
}

/** Process an incoming JSON data request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_json_request(HTTPCNX *http,char *uri)
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
			http_format(http, const_cast<char *>("{\"error\": \"globalvar not found\", \"query\": \"%s\"}\n"), arg1);
			http_type(http, const_cast<char *>("text/json"));
			return 1;
		}

		/* assignment, if any */
		if (value) global_setvar(arg1,value);

		/* post the response */
		http_format(http, const_cast<char *>("{\"%s\": \"%s\"}\n"),
			arg1, http_unquote(buffer));
		http_type(http, const_cast<char *>("text/json"));
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
			http_format(http, const_cast<char *>("{\"error\": \"object not found\", \"query\": \"%s\"}\n"), arg1);
			http_type(http, const_cast<char *>("text/json"));
			return 1;
		}

		if ( arg2[0]=='*' )
		{
			bool use_tuple = strcmp(arg2,"*")==0 || strcmp(arg2,"*[tuple]")==0;
			if ( !use_tuple && strcmp(arg2,"*[dict]")!=0 )
			{
				http_format(http, const_cast<char *>("{\"error\": \"invalid '*' query format\", \"query\": \"%s\"}\n"), arg2);
				http_type(http, const_cast<char *>("text/json"));
				return 1;
			}
			PROPERTY *prop;
			char buffer[1024];
			if ( use_tuple ) http_format(http,"["); else http_format(http, const_cast<char *>("{"));
#define PROPERTY(N,F,V) {if ( use_tuple ) http_format(http,"\n\t{\"N##\": \"F##\"},", V); else http_format(http," \"N##\": \"F##\",", V);}
			PROPERTY("id","%d",obj->id);
			PROPERTY("class","%s",obj->oclass->name);
			if ( obj->name ) PROPERTY("name","%s",object_name(obj,buffer,sizeof(buffer)));
			if ( strlen(obj->groupid)>0 ) PROPERTY("groupid","%s",obj->groupid.get_string());
			if ( obj->parent ) PROPERTY("parent","%s",object_name(obj->parent,buffer,sizeof(buffer)));
			PROPERTY("rank","%d",obj->rank);
			PROPERTY("clock","%lld",obj->clock);
			if ( obj->valid_to < TS_NEVER ) PROPERTY("valid_to","%lld",obj->valid_to);
			if ( obj->schedule_skew ) PROPERTY("schedule_skew","%lld",obj->schedule_skew);
			if ( !isnan(obj->latitude) ) PROPERTY("latitude","%.6f",obj->latitude);
			if ( !isnan(obj->longitude) ) PROPERTY("longitude","%.6f",obj->longitude);
			if ( obj->in_svc > TS_ZERO ) {
				PROPERTY("in_svc","%lld",obj->in_svc);
				if ( obj->in_svc_micro > 0 ) PROPERTY("in_svc_micro","%f",obj->in_svc_micro);
			}
			if ( obj->out_svc< TS_NEVER )
			{
				PROPERTY("out_svc","%lld",obj->out_svc);
				if ( obj->out_svc_micro > 0 ) PROPERTY("out_svc_micro","%f",obj->out_svc_micro);
			}
			if ( obj->heartbeat > 0 ) PROPERTY("heartbeat","%lld",obj->heartbeat);
			PROPERTY("flags","0x%lx",obj->flags);

			for ( prop=obj->oclass->pmap; prop!=NULL; prop=(prop->next?prop->next:(prop->oclass->parent?prop->oclass->parent->pmap:NULL)) )
			{
				if ( prop!=obj->oclass->pmap)
				{
					if ( use_tuple ) http_format(http, const_cast<char *>("%s\n"), ","); else http_format(http,
																										  const_cast<char *>("%s "), ",");
				}
				else
				{
					if ( use_tuple ) http_format(http,"%s","\n");
				}
				if ( object_get_value_by_name(obj,prop->name,buffer,sizeof(buffer))>0 )
				{
					if ( use_tuple )
						http_format(http, const_cast<char *>("\t{\"%s\": \"%s\"}"), prop->name, http_unquote(buffer));
					else
						http_format(http, const_cast<char *>("\"%s\": \"%s\""), prop->name, http_unquote(buffer));
				}
				else
				{
					http_format(http,
								const_cast<char *>("{\"error\" : \"unable to get property value\", \"object\": \"%s\", \"property\": \"%s\"}\n"), arg1, arg2);
					http_type(http, const_cast<char *>("text/json"));
					return 1;
				}
			}
#undef PROPERTY
			if ( use_tuple ) http_format(http, const_cast<char *>("\n\t]\n")); else http_format(http,
																								const_cast<char *>("}\n"));
		}
		else
		{
			PROPERTY *prop = object_get_property(obj, arg2, NULL);
			PROPERTYSPEC *spec = prop ? property_getspec(prop->ptype) : NULL;
			if ( !get_value_with_unit(obj,arg1,arg2,buffer,sizeof(buffer)) )
			{
				http_format(http,
							const_cast<char *>("{\"error\": \"property not found\", \"object\": \"%s\", \"property\": \"%s\"}\n"), arg1, arg2);
				http_type(http, const_cast<char *>("text/json"));
				return 1;
			}

			/* assignment, if any */
			if ( value && !object_set_value_by_name(obj,arg2,value) )
			{
				http_format(http,
							const_cast<char *>("{\"error\": \"property write failed\", \"object\": \"%s\", \"property\": \"%s\", \"value\": \"%s\"}\n"), arg1, arg2, value);
				http_type(http, const_cast<char *>("text/json"));
				return 1;
			}

			/* post the response */
			http_format(http, const_cast<char *>("{\t\"object\" : \"%s\", \n"), arg1);
			http_format(http, const_cast<char *>("\t\"name\" : \"%s\", \n"), arg2);
			if ( spec!=NULL ) 
				http_format(http, const_cast<char *>("\t\"type\" : \"%s\", \n"), spec->name);
			http_format(http, const_cast<char *>("\t\"value\" : \"%s\"\n}\n"), http_unquote(buffer));
		}
		http_type(http, const_cast<char *>("text/json"));
		return 1;

	default:
		return 0;
	}
	return 0;
}

/** Process an incoming find request
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_find_request(HTTPCNX *http,char *uri)
{
	FINDPGM *finder = find_mkpgm(uri);
	FINDLIST *list;
	OBJECT *obj;
	if ( finder == NULL )
		return 0;
	list = find_runpgm(NULL,finder);
	if ( list == NULL )
		return 0;
	http_format(http,"[\n");
	obj = find_first(list);
	while ( 1 )
	{
		if ( obj->name == NULL )
			http_format(http,"\t{\"name\" : \"%s:%d\"}",obj->oclass->name,obj->id);
		else
			http_format(http,"\t{\"name\" : \"%s\"}",obj->name);
		obj = find_next(list,obj);
		if ( obj!=NULL )
			http_format(http,",\n\t");
		else
			break;
	}
	http_format(http,"\n\t]\n");
	http_type(http,"text/json");
	return 1;
}

/** Process a bulk modify request
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_modify_request(HTTPCNX *http, char *uri)
{
	char *p = uri;
	while ( p != NULL && *p != '\0' ) 
	{
		char oname[1024], pname[1024], value[1024];
		if ( sscanf(p,"%[^.].%[^=]=%[^;]",oname,pname,value) == 3 )
		{
			OBJECT *obj = object_find_name(oname);
			if ( obj == NULL )
				output_error("object '%s' not found", oname);
			else if ( object_set_value_by_name(obj,pname,value) <= 0 )
				output_error("object '%s' property '%s' set to '%s' failed", oname, pname, value);
		} 
		else
			output_error("modify syntax error at '%s'", p);
		p = strchr(p+1,';');
		if ( p != NULL ) p++;
	};
	return 1;
}

/** Process a bulk read request
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_read_request(HTTPCNX *http, char *uri)
{
	char *p = uri;
	http_format(http,"[\n\t");
	int first=1;
	while ( *p != '\0' )
	{
		char oname[1024], pname[1024];
		if ( sscanf(p,"%[^.].%[^;]",oname,pname) == 2 )
		{
			char value[1024];
			OBJECT *obj = object_find_name(oname);
			if ( obj == NULL )	
				output_error("object '%s' not found", oname);
			else if ( object_get_value_by_name(obj,pname,value,sizeof(value)) <= 0 )
				output_error("object '%s' property '%s' get failed", oname, pname);
			else
			{
				if ( !first )
					http_format(http,",\n\t");
				else
					first = 0;
				http_format(http,"{\"object\" : \"%s\" , \"property\" : \"%s\" , \"value\" : \"%s\" }",oname,pname,value);
			}
		}
		else
			output_error("read syntax error at '%s'", p);
		p = strchr(p+1,';');
		if ( p != NULL ) 
			p++;
		else
			break;
	}
	http_format(http,"\n\t]\n");
	http_type(http,"text/json");
	return 1;
}

/** Process an incoming GUI request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_gui_request(HTTPCNX *http,char *uri)
{
	gui_set_html_stream((void*)http, reinterpret_cast<GUISTREAMFN>(http_format));
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
int http_copy(HTTPCNX *http, const char *context, char *source, int cook, size_t pos)
{
	char *buffer;
	size_t len;
	int old_cooked;
	FILE *fp = fopen(source,"rb");
	if (fp==NULL)
	{
		output_error("unable to find %s output '%s': %s", context, source, strerror(errno));
		return 0;
	}
	if ( pos >= 0 )
		fseek(fp,pos,SEEK_SET);
	else
		pos = 0;
	len = filelength(fileno(fp)) - pos;
	if (len<0)
	{
		output_error("%s output '%s' not accessible", context, source);
		fclose(fp);
		return 0;
	}
	if ( len == 0 )
	{
		http_mime(http,source);
		http_write(http,"",0);
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
	size_t result = fread(buffer,1,len,fp);
	if (result<0)
	{
		output_error("%s output '%s' read failed", context, source);
		free(buffer);
		fclose(fp);
		return 0;
	}
	http_mime(http,source);
	old_cooked = http->cooked;
	http->cooked = cook;
	http_write(http,buffer,len);
	http->cooked = old_cooked;
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
	return http_copy(http,"file",fullpath,false,0);
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
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"Java",output,true,0);
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
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"Perl",output,true,0);
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
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"Python",output,true,0);
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
#ifdef _WIN32
	sprintf(command,"r CMD BATCH %s",script);
#else
	sprintf(command,"R --vanilla CMD BATCH %s",script);
#endif

	/* temporary cut off of plt extension to build output file */
	*r = '\0'; sprintf(output,"%s.%s",uri,ext); *r='.';

	/* run gnuplot */
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"R",output,true,0);
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
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"Scilab",output,true,0);
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
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"Octave",output,true,0);
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
#ifdef _WIN32
	sprintf(command,"wgnuplot %s",script);
#else
	sprintf(command,"gnuplot %s",script);
#endif
	/* temporary cut off of plt extension to build output file */
	*plt = '\0'; sprintf(output,"%s.%s",uri,ext); *plt='.';

	/* run gnuplot */
	IN_MYCONTEXT output_verbose("%s", command);
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
	return http_copy(http,"gnuplot",output,true,0);
}

/** Process an incoming runtime file request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_get_rt(HTTPCNX *http,char *uri)
{
	char fullpath[1024];
	char filename[1024];
	size_t pos = 0;
	if ( sscanf(uri,"%1023[^:]:%ld",filename,&pos)==0 )
		strncpy(filename,uri,sizeof(filename)-1);
	if (!find_file(filename,NULL,R_OK,fullpath,sizeof(fullpath)))
	{
		output_error("runtime file '%s' couldn't be located in GLPATH='%s'", filename,getenv("GLPATH"));
		return 0;
	}
	return http_copy(http,"runtime",fullpath,true,pos);
}

/** Process an incoming runtime file request
	@returns non-zero on success, 0 on failure (errno set)
 **/
int http_get_rb(HTTPCNX *http,char *uri)
{
	char fullpath[1024];
	if (!find_file(uri,NULL,R_OK,fullpath,sizeof(fullpath)))
	{
		output_error("binary file '%s' couldn't be located in GLPATH='%s'", uri,getenv("GLPATH"));
		return 0;
	}
	return http_copy(http,"runtime",fullpath,false,0);
}

/** Collect a KML documnent
    @returns non-zero on success, 0 on failure (errno set)
 **/
int http_kml_request(HTTPCNX *http, char *action)
{
//	static long long lock;
//	wlock(&lock);
	char *p = strchr(action,'?');
	http_type(http,"text/kml");
	if ( p==NULL )
	{	kml_dump(action); // simple dump of everything
		return http_copy(http,"KML",action,false,0);
	}
	else
	{
		char buffer[1024];
		char *propname = p+1; // now "property=value"
		OBJECT *obj;
		*p='\0'; // action is now the target object name
		obj = object_find_name(action);
		if ( obj==NULL )
		{
			http_status(http,HTTP_NOTACCEPTABLE);
			return 0;
		}
		p = strchr(propname,'=');
		object_get_value_by_name(obj,propname,buffer,sizeof(buffer));
		if ( p!=NULL )
		{	// set the value
			object_set_value_by_name(obj,propname,p+1);
		}
		return http_format(http,"%s",buffer);
	}
//	wunlock(&lock);
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
		output_verbose("main loop resume");
		exec_mls_resume(TS_NEVER);
		return 1;
	}
	else if ( strcmp(action,"pause")==0 )
	{
		output_verbose("main loop pause");
		exec_mls_resume(global_clock);
		return 1;
	}
	else if ( strcmp(action,"pause_wait")==0 )
	{
		output_verbose("main loop pause");
		exec_mls_resume(global_clock);
		output_verbose("waiting for pause");
		while ( global_mainloopstate!=MLS_PAUSED )
			usleep(100000);
		return 1;
	}
	else if ( sscanf(action,"pauseat=%[-0-9%:A-Za-z ]",buffer)==1 )
	{
		TIMESTAMP ts;
		http_decode(buffer);
		ts = convert_to_timestamp(buffer);
		if ( ts!=TS_INVALID )
		{
			output_verbose("main loop pause at %s", buffer);
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
		output_verbose("server shutdown by client");
		http_status(http,HTTP_OK);
		http_send(http);
		http_close(http);
		exit(XC_SUCCESS);
	}
	else if ( strcmp(action,"stop")==0 )
	{
		output_verbose("main loop stopped");
		global_stoptime = global_clock;
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
	return http_copy(http,"icon",fullpath,false,0);
}

/** Process an incoming request
	@returns nothing
 **/
void *http_response(void *ptr)
{
	SOCKET fd = *static_cast<SOCKET*>(ptr);
	HTTPCNX *http = http_create(fd);
	size_t len;
	int content_length = 0;
	char *user_agent = NULL;
	char *host = NULL;
	int keep_alive = 0;
	char *connection = NULL;
	char *accept = NULL;
	typedef enum {INTEGER,STRING} response_type;
	struct s_map {
		const char *name;
		response_type type;
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
			output_error("request [%s] is bad", request);
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
		IN_MYCONTEXT output_verbose("%s (host='%s', len=%d, keep-alive=%d)",http->query,host?host:"???",content_length, keep_alive);

		/* reject anything but a GET */
		if (stricmp(method,"GET")!=0)
		{
			http_status(http,HTTP_METHODNOTALLOWED);
			/* technically, we should add an Allow entry to the response header */
			output_error("request [%s %s %s]: '%s' is not an allowed method", method, uri, version, method);
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
			static struct s_path_map {
				const char *path;
				int (*request)(HTTPCNX*,char*);
				const char *success;
				const char *failure;
			} map[] = {
				/* this is the map of recognize request types */
				{"/control/",	http_control_request,	HTTP_ACCEPTED, HTTP_NOTFOUND},
				{"/open/",		http_open_request,		HTTP_ACCEPTED, HTTP_NOTFOUND},
				{"/raw/",		http_raw_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/xml/",		http_xml_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/gui/",		http_gui_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/output/",	http_output_request,	HTTP_OK, HTTP_NOTFOUND},
				{"/action/",	http_action_request,	HTTP_ACCEPTED,HTTP_NOTFOUND},
				{"/rt/",		http_get_rt,			HTTP_OK, HTTP_NOTFOUND},
				{"/rb/",		http_get_rb,			HTTP_OK, HTTP_NOTFOUND},
				{"/perl/",		http_run_perl,			HTTP_OK, HTTP_NOTFOUND},
				{"/gnuplot/",	http_run_gnuplot,		HTTP_OK, HTTP_NOTFOUND},
				{"/java/",		http_run_java,			HTTP_OK, HTTP_NOTFOUND},
				{"/python/",	http_run_python,		HTTP_OK, HTTP_NOTFOUND},
				{"/r/",			http_run_r,				HTTP_OK, HTTP_NOTFOUND},
				{"/scilab/",	http_run_scilab,		HTTP_OK, HTTP_NOTFOUND},
				{"/octave/",	http_run_octave,		HTTP_OK, HTTP_NOTFOUND},
				{"/kml/", 		http_kml_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/json/",		http_json_request,		HTTP_OK, HTTP_NOTFOUND},
				{"/find/",	http_find_request,	HTTP_OK, HTTP_NOTFOUND},
				{"/modify/",	http_modify_request,	HTTP_OK, HTTP_NOTFOUND},
				{"/read/",	http_read_request,	HTTP_OK, HTTP_NOTFOUND},
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

					/* keep-alive not desired*/
					if (connection && stricmp(connection,"close")==0)
						break;
					else
						continue;
				}
			}
			break;
		}
		break;
	}
	http_close(http);
	free(http);
	IN_MYCONTEXT output_verbose("socket %d closed",http->s);
	return 0;
}


