/** $Id$

 Object class header

 **/

#ifndef _UDP_H
#define _UDP_H

#include "gridlabd.h"
#include "connection.h"

#ifdef _WIN32
#ifdef int64
#undef int64 // wtypes.h uses the term int64
#endif
	#include <winsock2.h>
#define snprintf _snprintf
#ifndef int64
#define int64 __int64
#endif
#else
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/errno.h>
	#include <netdb.h>

	#ifdef INVALID_SOCKET
	#undef INVALID_SOCKET
	#endif

	#define INVALID_SOCKET (-1)
	#define SOCKET_ERROR (-1)
	#define int64 long long
#endif

class udp : public connection_transport {
public:

	// utilities
	inline CONNECTIONTRANSPORT get_transport() { return CT_UDP;};
	const char *get_transport_name(void) { return "udp";} ;
	int option(char *command);

	// read accessors
	unsigned int get_header_version(void) const;
	unsigned int get_header_size(void) const;
	const char *get_message_format(void) const;
	double get_message_version(void) const;
	timeval get_timeout(void) const;
	const char *get_hostname(void) const;
	unsigned int get_portnum(void) const;
	const char *get_uri(void) const;
	const char *get_errormsg(void) const;
	unsigned int get_debug_level(void) const;
	struct sockaddr_in *get_sockdata(size_t n=sizeof(struct sockaddr_in)) const;
	const char *get_buffer(void) const;
	size_t get_buffer_left(void) const;

	// write accessors
	void set_header_version(unsigned int n);
	void set_header_size(unsigned int n);
	void set_message_format(const char *s);
	void set_message_version(double x);
	void set_timeout(unsigned int n);
	void set_hostname(const char *s);
	void set_portnum(unsigned int n);
	void set_uri(const char *fmt, ...);
	void set_errormsg(const char *fmt, ...);
	void set_debug_level(unsigned int n);
	void set_sockdata(struct sockaddr_in *p, size_t n=sizeof(struct sockaddr_in));
	void set_output(const char *fmt, ...);
	void add_output(const char *fmt, ...);

private:
	// private data
	unsigned int header_version;
	unsigned int header_size;
	char message_format[6];
	double message_version;
	void *sockdata;
	char hostname[1024];
	unsigned int portnum;
	char uri[1024];
	char errormsg[1024];
	unsigned int debug_level;
	timeval timeout;
	SOCKET sd;

public:
	// construction
	udp();
	virtual ~udp();
	int create(void);
	int init(void);

	// event handlers 
	size_t send(const char *msg, const size_t len);
	size_t recv(char *buffer, const size_t maxlen);
	int call_setsockopt(SOCKET s, int level, int optname, timeval *optval, int optlen);
	void flush(void);
};

#endif // _UDP_H
