/* $Id$ 
 * server.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <sys/errno.h>

#define MAXSTR		1024		// maximum string length
#define PORTNUM		80		// default port (httpd port)

void server_request(int);	// Function to handle clients' request(s)

#include "server.h"
#include "output.h"
#include "globals.h"

STATUS server_startup(int argc, char *argv[])
{
	int portNumber = PORTNUM;
	int sockfd,newsockfd,clilen,childpid;
	struct sockaddr_in serv_addr,cli_addr;
	int status;
	char buf[MAXSTR],ack[MAXSTR];

	/* create a new socket */
	if ((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0)
	{
		output_error("can't open stream socket");
		return FAILED;
	}

	bzero((char *)&serv_addr,sizeof(serv_addr)) ;

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(portNumber);

	/* bind socket to server address */
	if (bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
	{
		output_error("can't bind local address");
		return FAILED;
	}
	
	/* listen for connection */
	listen(sockfd,5);
	output_verbose("server listening to port %d", portNumber);

	
	// repeat forever..
	while (1)
	{
		clilen = sizeof(cli_addr);

		/* accept client request and get client address */
		newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);
		if (newsockfd<0 && errno!=EINTR)
			output_error("server accept error: %s", strerror(errno));
		else if (newsockfd > 0)
		{
			handleRequest(newsockfd);
			close(newsockfd);
		}
	}
}


void handleRequest(int newsockfd) 
{
	char input[MAXSTR],output[MAXSTR];
	char method[64];
	char query[1024];
	char xml[] = "Content-type: text/xml\n\n<xml>%s</xml>\n";

	/* read socket */
	read(newsockfd,(void *)input,MAXSTR);

	/* @todo process input and write to output */
	output_verbose("received incoming request [%s]", input);
	if (sscanf(input,"%63s %1023s",method,query)==2)
	{
		if (strcmp(method,"GET")==0)
		{
			char buf[1024];
			sprintf(output,xml,global_getvar(query,buf,sizeof(buf)));
		}
		else	
			sprintf(output,xml,"GET not supported");
	}
	else
		sprintf(output,xml,"invalid query");

	/* write response */
	write(newsockfd,output,strlen(output));
}