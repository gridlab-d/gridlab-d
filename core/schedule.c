/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.c
	@addtogroup schedule
**/

#include "platform.h"
#include "schedule.h"

/** Create a schedule 
	@return a pointer to the new schedule, NULL is failed
 **/
SCHEDULE *schedule_create(char *name,		/**< the name of the schedule */
						  char *definition)	/**< the definition of the schedule (using crontab format) */
{
	throw_exception("schedule_create(char *name='%s', char *definition='%s') is not implemented)", name, definition);
	return NULL;
}

double schedule_read(SCHEDULE *schedule, TIMESTAMP ts)
{
	throw_exception("schedule_read(SCHEDULE *schedule='{name=%s,...}', TIMESTAMP *ts=%" FMT_INT64 "d") is not implemented)", schedule->name, ts);
	return NaN;
}
