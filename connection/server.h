/// $Id$
/// @file server.h
/// @addtogroup connection
/// @{

#ifndef _SERVER_H
#define _SERVER_H

#include "gridlabd.h"
#include "connection.h"

#include <pthread.h>

class RemoteSocket : public Socket {
private:
	class server *svr;
	pthread_t proc;
public:
	inline RemoteSocket(class server*s) : svr(s) { };
	inline class server *get_server(void) { return svr; };
	inline void set_proc(pthread_t p) { proc=p;};
	inline pthread_t *get_proc(void) { return &proc; };
};

class server : public connection_mode {
public:
	typedef enum {STARTING=0,READY=1,SHUTDOWN=2} SERVERSTATUS;
private:
	SERVERSTATUS status;
	Socket local; ///< socket info
	int type; ///< socket type (SOCK_DGRAM or SOCK_STREAM)
	int maxclients; ///< maximum clients allowed
	int numactive; ///< number of active clients
	pthread_t handler;

public:
	/* required implementations */
	server(void);
	~server(void);
	int create(void);
	int init(void);
	// TODO add other event handlers here

	// utilities
	CONNECTIONMODE get_mode() { return CM_SERVER;};
	const char *get_mode_name(void) { return "server"; }; ///< get a string describing the mode
	int option(char *command);
public:
	void accept_tcp(void);
	void accept_udp(void);
	void process_msg(Socket&);
public:
	inline SERVERSTATUS get_status(void) { return status; };
	inline Socket &get_socket(void) { return local; };
	inline int get_type(void) { return type; };
	inline int get_maxbacklog() { return maxclients-numactive; };
public:
	void set_type(int);
};

#endif /// @} _SERVER_H
