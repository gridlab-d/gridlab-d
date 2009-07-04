/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule Schedules
**/

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

#include "timestamp.h"

#define MAXBLOCKS 4
#define MAXVALUES 64

/** The SCHEDULE structure defines POSIX style schedules */
typedef struct s_schedule SCHEDULE;
struct s_schedule {
	char name[64];						/**< the name of the schedule */
	char definition[1024];				/**< the definition string of the schedule */
	char blockname[MAXBLOCKS][64];		/**< the name of each block */
	unsigned char block;				/**< the last block used (4 max) */
	unsigned char index[14][8784*60];	/**< the schedule index (enough room for all 14 annual calendars to 1 minute resolution) */
	unsigned long dtnext[14][8784*60];	/**< the time until the next schedule change (in minutes) */
	double data[MAXBLOCKS*MAXVALUES];	/**< the list of values used in each block */
	double sum[MAXBLOCKS];				/**< the sum of values for each block -- used to normalize */
	double abs[MAXBLOCKS];				/**< the sum of the absolute values for each block -- used to normalize */
	unsigned int count[MAXBLOCKS];		/**< the number of values given in each block */
	TIMESTAMP next_t;					/**< the time of the next schedule event */
	double value;						/**< the current scheduled value */
	double duration;					/**< the duration of the current scheduled value */
	SCHEDULE *next;	/* next schedule in list */
};

SCHEDULE *schedule_create(char *name, char *definition);
int schedule_normalize(SCHEDULE *sch, int use_abs);
int schedule_index(SCHEDULE *sch, TIMESTAMP ts);
double schedule_value(SCHEDULE *sch, int index);
long schedule_dtnext(SCHEDULE *sch, int index);
TIMESTAMP schedule_sync(SCHEDULE *sch, TIMESTAMP t);
TIMESTAMP schedule_syncall(TIMESTAMP t);

#endif
