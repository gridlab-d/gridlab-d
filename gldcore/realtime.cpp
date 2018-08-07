/* realtime.c
 * Copyright (C) 2008 Battelle Memorial Institute
 * This module handle realtime events such as regular update to the environment, alarms and processing time limits.
 */

#include <stdlib.h>
#include <errno.h>
#include "realtime.h"

time_t realtime_now(void)
{
	return time(NULL);
}
static time_t starttime = 0;
time_t realtime_starttime(void)
{
	if ( starttime==0 )
		starttime = realtime_now();
	return starttime;
}
time_t realtime_runtime(void)
{
	return realtime_now() - starttime;
}

/****************************************************************/
typedef struct s_eventlist {
	time_t at;
	STATUS (*call)(void);
	struct s_eventlist *next;
} EVENT;
static EVENT *eventlist = NULL;

STATUS realtime_schedule_event(time_t at, STATUS (*callback)(void))
{
	EVENT *event = malloc(sizeof(EVENT));
	if (event==NULL)
	{
		errno=ENOMEM;
		return FAILED;
	}
	event->at = at;
	event->call = callback;
	event->next = eventlist;
	eventlist = event;
	return SUCCESS;
}

STATUS realtime_run_schedule(void)
{
	time_t now = realtime_now();
	EVENT *event, *last=NULL;
	for (event=eventlist; event!=NULL; event=event->next)
	{
		if (event->at<=now)
		{
			STATUS (*call)(void) = event->call;

			/* delete from list */
			if (last==NULL) /* event is first in list */
				eventlist = event->next;
			else
				last->next = event->next;
			free(event);
			event = NULL;

			/* callback */
			if ((*call)()==FAILED)
				return FAILED;

			/* retreat to previous event */
			event = last;
			if (event==NULL)
				break;
		}
	}
	return SUCCESS;
}
