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

void server_request(int);	// Function to handle clients' request(s)
void handleRequest(SOCKET newsockfd);

#include "server.h"
#include "output.h"
#include "globals.h"
#include "object.h"

#include "legal.h"

void *server_routine(int sockfd)
{
	struct sockaddr_in cli_addr;
	SOCKET newsockfd;
	int clilen;

	// repeat forever..
	while (1)
	{
		clilen = sizeof(cli_addr);

		/* accept client request and get client address */
		newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
		output_verbose("accepting incoming connection from %d.%d.%d.%d on port %d", cli_addr.sin_addr.S_un.S_un_b.s_b1,cli_addr.sin_addr.S_un.S_un_b.s_b2,cli_addr.sin_addr.S_un.S_un_b.s_b3,cli_addr.sin_addr.S_un.S_un_b.s_b4,cli_addr.sin_port);
		if (newsockfd<0 && errno!=EINTR)
			output_error("server accept error on fd=%d: %s", sockfd, strerror(errno));
		else if (newsockfd > 0)
		{
			handleRequest(newsockfd);
#ifdef WIN32
			closesocket(newsockfd);
#else
			close(newsockfd);
#endif
		}
	}
}

STATUS server_startup(int argc, char *argv[])
{
	int portNumber = global_server_portnum;
	SOCKET sockfd;
	int childpid;
	struct sockaddr_in serv_addr;
	int status;
	char buf[MAXSTR],ack[MAXSTR];
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

#ifdef WIN32
	memset(&serv_addr,0,sizeof(serv_addr));
#else
	bzero((char *)&serv_addr,sizeof(serv_addr)) ;
#endif

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNumber);

	/* bind socket to server address */
	if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		output_error("can't bind to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
		return FAILED;
	}
	output_verbose("bind ok to %d.%d.%d.%d",serv_addr.sin_addr.S_un.S_un_b.s_b1,serv_addr.sin_addr.S_un.S_un_b.s_b2,serv_addr.sin_addr.S_un.S_un_b.s_b3,serv_addr.sin_addr.S_un.S_un_b.s_b4);
	
	/* listen for connection */
	listen(sockfd,5);
	output_verbose("server listening to port %d", portNumber);

	/* start the server thread */
	if (pthread_create(&thread,NULL,server_routine,sockfd))
	{
		output_error("server thread startup failed");
		return FAILED;
	}
	output_verbose("server thread started, switching to realtime mode");
	global_run_realtime = 1;
	return SUCCESS;
}

int html_response(void *ref, char *format, ...)
{
	int len;
	char html[65536];
	SOCKET s = (SOCKET)ref;
	va_list ptr;

	va_start(ptr,format);
	len = vsprintf(html,format,ptr);
	va_end(ptr);

#ifdef WIN32
	len = send(s,html,strlen(html),0);
#else
	len = write(newsockfd,html,strlen(html));
#endif	
	return len;
}

