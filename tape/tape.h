/** $Id: tape.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape.h
 **/

#ifndef _TAPE_H
#define _TAPE_H

#include "gridlabd.h"
#include "object.h"
#include "aggregate.h"
#include "memory.h"

/* tape global controls */
static char timestamp_format[32]="%Y-%m-%d %H:%M:%S";
typedef enum {VT_INTEGER, VT_DOUBLE, VT_STRING} VARIABLETYPE;
typedef enum {TS_INIT, TS_OPEN, TS_DONE, TS_ERROR} TAPESTATUS;
typedef enum {FT_FILE, FT_ODBC, FT_MEMORY} FILETYPE;
typedef enum {SCREEN, EPS, GIF, JPG, PDF, PNG, SVG} PLOTFILE;
typedef enum e_complex_part {NONE = 0, REAL, IMAG, MAG, ANG, ANG_RAD} CPLPT;
typedef enum {UNKNOWN=0, PLAYER=1, RECORDER=2, GROUPRECORDER=3} DELTATAPEOBJ;

typedef struct s_tape_operations {
	int (*open)(void *my, char *fname, char *flags);
	char *(*read)(void *my,char *buffer,unsigned int size);
	int (*write)(void *my, char *timestamp, char *value);
	int (*rewind)(void *my);
	void (*close)(void *my);
	void (*flush)(void *my);
} TAPEOPS;

typedef struct s_tape_funcs {
	char256 mode;
	void *hLib;
	TAPEOPS *player;
	TAPEOPS *shaper;
	TAPEOPS *recorder;
	TAPEOPS *collector;
	TAPEOPS *histogram;
	struct s_tape_funcs *next;
} TAPEFUNCS;

CDECL TAPEFUNCS *get_ftable(char *mode);

typedef struct {
	const char *name;
	VARIABLETYPE type;
	void *addr;
	double min, max;
} VARMAP;

typedef struct s_recobjmap {
	OBJECT *obj;
	PROPERTY prop; // must be an instance
	struct s_recobjmap *next;
} RECORDER_MAP;

typedef struct s_deltaobj {
	OBJECT *obj;
	DELTATAPEOBJ obj_type;
	struct s_deltaobj *next;
} DELTAOBJ_LIST;

extern void enable_deltamode(TIMESTAMP t); /* indicate when deltamode is needed */
EXPORT int delta_add_tape_device(OBJECT *obj, DELTATAPEOBJ tape_type);

void set_csv_options(void);

// TODO: Misc prototypes from across the module. Move all of these into appropriate header files for each component

#endif
