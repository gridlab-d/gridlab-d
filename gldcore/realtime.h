/* realtime.h
 * Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef _REALTIME_H
#define _REALTIME_H

#include <time.h>
#include "globals.h"

time_t realtime_now(void);
time_t realtime_starttime(void);
time_t realtime_runtime(void);
STATUS realtime_schedule_event(time_t, STATUS (*callback)(void));
STATUS realtime_run_schedule(void);

#endif
