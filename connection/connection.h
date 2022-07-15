/// $Id$
/// @file connection.h
/// @addtogroup connection Connection module
/// 
/// The connection module implements general data exchange between gridlabd and other applications.
/// See http://sourceforge.net/apps/mediawiki/gridlab-d/index.php?title=Dev:Connection for details.
///
/// @{

#ifndef _CONNECTION_H
#define _CONNECTION_H

#include <stdarg.h>

#include "gridlabd.h"
#include "cache.h"
#include "transport.h"
#include "varmap.h"

#ifdef _WIN32
#define snprintf _snprintf
#endif

typedef int (EXCHANGETRANSLATOR)(connection_transport*, const char *tag, char *v, size_t vlen, int options);
#define ET_MATCHONLY 0
#define ET_GROUPOPEN 1
#define ET_GROUPCLOSE 2
#define ETO_NONE 0
#define ETO_QUOTES 3

///< Message flags for client_initiated and server_response
typedef enum {
	// NULL is used to terminate message part list
	MSG_CONTINUE = 0x01, ///< msg part of a group of message (automatically new if first in group)
	MSG_INITIATE, ///< open a new message group (if current group exists then gives warning/error)
	MSG_COMPLETE, ///< closes current message group
	MSG_TAG, ///< element tag (followed by tag name)
	MSG_SCHEMA, ///< group element is a schema (followed by VARMAP*)
	MSG_STRING, ///< group element is a string (followed by "name",maxsize,"value")
	MSG_REAL, ///< group element is a double (e.g. "name","%wfmt","%rfmt",ptr)
	MSG_INTEGER, ///< group element is an int64 (e.g. "name","%wfmt","%rfmt",ptr)
	MSG_DATA, ///< group element is gridlabd data (followed by VARMAP*)
	MSG_OPEN, ///< open a subgroup (followed by "name")
	MSG_CLOSE, ///< close a subgroup (nothing follows)
	MSG_CRITICAL, ///< flag message as critical (must preceded MSG_INITIATE to affect incoming traffic)
} MESSAGEFLAG;

///< Connection security flags
typedef enum {
	CS_NONE, ///< no security (everything allowed)
	CS_LOW, ///< low security (everything allowed unless explicity forbidden)
	CS_STANDARD, ///< standard security (obey PT_ACCESS except when forbidden)
	CS_HIGH, ///< high security (obey PT_ACCESS only when allowed)
	CS_EXTREME, ///< extreme security (everything forbidden unless explicitly permitted)
	CS_PARANOID, ///< paranoid security (#CS_EXTREME plus violation start lockout clock)
} CONNECTIONSECURITY; ///< level of security

///< Connection modes
typedef enum {
#ifndef CM_NONE // might already be defined in GDI
	CM_NONE, ///< no connection active (uninitialized connection)
#endif
	CM_CLIENT=CM_NONE+1, ///< server connection (responds to requests)
	CM_SERVER, ///< client connecdtion (initiates requests)
} CONNECTIONMODE; ///< type of connection (e.g., client, server)


//GLOBAL bool enable_subsecond_models INIT(false);
/// The connection_mode class provides control over connection-specific things
/// such as the mode (client/server), the transport (TCP/UCP) and the cache.
/// A connection is established when the native class is initialized.

class connection_mode {
public:
	// message handlers for child class
	virtual void error(const char *fmt, ...);
	virtual void warning(const char *fmt, ...);
	virtual void info(const char *fmt, ...);
	virtual void debug(int level, const char *fmt, ...);
	virtual void exception(const char *fmt, ...);

	// required implementation by child class
	virtual CONNECTIONMODE get_mode()=0;
	virtual const char *get_mode_name(void)=0; ///< get a string describing the transport type
	virtual int option(char *command)=0;

	// utilities
	static CONNECTIONMODE get_mode(const char *s);
	static connection_mode *new_instance(const char *options);

