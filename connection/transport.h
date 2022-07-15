/// $Id$
/// @file transport.h
/// @addtogroup connection
/// @{

#ifndef _TRANSPORT_H
#define _TRANSPORT_H

#include "socket.h"

typedef enum {
	CT_NONE=0, ///< no transport specified (uninitialized transport)
	CT_UDP=1, ///< UDP transport
	CT_TCP=2, ///< TCP transport
} CONNECTIONTRANSPORT;

class connection_transport {
protected:
	int maxmsg;
	char input[2048];
	char output[2048];
	int position;
	int field_count;
	char *delimiter;
	typedef enum {TE_ABORT, TE_RETRY, TE_IGNORE} TRANSPORTERROR;
	TRANSPORTERROR on_error;
	int maxretry;
public:
	// message handlers for child class
	virtual void error(const char *fmt, ...);
	virtual void warning(const char *fmt, ...);
	virtual void info(const char *fmt, ...);
	virtual void debug(int level, const char *fmt, ...);
	virtual void exception(const char *fmt, ...);

	// required implementation by child class
	virtual int create(void)=0;
	virtual int init(void)=0; ///< initialization of transport
	virtual CONNECTIONTRANSPORT get_transport()=0; ///< get the type of transport
	virtual const char *get_transport_name(void)=0; ///< get a string describing the transport type
	virtual int option(char *command)=0; ///< set a transport option
	virtual size_t send(const char *msg, const size_t len)=0; // send message
	virtual size_t recv(char *buffer, const size_t maxlen)=0; // recv message
	virtual void set_message_format(const char *s)=0;
	virtual void set_message_version(double x)=0;

	// utilities
	static CONNECTIONTRANSPORT get_transport(const char *s); ///< get the type of transport from a name
	static connection_transport *new_instance(CONNECTIONTRANSPORT e); ///< create a new instance of a transport

	// message queue
	bool message_open();
	bool message_close();
	bool message_continue();
	int message_append(const char *fmt,...);
	inline void set_delimiter(const char *d) { delimiter=const_cast<char*>(d);};
	inline void reset_fieldcount(void) { field_count=0;};

	// buffer access
	char *get_input() { return input; };
	char *get_output() { return output; };
	size_t get_position() { return position; };
	size_t get_size() { return maxmsg - position; };

	// translation control (incoming only)
protected:
	void *translation; // translation result
	void *(*translator)(char*,void*); // callback function to create/replace translation
public:
	inline void set_translator(void *(*fnc)(char*,void*)) { translator=fnc;}; // set the translator function
	inline void *get_translation(void) { return translation; };
	inline void set_translation(void *p) { translation=p; };

	// error handling
	inline void onerror_abort() { on_error = TE_ABORT; };
	inline void onerror_retry() { on_error = TE_RETRY; };
	inline void onerror_ignore() { on_error = TE_IGNORE; };
	inline TRANSPORTERROR get_onerror(void) { return on_error; };
	inline int get_maxretry() { return maxretry;};
	inline void set_maxretry(int n=-1) { maxretry = n; };
public:
	connection_transport(void);
	virtual ~connection_transport(void);
};

#endif /// @} _TRANSPORT_H
