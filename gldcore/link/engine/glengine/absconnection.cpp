#include "absconnection.h"

#include "udpconnection.h"

absconnection::absconnection(const string &toIp,const int &toPort)
:connectionOK(false),
 ip(toIp),
 port(toPort),
 protocol("UNKNOWN")
{
}

absconnection::~absconnection()
{
}

absconnection *absconnection::getconnectionTCP(const string &toIP,const int &toPort){
	return NULL;
}

absconnection *absconnection::getconnectionUDP(const string &toIP,const int &toPort){
	return new udpconnection(toIP,toPort);
}

absconnection *absconnection::getconnection(ENGINE_SOCK_TYPE socktype,const string &toIP,const int &toPort){
	switch(socktype){
		case UDP:
			return new udpconnection(toIP,toPort);
		case TCP:
			return NULL;
		deafult:
			return NULL;
	}
	return NULL;
}

const string absconnection::getProtocolStr(){
	stringstream ss;

	ss << protocol << "://" << ip << ":" << port;

	return ss.str();
}

const char *absconnection::getErrorMessage()
{
	int errorCode=this->getErrorCode();
#ifdef _WIN32
	switch (errorCode) {
	case 6: return "invalid handle";
	case 8: return "insufficient memory";
	case 87: return "invalid parameter";
	case 995: return "operation aborted";
	case 996: return "imcomplete operation";
	case 997: return "pending operation";
	case 10004: return "interrupted call";
	case 10009: return "invalid file handle";
	case 10013: return "access denied";
	case 10014: return "bad address";
	case 10022: return "invalid argument";
	case 10024: return "too many open files";
	case 10035: return "resource temporarily unavailable";
	case 10036: return "operation now in progress";
	case 10037: return "operation already in progress";
	case 10038: return "operation not on a socket";
	case 10039: return "no destination address";
	case 10040: return "message too long";
	case 10041: return "wrong protocol for socket";
	case 10042: return "bad protocol option";
	case 10043: return "protocol not supported";
	case 10044: return "socket type not supported";
	case 10045: return "operation not supported";
	case 10046: return "protocol family not supported";
	case 10047: return "address family not supported";
	case 10048: return "address already in use";
	case 10049: return "cannot assigned requested address";
	case 10050: return "network is down";
	case 10051: return "network is unreachable";
	case 10052: return "connection dropped on reset";
	case 10053: return "connection aborted by software";
	case 10054: return "connection reset by remote peer";
	case 10055: return "no buffer space available";
	case 10056: return "socket already connected";
	case 10057: return "socket is not connected";
	case 10058: return "socket shut down";
	case 10059: return "too many references";
	case 10060: return "connection timed out";
	case 10061: return "connection refused";
	case 10062: return "cannot translate name";
	case 10063: return "name too long";
	case 10064: return "host is down";
	case 10065: return "no route to host";
	case 10066: return "directory not empty";
	case 10067: return "too many processes";
	case 10068: return "user quota exceeded";
	case 10069: return "disk quota exceeded";
	case 10070: return "stale file handle";
	case 10071: return "item not available locally";
	case 10091: return "network subsystem not available";
	case 10092: return "socket version not supported";
	case 10093: return "sockets not yet initialized";
	case 10101: return "shutdown in progress";
	case 10102: return "no more results";
	case 10103: return "call cancelled";
	case 10104: return "procedure call table full";
	case 10105: return "service provider is invalid";
	case 10106: return "service provider initialization failed";
	case 10107: return "system call failure";
	case 10108: return "service not found";
	case 10109: return "class type not found";
	case 10110: return "no more results";
	case 10111: return "call cancelled";
	case 10112: return "database query refused";
	case 11001: return "host not found";
	case 11002: return "authority not responding";
	case 11003: return "unspecified unrecoverable error";
	case 11004: return "no data record";
	case 11005: return "QoS receiver arrived";
	case 11006: return "QoS sender arrived";
	case 11007: return "no QoS senders";
	case 11008: return "no QoS receivers";
	case 11009: return "QoS request confirmed";
	case 11010: return "QoS admission error";
	case 11011: return "QoS policy failure";
	case 11012: return "QoS style invalid";
	case 11013: return "QoS object invalid";
	case 11014: return "QoS traffic control error";
	case 11015: return "QoS general error";
	case 11016: return "QoS service type error";
	case 11017: return "QoS flow spec error";
	case 11018: return "QoS provider buffer invalid";
	case 11019: return "QoS filter style invalid";
	case 11020: return "QoS filter type invalid";
	case 11021: return "QoS filter count invalid";
	case 11022: return "QoS object length invalid";
	case 11023: return "QoS flow count incorrect";
	case 11024: return "QoS object unrecognized";
	case 11025: return "QoS object policy invalid";
	case 11026: return "QoS flow description invalid";
	case 11027: return "QoS provider flow spec invalid";
	case 11028: return "QoS provider filter spec invalid";
	case 11029: return "QoS shape discard mode object invalid";
	case 11030: return "QoS shape rate object invalid";
	case 11031: return "QoS element type reserved policy";
	default: return "not a socket error";
	}
#else
	return strerror(code);
#endif
}
