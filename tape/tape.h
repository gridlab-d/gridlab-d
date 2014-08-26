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

/* recorder-specific enums */
typedef enum {HU_DEFAULT, HU_ALL, HU_NONE} HEADERUNITS;
typedef enum {LU_DEFAULT, LU_ALL, LU_NONE} LINEUNITS;

typedef struct s_tape_operations {
	int (*open)(void *my, char *fname, char *flags);
	char *(*read)(void *my,char *buffer,unsigned int size);
	int (*write)(void *my, char *timestamp, char *value);
	int (*rewind)(void *my);
	void (*close)(void *my);
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
	char *name;
	VARIABLETYPE type;
	void *addr;
	double min, max;
} VARMAP;

typedef struct s_recobjmap {
	OBJECT *obj;
	PROPERTY prop; // must be an instance
	struct s_recobjmap *next;
} RECORDER_MAP;

/** @}
  @addtogroup player
	@{ 
 **/
struct player {
	/* public */
	char1024 file; /**< the name of the player source */
	char8 filetype; /**< the type of the player source */
	char256 mode;
	char256 property; /**< the target property */
	int32 loop; /**< the number of time to replay the tape */
	/* private */
	FILETYPE type;
	union {
		FILE *fp;
		MEMORY *memory;
		void *tsp;
		/** add handles for other type of sources as needed */
	};
	TAPESTATUS status;
	int32 loopnum;
	struct {
		TIMESTAMP ts;
		int64 ns;
		char256 value;
	} next;
	struct {
		TIMESTAMP ts;
		TIMESTAMP ns;
		char256 value;
	} delta_track;	/* Added for deltamode fixes */
	PROPERTY *target;
	TAPEOPS *ops;
	char lasterr[1024];
}; /**< a player item */
/** @}
	@addtogroup shaper
	@{
 **/
typedef struct s_shapertarget {
	double *addr; /**< the address of the target property */
	TIMESTAMP ts; /**< the current timestamp to shape for */
	double value; /**< the current value to shape */
} SHAPERTARGET; /**< the shaper target structure */

struct shaper {
	/* public */
	char1024 file; /**< the name of the shaper source */
	char8 filetype; /**< the type of the shaper source */
	char32 mode;
	char256 property; /**< the target property */
	char256 group; /**< the object grouping to use in choosing the shape target */
	double magnitude;	/**< the magnitude of a queue event (only used when \p events > 0) */
	double events;		/* the number of queue events per interval (\p events = 0 => direct shape)*/
	/* private */
	TAPEOPS *ops;
	FILETYPE type;
	union {
		FILE *fp;
		MEMORY *memory;
		void *tsp;
		/** add handles for other type of sources as needed */
	};
	TAPESTATUS status;
	char lasterr[1024];
	int16 interval;	/* the interval over which events is counted (usually 24) */
	int16 step;		/* the duration of a single step in the shape integral (usually 3600s) */
	double scale;	/* the scaling of the shape over the interval */
	int32 loopnum;
	unsigned char shape[12][31][7][24];
#define SHAPER_QUEUE 0x0001
	unsigned int n_targets;
	SHAPERTARGET *targets;
};
/** @}
	@addtogroup recorder
	@{
 **/
struct recorder {
	/* public */
	char1024 file;
	char8 filetype;
	char32 mode;
	char1024 multifile;
	char1024 multitempfile;
	FILE *multifp, *inputfp;
	int16 multirun_ct;
	char1024 multirun_header;
	int16 format; /* 0=YYYY-MM-DD HH:MM:SS; 1=timestamp */
	double dInterval;
	TIMESTAMP interval;
	int32 limit;
	char1024 property;
	char1024 out_property;
	PLOTFILE output; /* {EPS|GIF|JPG|PDF|PNG|SVG} More can be added */
	char1024 plotcommands;
	char32 xdata;
	char32 columns;
	char32 trigger;
	/* private */
	RECORDER_MAP *rmap;
	TAPEOPS *ops;
	FILETYPE type;
	HEADERUNITS header_units;
	LINEUNITS line_units;
	union {
		FILE *fp;
		MEMORY *memory;
		void *tsp;
		/** add handles for other type of sources as needed */
	};
	TAPESTATUS status;
	char8 delim;
	struct {
		TIMESTAMP ts;
		int64 ns;
		char1024 value;
	} last;
	int32 samples;
	PROPERTY *target;
};
/** @}
	@addtogroup collector
	@{
 **/
struct collector {
	/* public */
	char1024 file;
	char8 filetype;
	char32 mode;
	int16 format; /* 0=YYYY-MM-DD HH:MM:SS; 1=timestamp */
	double dInterval;
	TIMESTAMP interval;
	int32 limit;
	char1024 property;
	PLOTFILE output; /* {EPS|GIF|JPG|PDF|PNG|SVG} More can be added */
	char1024 plotcommands;
	char32 xdata;
	char32 columns;
	char32 trigger;
	char256 group;
	/* private */
	TAPEOPS *ops;
	FILETYPE type;
	union {
		FILE *fp;
		MEMORY *memory;
		void *tsp;
		/** add handles for other type of sources as needed */
	};
	TAPESTATUS status;
	char8 delim;
	struct {
		TIMESTAMP ts;
		char1024 value;
	} last;
	int32 samples;
	AGGREGATION *aggr;
};

void enable_deltamode(TIMESTAMP t1); /* indicate when deltamode is needed */

void set_csv_options(void);

#endif
