// $Id$
// 
// Implements the server connection mode
//

#include <cstring>

#include "server.h"

extern CONNECTIONSECURITY connection_security;
extern double connection_lockout;

static void* tcp_handler(void *arg)
{
	server *srv = (server*)arg;
	while ( srv->get_status()==server::READY )
		srv->accept_tcp();
	return 0;
}

static void* udp_handler(void *arg)
{
	server *srv = (server*)arg;
	while ( srv->get_status()==server::READY )
		srv->accept_udp();
	return 0;
}

static void* msg_handler(void *arg)
{
	RemoteSocket *client = (RemoteSocket *)arg;
	client->get_server()->process_msg(*client);
	delete client;
	return 0;
}

server::server(void)
: local()
{
	type = 0;
	maxclients = 0; // no limit
	status = STARTING;
}

server::~server(void)
{
	status = SHUTDOWN;
	// TODO wait for threads?
}

int server::option(char *command)
{
	if ( sscanf(command,"maxclients %d",&maxclients)==1 )
		return 1;
	// TODO add other options here (ignore unrecognized options)
	return 0;
}

int server::create(void) 
{
	switch (type) {
	case SOCK_STREAM: // tcp
		local = Socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
		break;
	case SOCK_DGRAM: // udp
		local = Socket(PF_INET,SOCK_DGRAM,IPPROTO_UDP);
		break;
	default:
		gl_error("unsupported/unspecified socket type");
		return 0;
	}

	// default socket info
	local.set_family(AF_INET);
	local.set_addr(connection_security<CS_STANDARD?INADDR_ANY:INADDR_LOOPBACK);
	local.set_port(3306); // per ICAN
	return 1; /* return 1 on success, 0 on failure */
}

int server::init(void)
{
	// bind to address
	if ( local.bind()<0 )
	{
		gl_error("%s: unable to bind to address", local.get_saddr());
		return 0;
	}

	// maximum clients set for more secure systems
	if ( connection_security>CS_STANDARD && maxclients==0 ) maxclients = 64;

	if ( type==SOCK_STREAM )
	{
		if ( local.listen(maxclients)!=0 )
		{
			gl_error("%s: unable to listen for incoming connections", local.get_saddr());

		}
		else
			status = READY;

		// start handler
		if ( pthread_create(&handler,NULL,tcp_handler,(void*)this)!=0 )
		{
			gl_error("%s: unable to start tcp handler", local.get_saddr());
			return 0;
		}
	}
	else if ( type==SOCK_DGRAM )
	{
		status = READY;
		if ( pthread_create(&handler,NULL,udp_handler,(void*)this)!=0 )
		{
			gl_error("%s: unable to start udp handler", local.get_saddr());
			return 0;
		}
	}

	return 1;
}

void server::accept_tcp(void)
{
	RemoteSocket *client = new RemoteSocket(this);
	
	// accept next incoming connection
	if ( client->accept(local) )
	{
		// process incoming messages as they arrive
		if ( pthread_create(client->get_proc(),NULL,msg_handler,(void*)client)!=0 )
			gl_error("%s: unable to start stream handler for %s", local.get_saddr(), client->get_saddr());
	}

	// only an error if not interrupted
	else if ( client->get_lasterror()!=EINTR )
		gl_error("%s: server accept error %d", local.get_saddr(), client->get_lasterror()); 
}

void server::accept_udp(void)
{
	RemoteSocket *client = new RemoteSocket(this);

	// wait/peek at incoming message
	int infosize = client->get_infosize();
	switch ( ::recvfrom(local.get_socket(),NULL,0,MSG_PEEK,(struct sockaddr*)client->get_info(),(socklen_t*)&infosize) ) {
	case 0:
		// connection gracefully closed
		break;
	case SOCKET_ERROR:
		// TODO handle connection error
		break;
	default:
		// begin processing incoming messages as they arrive
		if ( pthread_create(client->get_proc(),NULL,msg_handler,(void*)client)!=0 )
			gl_error("%s: unable to start datagram handler", local.get_saddr());
		break;
	}
	return;
}

void server::process_msg(Socket &client)
{
	char buffer[4096];
	int infosize = client.get_infosize();
	int len = ::recvfrom(local.get_socket(),buffer,sizeof(buffer),0,(struct sockaddr*)client.get_info(),(socklen_t*)&infosize);
	if ( len>0 )
	{
		// TODO dispatch message to protocol handlers
	}
}

void server::set_type(int t)
{
	if ( type!=0 ) throw "server::set_type(int): unable to set socket type after server is created";
	type = t;
}

