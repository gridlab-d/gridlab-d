/// $Id$
/// @file varmap.h
/// @addtogroup connection
/// @{

#ifndef _VARMAP_H
#define _VARMAP_H

#include "gridlabd.h"
#include <string>
using namespace std;

typedef enum {
	DXD_READ=0x01,  ///< data is incoming
	DXD_WRITE=0x02, ///< data is outgoing
	DXD_ALLOW=0x04, ///< data exchange allowed for protected properties
	DXD_FORBID=0x08,///< data exchange forbidden for unprotected properties
} DATAEXCHANGEDIRECTION;
typedef enum {
	CT_UNKNOWN=1,
	CT_PUBSUB=2, ///< This is a publish subscribe type of communication
	CT_ROUTE=3, ///< This is a point to point type of communication
}COMMUNICATIONTYPE;
typedef struct s_varmap {
	char *local_name; ///< local name
	char *remote_name; ///< remote name
	DATAEXCHANGEDIRECTION dir; ///< direction of exchange
	gld_property *obj; ///< local object and property (obj==NULL for globals)
	struct s_varmap *next; ///< next variable in map
	COMMUNICATIONTYPE ctype; ///< The actual communication type. Used only for communication with FNCS.
	char threshold[1024]; ///< The threshold to exceed to actually trigger sending a message. Used only for communication with FNCS.
	void *last_value; ///< The last value that was sent out to FNCS. Used only for communication with FNCS.
} VARMAP; ///< variable map structure

class varmap {
public:
	VARMAP *map;
public:
	varmap();
	int add(char *spec, COMMUNICATIONTYPE comtype); ///< add a variable mapping using full spec, e.g. "<event>:<local><dir><remote>"
	void resolve(void); ///< process unresolved local names
	void linkcache(class connection_mode*, void *xlate); ///< link cache to variables
	inline VARMAP *getfirst(void) ///< get first variable in map
		{ return map; }; 
	inline VARMAP *getnext(VARMAP*var) /// get next variable in map
		{ return var->next; };
};

#endif /// @} _VARMAP_H
