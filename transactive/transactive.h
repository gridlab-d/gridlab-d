// $Id: transactive.h 5217 2015-07-05 17:40:35Z dchassin $
//

#ifndef _TRANSACTIVE_H
#define _TRANSACTIVE_H

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "gridlabd.h"

// verbose options
#define VO_NONE            0x0000 ///< enable no verbose output
#define VO_SOLVER          0x0001 ///< enable solver verbose output
#define VO_INTERCONNECTION 0x0002 ///< enable interconnection verbose output
#define VO_CONTROLAREA     0x0004 ///< enable controlarea verbose output
#define VO_INTERTIE        0x0008 ///< enable intertie verbose output
#define VO_GENERATOR       0x0010 ///< enable generator verbose output
#define VO_LOAD            0x0020 ///< enable load verbose output
#define VO_ALL             0xffff ///< enable all verbose output

// constraint violations
#define CV_NONE			   0x0000 ///< allow no constraint violations
#define CV_INTERCONNECTION 0x0002 ///< allow interconnection constraint violation
#define CV_CONTROLAREA     0x0004 ///< allow controlarea constraint violation
#define CV_GENERATOR       0x0008 ///< allow generator constraint violation
#define CV_INTERTIE        0x0010 ///< allow intertie constraint violation
#define CV_LOAD            0x0020 ///< allow load constraint violation
#define CV_ALL             0xffff ///< allow all constraint violation

// message types
#define TM_AREA_STATUS			"AS E=%lg R=%lg G=%lg D=%lg L=%lg" // E=inertia[MJ], R=capacity[MW], G=supply[MW], D=demand[MW], L=losses[MW]
#define TM_REGISTER_CONTROLAREA	"RC O=%p" // O=object
#define TM_REGISTER_INTERTIE	"RI O=%p" // O=object
#define TM_ACTUAL_GENERATION    "GA R=%lg P=%lg E=%lg" // R=capacity[MW] P=power[MW] E=energy[MJ]
#define TM_ACTUAL_LOAD			"LA P=%lg E=%lg" // P=power[MW] E=energy[MJ]

// kml mark colors
#define KC_GREEN 0
#define KC_RED 1
#define KC_ORANGE 2
#define KC_BLUE 3
#define KC_PURPLE 4
#define KC_CYAN 5
#define KC_BLACK 6

// transaction message tracing
typedef enum {
	TMT_NONE            = 0x0000, ///< no messages traced
	TMT_INTERCONNECTION = 0x0001, ///< interconnection messages traced
	TMT_CONTROLAREA     = 0x0002, ///< controlarea messages traced
	_TMT_LAST,
	TMT_ALL				= 0xffff, ///< all messages traced
} TRANSACTIVEMESSAGETYPE; ///< message trace enumeration

void trace_message(TRANSACTIVEMESSAGETYPE type, const char *msg);

// object list handling
typedef struct s_objectlist {
	OBJECT *obj;
	struct s_objectlist *next;
} OBJECTLIST;
inline OBJECTLIST *add_object(OBJECTLIST *list, OBJECT *obj)
{
	OBJECTLIST *item = (OBJECTLIST*)malloc(sizeof(OBJECTLIST));
	item->obj = obj;
	item->next = list;
	return item;
}

#include "weather.h"
#include "interconnection.h"
#include "scheduler.h"
#include "controlarea.h"
#include "intertie.h"
#include "generator.h"
#include "load.h"
#include "dispatcher.h"

#ifdef DLMAIN
#define EXTERN
#define INIT(X) = X
#else
#define EXTERN extern
#define INIT(X)
#endif

EXTERN double nominal_frequency INIT(60.0); ///< nominal system frequency (Hz)
EXTERN double maximum_frequency INIT(63.0); ///< maximum allowed system frequency (Hz)
EXTERN double minimum_frequency INIT(57.0); ///< minimum allowed system frequency (Hz)
#define FB_NONE 0 ///< no frequency bounds (no warning or error)
#define FB_SOFT 1 ///< soft frequency bounds (warning only)
#define FB_HARD 2 ///< hard frequency bounds (fatal error)
EXTERN enumeration frequency_bounds INIT(FB_SOFT); ///< frequency bounds (FB_NONE, FB_SOFT, FB_HARD)
EXTERN TRANSACTIVEMESSAGETYPE trace_messages INIT(TMT_NONE); ///< message trace mode (TMT_NONE, TMT_INTERCONNECTION, TMT_CONTROLAREA, TMT_ALL)
EXTERN set verbose_options INIT(VO_NONE); ///< verbose simulation level (see VO_*)
EXTERN set allow_constraint_violation INIT(CV_NONE); ///< constraints control (see CV_*)

EXTERN double schedule_interval INIT(3600); ///< default schedule update interval (midpoint of ramp interval)
EXTERN double dispatch_ramptime INIT(20*60); ///< default schedule ramp interval for dispatchers

EXTERN double droop_control INIT(0.05);
EXTERN double lift_control INIT(0.05);

#endif

