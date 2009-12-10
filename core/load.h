/** $Id: load.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file load.h
	@addtogroup load_glm
	@ingroup core
 @{
 **/

#ifndef _LOAD_H
#define _LOAD_H

#include "globals.h"
#include "module.h"

#include "load_xml.h"

#define UR_NONE  0x00 /* no flags */
#define UR_RANKS 0x01 /* reference has ranking impact */
#define UR_TRANSFORM 0x02 /* reference is via a transform */

#ifdef __cplusplus
extern "C" {
#endif
STATUS loadall(char *filename);
#ifdef __cplusplus
}
#endif

typedef struct s_unresolved {
	OBJECT *by;
	PROPERTYTYPE ptype;
	void *ref;
	int flags;
	CLASS *oclass;
	char256 id;
	char *file;
	unsigned int line;
	struct s_unresolved *next;
} UNRESOLVED;

typedef struct s_unresolved_func {
	char1024 funcstr;
	OBJECT *obj;
	double *targ;
	unsigned int line;
	struct s_unresolved_func *next;
} UNR_FUNC;

/* I need these in gld_loadHndl. -MH */
STATUS load_set_index(OBJECT *obj, OBJECTNUM id);
OBJECT *load_get_index(OBJECTNUM id);
double load_latitude(char *buffer);
double load_longitude(char *buffer);
int time_value(char *, TIMESTAMP *t);
int time_value_datetime(char *c, TIMESTAMP *t);
int set_flags(OBJECT *obj, char1024 propval);
UNRESOLVED *add_unresolved(OBJECT *by, PROPERTYTYPE ptype, void *ref, CLASS *oclass, char *id, char *file, unsigned int line, int flags);
int load_resolve_all();

#endif

/**@}*/
