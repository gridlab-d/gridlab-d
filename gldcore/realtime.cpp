/* realtime.c
 * Copyright (C) 2008 Battelle Memorial Institute
 * This module handle realtime events such as regular update to the environment, alarms and processing time limits.
 */

#include <cerrno>
#include <cstdlib>

#include "realtime.h"

extern time_t realtime_now()
{
	return time(nullptr);
}
static time_t starttime = 0;
extern time_t realtime_starttime()
{
	if ( starttime==0 )
		starttime = realtime_now();
	return starttime;
}
extern time_t realtime_runtime()
{
	return realtime_now() - starttime;
}

/****************************************************************/
typedef struct s_eventlist {
	time_t at;
	STATUS (*call)();
	struct s_eventlist *next;
} EVENT;
static EVENT *eventlist = nullptr;

extern STATUS realtime_schedule_event(time_t at, STATUS (*callback)())
{
	EVENT *event = static_cast<EVENT *>(malloc(sizeof(EVENT)));
	if (event==nullptr)
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

extern STATUS realtime_run_schedule()
{
	time_t now = realtime_now();
	EVENT *event, *last=nullptr;
	for (event=eventlist; event!=nullptr; event=event->next)
	{
		if (event->at<=now)
		{
			STATUS (*call)(void) = event->call;

			/* delete from list */
			if (last==nullptr) /* event is first in list */
				eventlist = event->next;
			else
				last->next = event->next;
			free(event);
			event = nullptr;

			/* callback */
			if ((*call)()==FAILED)
				return FAILED;

			/* retreat to previous event */
			event = last;
			if (event==nullptr)
				break;
		}
	}
	return SUCCESS;
}
