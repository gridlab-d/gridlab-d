/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule Schedules

	Schedules are defined as a multiline string
	
	@par Schedule syntax
	<code>
	// comments are ignored until and end-of-line
	block1-name { // each block must have a unique name
		minutes hours days months weekdays value // uses the crontab format
		minutes hours days months weekdays value // multiple entries separate by newlines or semicolons
	}
	block2-name { // normalization is done over each block
		minutes hours days months weekdays // omitted values a taken to be 1.0
		}
	block3-name { // block are combined
		// omitted entries are taken to be 0.0
	}
	</code>

	Optionally, a simple schedule can be provided

	<code>
	minutes hours days months weekdays value // uses the crontab format
	minutes hours days months weekdays value // multiple entries separate by newlines or semicolons
	</code>

**/

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "class.h"
#include "timestamp.h"

#define MAXBLOCKS 4
#define MAXVALUES 64
#define GET_BLOCK(I) ((I)>>6)&0x02)
#define GET_VALUE(I) ((I)&0x3f)

typedef uint32 SCHEDULEINDEX;
#define GET_CALENDAR(N) (((N)>>20)&0x0f)
#define GET_MINUTE(N) ((N)&0x0fffff)
#define SET_CALENDAR(N,X) (N)|=(((X)&0x0f)<<20)
#define SET_MINUTE(N,X) (N)|=((X)&0x0fffff)

/** The SCHEDULE structure defines POSIX style schedules */
typedef struct s_schedule SCHEDULE;
struct s_schedule {
	/* the output value must be first for transform to stream */
	double value;						/**< the current scheduled value */
	char name[64];						/**< the name of the schedule */
	char definition[65536];				/**< the definition string of the schedule */
	char blockname[MAXBLOCKS][64];		/**< the name of each block */
	unsigned char block;				/**< the last block used (4 max) */
	unsigned char index[14][366*24*60];	/**< the schedule index (enough room for all 14 annual calendars to 1 minute resolution) */
	unsigned char dtnext[14][366*24*60];/**< the time until the next schedule change (in minutes) */
	double data[MAXBLOCKS*MAXVALUES];	/**< the list of values used in each block */
	unsigned int weight[MAXBLOCKS*MAXVALUES];	/**< the weight (in minutes) associate with each value */
	double sum[MAXBLOCKS];				/**< the sum of values for each block -- used to normalize */
	double abs[MAXBLOCKS];				/**< the sum of the absolute values for each block -- used to normalize */
	unsigned int count[MAXBLOCKS];		/**< the number of values given in each block */
	unsigned int minutes[MAXBLOCKS];	/**< the total number of minutes associate with each block */
	TIMESTAMP next_t;					/**< the time of the next schedule event */
	TIMESTAMP since;
	double duration;					/**< the duration of the current scheduled value (in hours) */
	double fraction;					/**< the fractional weight of the block of the current value (pu time) */
	int flags;							/**< the schedule flags (see SN_*) */
	SCHEDULE *next;	/* next schedule in list */
};

typedef enum {
	XS_UNKNOWN	= 0x00, 
	XS_DOUBLE	= 0x01, 
	XS_COMPLEX	= 0x02, 
	XS_LOADSHAPE= 0x04, 
	XS_ENDUSE	= 0x08, 
	XS_SCHEDULE = 0x10,
	XS_ALL		= 0x1f,
	} XFORMSOURCE;
typedef struct s_schedulexform {
	double *source;
	XFORMSOURCE source_type;
	void *source_addr;
	SCHEDULE *source_schedule;
	double *target;
	struct s_object_list *target_obj;
	struct s_property_map *target_prop;
	double scale;
	double bias;
	struct s_schedulexform *next;
} SCHEDULEXFORM;

#define SN_NORMAL   0x0001	/**< schedule normalization flag - normalize enabled */
#define SN_ABSOLUTE 0x0002	/**< schedule normalization flag - use absolute values */
#define SN_WEIGHTED 0x0004	/**< schedule normalization flag - use weighted values */
#define SN_BOOLEAN 0x8000 /**< schedule is boolean (only one/zero values are expected) */
#define SN_NONZERO 0x4000 /**< schedule is non-zero (no zero values are expected) */
#define SN_POSITIVE 0x2000 /**< schedule is positive (no negative values are expected) */

#ifdef __cplusplus
extern "C" {
#endif

SCHEDULE *schedule_getnext(SCHEDULE *sch);
SCHEDULE *schedule_find_byname(char *name);
SCHEDULE *schedule_create(char *name, char *definition);
SCHEDULE *schedule_new(void);
void schedule_add(SCHEDULE *sch);
int schedule_validate(SCHEDULE *sch, int flags);
int schedule_normalize(SCHEDULE *sch, int flags);
SCHEDULEINDEX schedule_index(SCHEDULE *sch, TIMESTAMP ts);
double schedule_value(SCHEDULE *sch, SCHEDULEINDEX index);
int32 schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index);
TIMESTAMP schedule_sync(SCHEDULE *sch, TIMESTAMP t);
TIMESTAMP schedule_syncall(TIMESTAMP t);
TIMESTAMP scheduletransform_syncall(TIMESTAMP t, XFORMSOURCE restrict);
int schedule_test(void);
void schedule_dump(SCHEDULE *sch, char *file);

int schedule_add_xform(XFORMSOURCE stype, double *source, double *target, double scale, double bias, struct s_object_list *obj, struct s_property_map *prop, SCHEDULE *s);
SCHEDULEXFORM *scheduletransform_getnext(SCHEDULEXFORM *xform);

#ifdef __cplusplus
}
#endif

#endif
