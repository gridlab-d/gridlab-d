#ifndef _generators_H
#define _generators_H

#include <stdarg.h>
#include <vector>

#include "gridlabd.h"

#ifdef _GENERATORS_GLOBALS
#define GLOBAL
#define INIT(A) = (A)
#else
#undef GLOBAL
#define GLOBAL extern
#undef INIT
#define INIT(A)
#endif

//Phase definitions pilfered from powerflow_object.h for readability
#define PHASE_A		0x0001		/**< A phase connection */
#define PHASE_B		0x0002		/**< B phase connection */
#define PHASE_C		0x0004		/**< C phase connection */
#define PHASE_ABC	0x0007		/**< three phases connection */
#define PHASE_N		0x0008		/**< N phase connected */
#define PHASE_ABCN	0x000f		/**< three phases neutral connection */
#define GROUND		0x0080		/**< ground line connection */
/* delta configuration */
#define PHASE_D		0x0100		/**< delta connection (requires ABCN) */
/* split phase configurations */
#define PHASE_S1	0x0010		/**< split line 1 connection */
#define PHASE_S2	0x0020		/**< split line 2 connection */
#define PHASE_SN	0x0040		/**< split line neutral connection */
#define PHASE_S		0x0070		/**< Split phase connection */

typedef struct s_gen_delta_obj {
	OBJECT *obj;
	FUNCTIONADDR preudpate_fxn;
    FUNCTIONADDR interupdate_fxn;
    FUNCTIONADDR post_delta_fxn;
} GEN_DELTA_OBJ;

GLOBAL bool enable_subsecond_models INIT(false); /* normally not operating in delta mode */
GLOBAL bool all_generator_delta INIT(false);			/* Flag to make all generator objects participate in deltamode (that are capable) -- otherwise is individually flagged per object */
GLOBAL unsigned long deltamode_timestep INIT(10000000); /* 10 ms timestep */
GLOBAL double deltamode_timestep_publish INIT(10000000.0); /* 10 ms timestep */
GLOBAL std::vector<GEN_DELTA_OBJ> delta_object;    /* Vector of generator objects and their various function calls */
GLOBAL TIMESTAMP deltamode_starttime INIT(TS_NEVER);	/* Tracking variable for next desired instance of deltamode */
GLOBAL TIMESTAMP deltamode_endtime INIT(TS_NEVER);		/* Tracking variable to see when deltamode ended - so differential calculations don't get messed up */
GLOBAL double deltamode_endtime_dbl INIT(TS_NEVER_DBL);		/* Tracking variable to see when deltamode ended - double valued for explicit movement calculations */
GLOBAL TIMESTAMP deltamode_supersec_endtime INIT(TS_NEVER);	/* Tracking variable to indicate the "floored" time of detamode_endtime */
GLOBAL double deltatimestep_running INIT(-1.0);			/** Value of the current deltamode simulation - used primarily to tell if we're in deltamode or not for VSI */

GLOBAL double default_line_voltage INIT(120.0);			//Value for the default nominal_voltage
GLOBAL double default_temperature_value INIT(59.0);		//Value for default temperature, used for battery model			

void schedule_deltamode_start(TIMESTAMP tstart);	/* Anticipated time for a deltamode start, even if it is now */
STATUS add_gen_delta_obj(OBJECT *obj, bool prioritize); /* Function to add deltamode objects to vector for later delta-function execution */

#define UNKNOWN 0

/* optional exports */
#ifdef OPTIONAL

/* TODO: define this function to enable checks routine */
EXPORT int check(void);

/* TODO: define this function to allow direct import of models */
EXPORT int import_file(char *filename);

/* TODO: define this function to allow direct export of models */
EXPORT int export_file(char *filename);

/* TODO: define this function to allow export of KML data for a single object */
EXPORT int kmldump(FILE *fp, OBJECT *obj);
#endif

#endif
