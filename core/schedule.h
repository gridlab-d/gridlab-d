/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule
**/

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

typedef struct s_schedule {
	char name[64];			/**< the name of the schedule */
	char definition[1024];	/**< the definition string of the schedule */
	unsigned char index[14][8784*60];	/**< the schedule index (enough room for all 14 annual calendars to 1 minute resolution) */
	double data[256]; /**< the schedule values **/
	SCHEDULE *next;		/* next schedule in list */
} SCHEDULE;

SCHEDULE *schedule_create(char *name, char *definition);
void schedule_normalize(SCHEDULE *sch);
double schedule_read(SCHEDULE *sch, TIMESTAMP ts);

#endif
