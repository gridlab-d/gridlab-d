/** $Id$
 *
 *  Created on: Feb 9, 2015
 *      Author: Andy Fisher
 */

#ifndef CONNECTION_FNCS_MSG_H_
#define CONNECTION_FNCS_MSG_H_

#include "gridlabd.h"
#include "varmap.h"
#include "connection.h"
#if HAVE_FNCS
#include <fncs.hpp>
#endif
#include<sstream>
#include<vector>
#include <string>
#include <fstream>
using namespace std;

class fncs_msg;

///< Function relays
typedef struct s_functionsrelay {
	char localclass[64]; ///< local class info
	char localcall[64]; ///< local function call address (NULL for outgoing)
	char remoteclass[64]; ///< remote class name
	char remotename[64]; ///< remote function name
	fncs_msg *route; ///< routing of relay
	TRANSLATOR *xlate; ///< output translation call
	struct s_functionsrelay *next;
	DATAEXCHANGEDIRECTION drtn;
	COMMUNICATIONTYPE ctype; ///<identifies which fncs communication function to call. Only used for communicating with fncs.
} FUNCTIONSRELAY;
extern "C" FUNCTIONADDR add_fncs_function(fncs_msg *route, const char *fclass,const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction, COMMUNICATIONTYPE ctype );
extern "C" FUNCTIONSRELAY *find_fncs_function(const char *rclass, const char *rname);
extern "C" size_t fncs_from_hex(void *buf, size_t len, const char *hex, size_t hexlen);

typedef enum {
	FT_VOID,
	FT_LIST,
	FT_REAL,
	FT_INTEGER,
	FT_STRING,
} FNCSTYPE;
typedef struct _fncslist {
	FNCSTYPE type;
	char tag[32];
	union {
		double real;
		int64 integer;
		struct _fncslist *list;
	};
	char st[1024];
	struct _fncslist *parent; // parent list (NULL for head)
	struct _fncslist *next;
} FNCSLIST;

class fncs_msg : public gld_object {
public:
	GL_ATOMIC(double,version);
	string *port;
	string *header_version;
	string *hostname;
	char1024 configFile;
	// TODO add published properties here

private:
	vector<string> *inFunctionTopics;
	varmap *vmap[14];
	TIMESTAMP last_approved_fncs_time;
	TIMESTAMP initial_sim_time;
	double last_delta_fncs_time;
	bool exitDeltamode;
	// TODO add other properties here as needed.

public:
	// required implementations
	fncs_msg(MODULE*);
	int create(void);
	int init(OBJECT* parent);
	int precommit(TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t0,TIMESTAMP t1);
	int prenotify(PROPERTY* p,char* v);
	int postnotify(PROPERTY* p,char* v);
	int finalize(void);
	TIMESTAMP plc(TIMESTAMP t1);
	int route(char *value);
	int option(char *value);
	int publish(char *value);
	int subscribe(char *value);
	int configure(char *value);
	int parse_fncs_function(char *value, COMMUNICATIONTYPE comstype);
	void incoming_fncs_function(void);
	int publishVariables(varmap *wmap);
	int subscribeVariables(varmap *rmap);
	char simulationName[1024];
	void term(TIMESTAMP t1);
	int fncs_link(char *value, COMMUNICATIONTYPE comtype);
	TIMESTAMP clk_update(TIMESTAMP t1);
	int get_varmapindex(const char *);
	SIMULATIONMODE deltaInterUpdate(unsigned int delta_iteration_counter, TIMESTAMP t0, unsigned int64 dt);
	SIMULATIONMODE deltaClockUpdate(double t1, unsigned long timestep, SIMULATIONMODE sysmode);
	// TODO add other event handlers here

public:
	static FNCSLIST *parse(char *buffer);
	static FNCSLIST *find(FNCSLIST *list, const char *tag);
	static char *get(FNCSLIST *list, const char *tag);
	static void destroy(FNCSLIST *list);

public:
	// special variables for GridLAB-D classes
	static CLASS *oclass;
	static fncs_msg *defaults;
};

#endif /* CONNECTION_FNCS_H_ */
