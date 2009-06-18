/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.h
	@addtogroup schedule
**/

#ifndef _SCHEDULE_H
#define _SCHEDULE_H

typedef struct s_schedule {
	char *name;			/**< the name of the schedule */
	char *definition;	/**< the definition string of the schedule */
	double *data;		/**< the schedule data (size is always period/resolution */
} SCHEDULE;

SCHEDULE *schedule_create(char *name, char *definition);
double schedule_read(SCHEDULE *, TIMESTAMP ts);

#endif
