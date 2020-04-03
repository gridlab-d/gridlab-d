#include "udpconnection.h"

udpconnection::udpconnection(const string &ip,const int  &port) : absconnection(ip,port)
{
#ifdef _WIN32
	// init socket system
	WORD wsaVersion = MAKEWORD(2,2);
	WSADATA wsaData;
	if ( WSAStartup(wsaVersion,&wsaData)!=0 )
	{
		connectionOK=false;
		
	}	
#endif
	this->sd=socket(AF_INET,SOCK_DGRAM,0);
	if(this->sd<0)
		connectionOK=false;
	
	this->togld.sin_family=AF_INET;
	this->togld.sin_port=htons(this->port);
	this->togld.sin_addr.s_addr =inet_addr(this->ip.c_str());
	connectionOK=true;
}

udpconnection::~udpconnection(void)
{
	if(connectionOK){
		close();
	}
}

int udpconnection::recv(char * buffer,const int &size){
	if(!connectionOK)
		return -1;

	struct sockaddr_in serv_addr;
	int slen = sizeof(serv_addr);

	return recvfrom(sd,buffer,size,0,(struct sockaddr*)&serv_addr,(socklen_t*)&slen);
}

int udpconnection::send(const char *buffer,const int &size){
	if(!connectionOK)
		return -1;


	return sendto(sd,buffer,size,0,(struct sockaddr*)&this->togld,sizeof(this->togld));
}

//connect operation not supported in udp, so we return 1.
int udpconnection::connect(int retries){

	if(!connectionOK)
		return 0;
	return 1;
}

void udpconnection::close(){
	if(!connectionOK)
		return;

	#ifdef _WIN32
		closesocket(this->sd);
	#else
		close(this->sd);
	#endif
	this->connectionOK=false;
}

int udpconnection::getErrorCode()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
}
