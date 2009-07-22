/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule Schedules

	Schedules are defined as a multiline string
	
	@par Schedule syntax
	<code>
	# comments are ignored until and end-of-line
	block1-name { # each block must have a unique name
		minutes hours days months weekdays value # uses the crontab format
		minutes hours days months weekdays value # multiple entries separate by newlines or semicolons
	}
	block2-name { # normalization is done over each block
	}
	block3-name { # block are combined
	}
	</code>

	Optionally, a simple schedule can be provided

**/

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "timestamp.h"

#define MAXBLOCKS 4
#define MAXVALUES 64

typedef unsigned long SCHEDULEINDEX;
#define GET_CALENDAR(N) (((N)>>20)&0x0f)
#define GET_MINUTE(N) ((N)&0x0fffff)
#define SET_CALENDAR(N,X) (N)|=(((X)&0x0f)<<20)
#define SET_MINUTE(N,X) (N)|=((X)&0x0fffff)

/** The SCHEDULE structure defines POSIX style schedules */
typedef struct s_schedule SCHEDULE;
struct s_schedule {
	char name[64];						/**< the name of the schedule */
	char definition[65536];				/**< the definition string of the schedule */
	char blockname[MAXBLOCKS][64];		/**< the name of each block */
	unsigned char block;				/**< the last block used (4 max) */
	unsigned char index[14][366*24*60];	/**< the schedule index (enough room for all 14 annual calendars to 1 minute resolution) */
	unsigned long dtnext[14][366*24*60];/**< the time until the next schedule change (in minutes) */
	double data[MAXBLOCKS*MAXVALUES];	/**< the list of values used in each block */
	unsigned int weight[MAXBLOCKS*MAXVALUES];	/**< the weight (in minutes) associate with each value */
	double sum[MAXBLOCKS];				/**< the sum of values for each block -- used to normalize */
	double abs[MAXBLOCKS];				/**< the sum of the absolute values for each block -- used to normalize */
	unsigned int count[MAXBLOCKS];		/**< the number of values given in each block */
	unsigned int minutes[MAXBLOCKS];	/**< the total number of minutes associate with each block */
	TIMESTAMP next_t;					/**< the time of the next schedule event */
	double value;						/**< the current scheduled value */
	double duration;					/**< the duration of the current scheduled value */
	int flags;							/**< the schedule flags (see SF_*) */
	SCHEDULE *next;	/* next schedule in list */
};

#define SN_NORMAL   0x0001	/**< schedule normalization flag - normalize enabled */
#define SN_ABSOLUTE 0x0002	/**< schedule normalization flag - use absolute values */
#define SN_WEIGHTED 0x0004	/**< schedule normalization flag - use weighted values */

#ifdef __cplusplus
extern "C" {
#endif
SCHEDULE *schedule_getnext(SCHEDULE *sch);
SCHEDULE *schedule_find_byname(char *name);
SCHEDULE *schedule_create(char *name, char *definition);
int schedule_normalize(SCHEDULE *sch, int flags);
SCHEDULEINDEX schedule_index(SCHEDULE *sch, TIMESTAMP ts);
double schedule_value(SCHEDULE *sch, SCHEDULEINDEX index);
long schedule_dtnext(SCHEDULE *sch, SCHEDULEINDEX index);
TIMESTAMP schedule_sync(SCHEDULE *sch, TIMESTAMP t);
TIMESTAMP schedule_syncall(TIMESTAMP t);
int schedule_test(void);
void schedule_dump(SCHEDULE *sch, char *file);

#ifdef __cplusplus
}
#endif

#endif
