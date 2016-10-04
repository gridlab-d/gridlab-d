/// $Id$
/// @file native.h
/// @addtogroup connection
/// @{

#ifndef _NATIVE_H
#define _NATIVE_H

#include "gridlabd.h"
#include "varmap.h"
#include "connection.h"

class native;

///< Function relays
typedef struct s_functionrelay {
	const char *localclass; ///< local class info
	const char *localcall; ///< local function call address (NULL for outgoing)
	char remoteclass[64]; ///< remote class name
	char remotename[64]; ///< remote function name
	native *route; ///< routing of relay
	TRANSLATOR *xlate; ///< output translation call
	struct s_functionrelay *next;
	DATAEXCHANGEDIRECTION drtn;
	COMMUNICATIONTYPE ctype; ///<identifies which fncs communication function to call. Only used for communicating with fncs.
} FUNCTIONRELAY;
extern "C" FUNCTIONADDR add_relay_function(native *route, const char *fclass,const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction);
extern "C" FUNCTIONRELAY *find_relay_function(const char *rname, const char *rclass);

class native : public gld_object {
public:
	GL_ATOMIC(enumeration,mode);
	GL_ATOMIC(enumeration,transport);
	GL_ATOMIC(double,timestep);
private:
	connection_mode *m;
	typedef enum {NONE,ALLOW,FORBID,
		INIT,PRECOMMIT,PRESYNC,SYNC,POSTSYNC,COMMIT,PRENOTIFY,POSTNOTIFY,FINALIZE,PLC,TERM,
		/* this is always last */_NUMVMI,		
		_FIRST=INIT, _LAST=TERM, /* determine which are part of schema */
	} VARMAPINDEX;
	varmap *map[_NUMVMI];
public:
	inline connection_mode *get_connection(void) { return m;};
	inline void set_connection(connection_mode *p) { if (m) delete m; m = p;};
	inline void new_connection(const char *command) { if (m) delete m; m = connection_mode::new_instance(command);};
protected:
	VARMAPINDEX get_varmapindex(const char *);
	int parse_function(const char *specs);
public:
	/* required implementations */
	native(MODULE*);
	int create(void);
	int init(OBJECT*,TRANSLATOR *xlate=NULL);
	int precommit(TIMESTAMP,TRANSLATOR *xlate=NULL);
	TIMESTAMP presync(TIMESTAMP,TRANSLATOR *xlate=NULL);
	TIMESTAMP sync(TIMESTAMP,TRANSLATOR *xlate=NULL);
	TIMESTAMP postsync(TIMESTAMP,TRANSLATOR *xlate=NULL);
	TIMESTAMP commit(TIMESTAMP,TIMESTAMP,TRANSLATOR *xlate=NULL);
	int prenotify(PROPERTY*,char*,TRANSLATOR *xlate=NULL);
	int postnotify(PROPERTY*,char*,TRANSLATOR *xlate=NULL);
	int finalize(TRANSLATOR *xlate=NULL);
	TIMESTAMP plc(TIMESTAMP,TRANSLATOR *xlate=NULL);
	// TODO add other event handlers here
	void term(TIMESTAMP,TRANSLATOR *xlate=NULL);
	int64 function(char*remotename,char*functionname,void*data,size_t datalen,TRANSLATOR *xlate_out=NULL, TRANSLATOR *xlate_in=NULL);
public:
	int link(char *value);
	int option(char *value);
	int option(char *target, char *command);
	inline int get_firstmap(void) { return (int)_FIRST; };
	inline int get_lastmap(void) { return (int)_LAST; };
	inline VARMAP *get_varmap(int n) { return map[n]->getfirst(); };
	inline varmap *get_initmap(void) { return map[INIT];};
public:
	static CLASS *oclass;
	static native *defaults;
};

#endif /// @} _NATIVE_H

