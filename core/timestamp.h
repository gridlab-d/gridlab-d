/** $Id: timestamp.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file timestamp.h
	@addtogroup timestamp
	@ingroup core
 @{
 **/

#ifndef _TIMESTAMP_H
#define _TIMESTAMP_H

#include "platform.h"
#include <time.h>

#define NORMALRES /* use this to control default timestamp resolution */

#ifdef VERYHIGHRES
#define TS_SCALE (-9) /* system timescale is 1 nanosecond (1e-9 s)*/
#define TS_SECOND ((int64)1000000000) /* duration of one second */
#define TS_RESOLUTION (1e-9) /* must match scale */
#elif defined HIGHRES
#define TS_SCALE (-6) /* system timescale is 1 nanosecond (1e-9 s)*/
#define TS_SECOND ((int64)1000000) /* duration of one second */
#define TS_RESOLUTION (1e-6) /* must match scale */
#elif defined MEDIUMRES
#define TS_SCALE (-3) /* system timescale is 1 nanosecond (1e-9 s)*/
#define TS_SECOND ((int64)1000) /* duration of one second */
#define TS_RESOLUTION (1e-3) /* must match scale */
#else /* NORMALRES */
#define TS_SCALE (0) /* system timescale is 1 second (1e-0 s)*/
#define TS_SECOND ((int64)1) /* duration of one second */
#define TS_RESOLUTION (1) /* must be 10^TS_SCALE */
#endif

typedef int64 TIMESTAMP;
typedef unsigned int64 DELTAT; /**< stores cumulative delta time values in ns */
typedef unsigned long DT; /**< stores incremental delta time values in ns */

#define TS_ZERO ((int64)0)
#define TS_MAX (32482080000LL) /* roughly 3000 CE, any date beyond this should be interpreted as TS_NEVER */
#define TS_INVALID ((int64)-1)
#define TS_NEVER ((int64)(((unsigned int64)-1)>>1))
#define MINYEAR 1970
#define MAXYEAR 2969
#define ISLEAPYEAR(Y) ((Y)%4==0 && ((Y)%100!=0 || (Y)%400==0))

#define DT_INFINITY (0xfffffffe)
#define DT_INVALID  (0xffffffff)
#define DT_SECOND 1000000000

typedef struct s_datetime {
	unsigned short year; /**< year (1970 to 2970 is allowed) */
	unsigned short month; /**< month (1-12) */
	unsigned short day; /**< day (1 to 28/29/30/31) */
	unsigned short hour; /**< hour (0-23) */
	unsigned short minute; /**< minute (0-59) */
	unsigned short second; /**< second (0-59) */
	unsigned int nanosecond; /**< usecond (0-999999999) */
	unsigned short is_dst; /**< 0=std, 1=dst */
	char tz[5]; /**< ptr to tzspec timezone id */
	unsigned short weekday; /**< 0=Sunday */
	unsigned short yearday; /**< 0=Jan 1 */
	TIMESTAMP timestamp; /**< GMT timestamp */
	int tzoffset; /**< time zone offset in seconds (-43200 - 43200) */
} DATETIME; /**< the s_datetime structure */

#ifdef __cplusplus
extern "C" {
#endif

/* basic date and time functions */
char *timestamp_current_timezone(void);
TIMESTAMP mkdatetime(DATETIME *dt);
int strdatetime(DATETIME *t, char *buffer, int size);
int convert_from_timestamp(TIMESTAMP ts, char *buffer, int size);
int convert_from_timestamp_delta(TIMESTAMP ts, DELTAT delta_t, char *buffer, int size);
double timestamp_to_days(TIMESTAMP t);
double timestamp_to_hours(TIMESTAMP t);
double timestamp_to_minutes(TIMESTAMP t);
double timestamp_to_seconds(TIMESTAMP t);
TIMESTAMP convert_to_timestamp(const char *value);
TIMESTAMP convert_to_timestamp_delta(const char *value, unsigned int *nanoseconds, double *dbl_time_value);
int local_datetime(TIMESTAMP ts, DATETIME *dt);

int timestamp_test(void);

char *timestamp_set_tz(char *tzname);
TIMESTAMP timestamp_from_local(time_t t);
time_t timestamp_to_local(TIMESTAMP t);

int local_tzoffset(TIMESTAMP t);

double timestamp_get_part(void *x, char *name);
TIMESTAMP earliest_timestamp(TIMESTAMP t, ...);
TIMESTAMP absolute_timestamp(TIMESTAMP t);

#ifdef __cplusplus
}
#endif

#endif

/**@}*/
