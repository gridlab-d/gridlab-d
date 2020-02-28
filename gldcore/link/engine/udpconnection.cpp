#include "engine.h"

typedef struct s_udp_socket_data{
	SOCKET_TYPE type;
	SOCKET sd;
	unsigned short port;
	unsigned long addr;
	struct sockaddr_in receiver;
	struct sockaddr_in sender;
} UDP_SOCKET_DATA;

bool setupUDPSocket(ENGINELINK *given){
	UDP_SOCKET_DATA *data=(UDP_SOCKET_DATA*)malloc(sizeof(UDP_SOCKET_DATA));
	memset(data,0,sizeof(UDP_SOCKET_DATA));
#ifdef _WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2,0),&wsaData)!=0)
	{
		// TODO collect error info
		gl_error("WSA initiatialization failed");
		return false;	
	}
#endif

	data->sd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	if ( data->sd==INVALID_SOCKET )
	{
		// TODO collect error info
		gl_error("unable to create socket");
		return false;
	}

	data->type=UDP;
	data->port = 6267;
	data->addr = INADDR_ANY;

	given->sd=(SOCKET_DATA*)data;
	return true;
}

bool bindUDPSocket(ENGINELINK *given){

	if(given->sd->type!=UDP){
		return false;
	}
	
	UDP_SOCKET_DATA *data=(UDP_SOCKET_DATA*)given->sd;
    // bind socket to specified address and port
	data->receiver.sin_family = AF_INET;
	data->receiver.sin_port = htons(data->port);
	data->receiver.sin_addr.s_addr = htonl(data->addr);
	if ( bind(data->sd, (const struct sockaddr*)&data->receiver, sizeof(data->receiver))==SOCKET_ERROR )
	{
		// TODO collect error info
		gl_error("socket bind failed");
		return false;
	}
	else
		gl_debug("socket bind to addr %ld port %d ok", ntohl(data->receiver.sin_addr.s_addr), ntohs(data->receiver.sin_port));

	return true;
}

bool acceptUDPSocket(ENGINELINK *given){
	if(given->sd->type!=UDP)
		return false;

	//no accept in UDP
	return true;
}

int recvUDPSocket(ENGINELINK *engine, char *buffer, int maxlen){

	if(engine->sd->type!=UDP){
		return -1;
	}
	UDP_SOCKET_DATA *data=(UDP_SOCKET_DATA*)engine->sd;
	int len = sizeof(data->sender);

	int rv = recvfrom(data->sd,buffer,maxlen,0,(struct sockaddr*)&data->sender,(socklen_t*)&len);
    if ( rv==SOCKET_ERROR )
    {
	    // TODO collect error info
	    gl_error("recvfrom timeout");
	    return -1;
    }
	if(rv==maxlen) //lose data we filled buffer!
		buffer[rv-1]='\0';
	else
		buffer[rv]='\0';
	
	gl_debug("incoming message from %d.%d.%d.%d/%d [%s]", 
		get_addr(&data->sender,1),
		get_addr(&data->sender,2),
		get_addr(&data->sender,3),
		get_addr(&data->sender,4),
		get_port(&data->sender), buffer);
	return rv;
}

int sendUDPSocket(ENGINELINK *engine, char *buffer,int len){

	if(engine->sd->type!=UDP){
		return -1;
	}
	UDP_SOCKET_DATA *data=(UDP_SOCKET_DATA*)engine->sd;

	gl_debug("outgoing message to %d.%d.%d.%d/%d [%s]", 
		get_addr(&engine->sender,1),
		get_addr(&engine->sender,2),
		get_addr(&engine->sender,3),
		get_addr(&engine->sender,4),
		get_port(&engine->sender), buffer);
      int rv;
     // rv = sendto(engine->sd,(char*)&len,sizeof(size_t),0,(const struct sockaddr*)&engine->sender,sizeof(engine->sender));
      rv = sendto(data->sd,buffer,len,0,(const struct sockaddr*)&data->sender,sizeof(data->sender));
      if ( rv==SOCKET_ERROR )
      {
	      // TODO collect error info
	      gl_error("sendto error");
	      return -1;
      }
      
      return len;
}