	EXCHANGETRANSLATOR *xlate_in;
	EXCHANGETRANSLATOR *xlate_out;
protected:
	connection_transport *transport;
	cache read_cache;
	cache write_cache;
	long long seqnum;
	int ignore_error;

public:
	connection_mode(void);
	int init(void);
	inline void set_transport(enumeration e) ///< change transport
	{
		set_transport((CONNECTIONTRANSPORT)e);
	};
	void set_transport(CONNECTIONTRANSPORT t);
	inline connection_transport *get_transport(void) { return transport;};
	void set_transport(const char *s); ///< change transport
	int update(VARMAP *var, DATAEXCHANGEDIRECTION dir, TRANSLATOR *xltr=NULL);
	int update(varmap *var, const char *tag, TRANSLATOR *xltr=NULL);
	int option(char *target, char *command);

	void set_translators(EXCHANGETRANSLATOR *out, EXCHANGETRANSLATOR *in, TRANSLATOR *data);

	cacheitem *create_cache(VARMAP*);

	int client_initiated(MESSAGEFLAG,...); ///< message group initiation (strings only)
	int server_response(MESSAGEFLAG,...); ///< message group response (strings only)
	//int client_initiated(char *tag, char *fmt, ...); ///< message item (formatted)
	int server_response(char *tag, char *fmt, ...); ///< message item (formatted)

	int exchange(EXCHANGETRANSLATOR*, bool critical);
	int exchange(EXCHANGETRANSLATOR*);
	int exchange(EXCHANGETRANSLATOR*, int64*id);
	int exchange(EXCHANGETRANSLATOR*, const char *tag, char *string); // tag/value exchange (match only)
	int exchange(EXCHANGETRANSLATOR*, const char *tag, size_t len, char *string); // string exchange (copy back)
	int exchange(EXCHANGETRANSLATOR*, const char *tag, double &real); // double exchange
	int exchange(EXCHANGETRANSLATOR*, const char *tag, int64 &integer); // int64 exchange
	int exchange_schema(EXCHANGETRANSLATOR*, cache *list); // schema exchange
	int exchange_data(EXCHANGETRANSLATOR*, cache *list); // data exchange
	int exchange_open(EXCHANGETRANSLATOR*, char *tag); // start tagged group exchange
	int exchange_close(EXCHANGETRANSLATOR*); // end group exchange

	int send(char *buf=NULL, size_t len=0); ///< send a raw payload
	int send(char *format, ...); ///< send a formatted payload
	int recv(char *buf=NULL, size_t len=0); ///< receive a raw payload
	int recv(char *format, ...); ///< receive a formatted payload

	int route_function(char*remotename,char*functionname,void*data,size_t datalen, int64 &result);
};

/* forward declarations of callback utilities */
typedef TIMESTAMP (*CLOCKUPDATE)(void*,TIMESTAMP);
//TODO: Move s_clockupdatelist to gridlabd.h
typedef struct s_clockupdatelist
{
	void *data;

	CLOCKUPDATE clkupdate;
	struct s_clockupdatelist *next;
} CLKUPDATELIST;

void add_clock_update(void *data, CLOCKUPDATE clkupdate);

typedef SIMULATIONMODE (*DELTAINTERUPDATE)(void *, unsigned int dIntervalCounter, TIMESTAMP t0, unsigned int64 dt);
typedef struct s_deltainterupdatelist
{
	void *data;
	DELTAINTERUPDATE deltainterupdate;
	struct s_deltainterupdatelist *next;
}DELTAINTERUPDATELIST;

void register_object_interupdate(void *data, DELTAINTERUPDATE deltainterupdate);

typedef SIMULATIONMODE(*DELTACLOCKUPDATE)(void*, double, unsigned long, SIMULATIONMODE);
typedef struct s_deltaclockupdatelist
{
	void *data;
	DELTACLOCKUPDATE dclkupdate;
	struct s_deltaclockupdatelist *next;
}DELTACLOCKUPDATELIST;

void register_object_deltaclockupdate(void *data, DELTACLOCKUPDATE deltaClockUpdate);
#endif /// @}