void handleRequest(SOCKET newsockfd) 
{
	char input[MAXSTR],output[MAXSTR];
	char method[64]="";
	char name[1024]="";
	char property[1024]="";
	char value[1024]="";
#define HTTP_OK 200
#define HTTP_BADREQUEST 400
#define HTTP_FORBIDDEN 403
#define HTTP_NOTFOUND 404
	int code=HTTP_OK;
	int len;
	int query_items;

	/* read socket */
#ifdef WIN32
	len = recv(newsockfd,(char*)input,MAXSTR,0);
#else
	len = read(newsockfd,(void *)input,MAXSTR);
#endif
	if (len<0)
	{
		output_error("socket read failed: error code %d",WSAGetLastError());
		return;
	}
	input[len]='\0';

	/* @todo process input and write to output */
	output_verbose("received incoming request [%s]", input);
	query_items = sscanf(input,"%63s /%1023[^/ ]/%1023[^= ]=%1023s",method,name,property,value);
	if (strcmp(method,"GET")==0 && strcmp(name,"gui")==0)
	{	
		char html[65536];
		int i=0;
		int len;
		gui_set_html_stream(newsockfd,html_response);
		if (gui_html_output_page(property)<0)
		{
			sprintf(html,"HTTP/1.1 %d Not Found\n"
				"Connection: close"
				"\nContent-type: text/html\n\n",
				HTTP_NOTFOUND, REV_MAJOR, REV_MINOR, REV_PATCH, BRANCH);
#ifdef WIN32
			len = send(newsockfd,html,strlen(html),0);
#else
			len = write(newsockfd,html,strlen(html));
#endif
		}
		return;
	}

	/* XML response */
	switch (query_items) {
	case 2:
		output_verbose("2 terms received (method='%s', name='%s'", method, name);
		if (strcmp(method,"GET")==0)
		{
			char buf[1024]="";
			output_verbose("getting global '%s'", name);
			if (global_getvar(name,buf,sizeof(buf))==NULL)
			{
				output_verbose("variable '%s' not found", name);
				code=HTTP_NOTFOUND;
			}
			else
				output_verbose("got %s=[%s]", name,buf);
			sprintf(output,"%s\n",buf);
		}
		else if (strcmp(method,"POST")==0)
		{
			output_error("POST not supported yet");
			code=HTTP_BADREQUEST;
			sprintf(output,"%s\n","POST not supported yet");			
		}
		else
		{
			code = HTTP_BADREQUEST;
			sprintf(output,"%s\n","method not supported");
		}
		break;
	case 3:
		output_verbose("3 terms received (method='%s', name='%s', property='%s'", method, name, property);
		if (strcmp(method,"GET")==0)
		{
			char buf[1024]="";
			OBJECT *obj;
			char *id = strchr(name,':');
			output_verbose("getting object '%s'", name);
			if (id==NULL)
				obj = object_find_name(name);
			else
				obj = object_find_by_id(atoi(id+1));
			if (obj==NULL)
			{
				output_verbose("object '%s' not found", name);
				code = HTTP_NOTFOUND;
				sprintf(output,"%s\n","object not found");
				break;
			}
			if (object_get_value_by_name(obj,property,buf,sizeof(buf)))
			          output_debug("set %s.%s=%s", name,property,value);
			sprintf(output,"%s\n",buf);
		}
		else if (strcmp(method,"POST")==0)
		{
			output_error("POST not supported yet");
			code = HTTP_BADREQUEST;
			sprintf(output,"%s\n","POST not supported yet");			
		}
		else	
		{
			code = HTTP_BADREQUEST;
			sprintf(output,"%s\n","method not supported");	
			}
		break;
	case 4:
		output_verbose("4 terms received (method='%s', name='%s', property='%s', value='%s'", method, name, property, value);
		if (strcmp(method,"GET")==0)
		{
			char buf[1024]="";
			OBJECT *obj;
			char *id = strchr(name,':');
			output_verbose("getting object '%s'", name);
			if (id==NULL)
				obj = object_find_name(name);
			else
				obj = object_find_by_id(atoi(id+1));
			if (obj==NULL)
			{
				output_verbose("object '%s' not found", name);
				sprintf(output,"%s\n","object not found");
				code = HTTP_NOTFOUND;
				break;
			}
			if (strcmp(value,"")!=0)
			{
				output_verbose("set %s.%s=[%s]", name, property, value);
				if (!object_set_value_by_name(obj,property,value))
				{
					code = HTTP_FORBIDDEN;
					output_verbose("set failed!");
				}
			else
			      output_debug("set %s.%s=%s", name,property,value);
			}
			if (object_get_value_by_name(obj,property,buf,sizeof(buf)))
				output_verbose("got %s.%s=[%s]", name,property,buf);
			sprintf(output,"%s\n",buf);
		}
		else if (strcmp(method,"POST")==0)
		{
			code = HTTP_BADREQUEST;
			output_error("POST not supported yet");
			sprintf(output,"%s\n","POST not supported yet");			
		}
		else
		{
			code = HTTP_BADREQUEST;
			sprintf(output,"%s\n","method not supported");
		}
		break;
	default:
		code = HTTP_BADREQUEST;
		sprintf(output,"%s\n","invalid query");
		break;
	}

	/* write response */
	{	char xml[1024];
		int i=0;
		if (strcmp(property,"")==0)
			sprintf(xml,"HTTP/1.1 %d OK\n"
				"Server: gridlabd %d.%d.%d (%s) \n"
				"Connection: close"
				"\nContent-type: text/xml\n\n"
				"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
				"<resultset>\n"
				"\t<global>%s</global>\n"
				"\t<answer>%s</answer>\n"
				"</resultset>\n", 
				code, 
				REV_MAJOR, REV_MINOR, REV_PATCH, BRANCH, 
				name,output);
		else
			sprintf(xml,"HTTP/1.1 %d OK\n"
				"Server: gridlabd %d.%d.%d (%s) \n"
				"Connection: close"
				"\nContent-type: text/xml\n\n"
				"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
				"<resultset>\n"
				"\t<object>%s</object>\n"
				"\t<property>%s</property>\n"
				"\t<answer>%s</answer>\n"
				"</resultset>\n", 
				code, 
				REV_MAJOR, REV_MINOR, REV_PATCH, BRANCH, 
				name,property,output);
#ifdef WIN32
		send(newsockfd,xml,strlen(xml),0);
#else
		write(newsockfd,xml,strlen(xml));
#endif
	}
	output_verbose("response [%s] sent", output);
}
