/* realtime.h
 * Copyright (C) 2008 Battelle Memorial Institute
 */

#ifndef _REALTIME_H
#define _REALTIME_H

#include <time.h>

#include "globals.h"

#ifdef __cplusplus
extern "C"
{
#endif

extern time_t realtime_now();
extern time_t realtime_starttime();
extern time_t realtime_runtime();
extern STATUS realtime_schedule_event(time_t, STATUS (*callback)());
extern STATUS realtime_run_schedule();

#ifdef __cplusplus
};
#endif

#endif
