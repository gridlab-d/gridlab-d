/** $Id: timestamp.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file timestamp.c
	@addtogroup timestamp Time management
	@ingroup core

	Time function handle time calculations and daylight saving time (DST).

	DST rules are recorded in the file \p tzinfo.txt. The file loaded
	is the first found according the following search sequence:
	-	The current directory
	-	The same directory as the executable \p gridlabd.exe
	-	The path in the \p GLPATH environment variable

	DST rules are recorded in the Posix-compliant format for the \p TZ
	environment variable:
	@code
		STZ[hh[:mm][DTZ][,M#[#].#.#/hh:mm,M#[#].#.#/hh:mm]]
	@endcode


 @{
 **/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include "platform.h"
#include "timestamp.h"
#include "exception.h"
#include "find.h"
#include "output.h"
#include "globals.h"
#include "lock.h"

#ifndef WIN32
	#define _tzname tzname
	#define _timezone timezone
	#define _daylight daylight
#endif

#define TZFILE "tzinfo.txt"

#define DAY (86400*TS_SECOND) /**< the number of ticks in one day */
#define HOUR (3600*TS_SECOND) /**< the number of ticks in one hour */
#define MINUTE (60*TS_SECOND) /**< the number of ticks in one minute */
#define SECOND (TS_SECOND)/**< the number of ticks in one second */
#define MICROSECOND (TS_SECOND/1000000) /**< the number of ticks in one microsecond */
#define NANOSECOND (TS_SECOND/1000000000) /**< the number of ticks in one nanosecond */

typedef struct{
	int month, nth, day, hour, minute;
} SPEC; /**< the specification of a DST event */

static daysinmonth[] = {31,28,31,30,31,30,31,31,30,31,30,31};
static char *dow[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

#define YEAR0 (1970) /* basis year is 1970 */
#define YEAR0_ISLY (0) /* set to 1 if YEAR0 is a leap year, 1970 is not */
#define DOW0 (4) /* 1/1/1970 is a Thursday (day 4) */

static int tzvalid=0;
static TIMESTAMP tszero[1000] = {-1}; /* zero timestamp offset for each year */
static TIMESTAMP dststart[1000], dstend[1000];
static TIMESTAMP tzoffset;
static char current_tzname[64], tzstd[32], tzdst[32];

#define LOCALTIME(T) ((T)-tzoffset+(isdst((T))?3600:0))
#define GMTIME(T) ((T)+tzoffset-(isdst((T)+tzoffset)?3600:0))

/** Read the current timezone specification
	@return a pointer to the first character in the timezone spec string
 **/
char *timestamp_current_timezone(void){
	return current_tzname;
}

/** Determine the year of a GMT timestamp
	Apply remainder if given
 **/
int timestamp_year(TIMESTAMP ts, TIMESTAMP *remainder)
{
#ifdef USE_TS_CACHE
	static int year = 0; /* reuse last result */
#else
	unsigned int year = (unsigned int)(ts/86400/365.24); /* estimate the year */
#endif
	int tsyear = 0;

	if (tszero[0] == -1){	/* need to initialize tszero array */
		TIMESTAMP ts = 0;
		int year0 = YEAR0;
		int n = (365 + YEAR0_ISLY) * DAY; /* n ticks in year */
		while (ts < TS_MAX && year0 < 2969){
			tszero[year0-YEAR0] = ts;
			ts += n; /* add n ticks from ts */
			year0++; /* add to year */
			n = (ISLEAPYEAR(year0) ? 366 : 365) * DAY; /* n ticks is next year */
		}
	}

	while(year > 0 && ts <= tszero[year]){
		year--;
	}

	while(year < MAXYEAR-YEAR0-1 && ts >= tszero[year+1]){
		year++;
	}

	if(remainder){
		*remainder = ts - tszero[year];
	}

	if((year + YEAR0) < 0){
		output_error("timestamp_year: the year has rolled over or become negative!");
		/*	TROUBLESHOOT
			This is indicative of something strange occuring within timestamp_year().  This is a "kicked up" error message that was
			insufficiently documented from earlier code.
		*/
	}

	return year + YEAR0;
}

/** Determine whether a GMT timestamp is under DST rules
 **/
int isdst(TIMESTAMP t)
{
	int DSTstart_year, DSTend_year;
	int year = timestamp_year(t + tzoffset, NULL) - YEAR0;

	//Preliminary check to make sure something exists
	if (dststart[year]>=0)	//If it's -1, no sense going forth
	{
		DSTstart_year = timestamp_year(dststart[year],NULL);
		DSTend_year = timestamp_year(dstend[year],NULL);

		//Southern hemisphere DST-oriented check
		if (DSTstart_year != DSTend_year)
		{
			//See if we're in the "late-year" DST region
			if (dststart[year] <= t)
			{
				return (t < dstend[year]);	//We are, do a normal check
			}
			else	//In "early-year" DST region - see if we can do some different checks 
			{
				//Make sure we won't underrun the array
				if (year>0)
				{
					//Make sure it is valid (maybe it is before DST is implemented)
					if ((dststart[year-1] > 0) || (dstend[year-1] > 0))
					{
						if (dstend[year-1] < t)	//See if we're above "last year's" ending date
						{
							return 0;	//We're greater than last year's, but less than this year's - clearly not in DST
						}
						else //We're still in last year's region, check us against that
						{
							return ((dststart[year-1] <= t) && (t < dstend[year-1]));
						}
					}
					else
					{
						return 0;	//Invalid DST (pre-DST?), so obviously not DST
					}
				}
				else	//First year of array, probably not wise to go -1 on it, just go like normal
				{
					return (t < dstend[year]);	//See if we're in a normal region
				}
			}
		}
		else	//Northern hemisphere/"sequenced" DST - see if we're in the region
		{
			return ((dststart[year] <= t) && (t < dstend[year]));
		}
	}
	else	//Not even a valid entry, so just return no DST
	{
		return 0;
	}
}

/** Calculate the current TZ offset in seconds
 **/
int local_tzoffset(TIMESTAMP t)
{
#ifdef USE_TS_CACHE
	static old_t = 0;
	static old_tzoffset = 0;
	if (old_t==0 || old_t!=t)
	{
		old_tzoffset = tzoffset + isdst(t)?3600:0;
		old_t = t;
	}
	return old_tzoffset;
#else
	return tzoffset + (isdst(t)?3600:0);
#endif
}

/** Converts a GMT timestamp to local datetime struct
	Adjusts to TZ if possible
 **/
int local_datetime(TIMESTAMP ts, DATETIME *dt)
{

	int64 n;
	TIMESTAMP rem = 0;
	TIMESTAMP local;
	int tsyear;

#ifdef USE_TS_CACHE
	/* allow caching */
	static TIMESTAMP old_ts =0;
	static DATETIME old_dt;
#endif

	if( ts == TS_NEVER || ts==TS_ZERO )
		return 0;

	if( dt==NULL || ts<TS_ZERO || ts>TS_MAX ) /* no buffer or timestamp out of range */
	{
		output_error("local_datetime(ts=%lli,...): invalid local_datetime request",ts);
		return 0;
	}
#ifdef USE_TS_CACHE
	/* check cache */
	if (old_ts == ts && old_ts!=0)
		memcpy(dt,&old_dt,sizeof(DATETIME));
	else
		old_ts = 0;
#endif

	local = LOCALTIME(ts);
	tsyear = timestamp_year(local, &rem);

	if (rem < 0)
	{
		// DPC: note that as of 3.0, the clock is initialized by default, so this error can only
		//      occur when an invalid timestamp is being converted to local time.  It should no
		//      longer occur as a result of a missing clock directive.
		//THROW("local_datetime(ts=%lli, ...): invalid timestamp cannot be converted to local time", ts);
		/*	TROUBLESHOOT
			This is the result of an internal core or module coding error which resulted in an
			invalid UTC clock time being converted to local time.
		*/
		output_error("local_datetime(ts=%lli,...): invalid local_datetime request",ts);
		return 0;
	}

	if(ts < TS_ZERO && ts > TS_MAX){ /* timestamp out of range */
		return 0;
	}
	
	if(ts == TS_NEVER){
		return 0;
	}

	/* ts is valid */
	dt->timestamp = ts;

	/* DST? */
	dt->is_dst = (tzvalid && isdst(ts));

	/* compute year */
	dt->year = tsyear;

	/* yearday and weekday */
	dt->yearday = (unsigned short)(rem / DAY);
	dt->weekday = (unsigned short)((local / DAY + DOW0 + 7) % 7);

	/* compute month */
	dt->month = 0;
	n = daysinmonth[0] * DAY;
	while(rem >= n){
		rem -= n; /* subtract n ticks from ts */
		dt->month++; /* add to month */
		if(dt->month == 12){
			dt->month = 0;
			++dt->year;
		}
		n = (daysinmonth[dt->month] + ((dt->month == 1 && ISLEAPYEAR(dt->year)) ? 1:0)) * 86400 * TS_SECOND;
		if(n < 86400 * 28){ /**/
			output_fatal("Breaking an infinite loop in local_datetime! (ts = %"FMT_INT64"ds", ts);
			/*	TROUBLESHOOT
				An internal protection against infinite loops in the time calculation
				module has encountered a critical problem.  This is often caused by
				an incorrectly initialized timezone system, a missing timezone specification before
				a timestamp was used, or a missing timezone localization in your system.
				Correct the timezone problem and try again.
			 */
			return 0;
		}
	}
	dt->month++; /* Jan=1 */

	/* compute day */
	dt->day = (unsigned short)(rem / DAY + 1);
	rem %= DAY;

	/* compute hour */
	dt->hour = (unsigned short)(rem / HOUR);
	rem %= HOUR;

	/* compute minute */
	dt->minute = (unsigned short)(rem / MINUTE);
	rem %= MINUTE;

	/* compute second */
	dt->second = (unsigned short)rem / TS_SECOND;
	rem %= SECOND;

	/* compute nanosecond */
	dt->nanosecond = (unsigned int)(rem * 1e9);

	/* determine timezone */
	strncpy(dt->tz, tzvalid ? (dt->is_dst ? tzdst : tzstd) : "GMT", sizeof(dt->tz));

	/* timezone offset in seconds */
	dt->tzoffset = tzoffset - (isdst(dt->timestamp)?3600:0);

#ifdef USE_TS_CACHE
	/* cache result */
	old_ts = ts;
	memcpy(&old_dt,dt,sizeof(old_dt));
#endif
	return 1;
}

/** Convert a datetime struct into a GMT timestamp
 **/
TIMESTAMP mkdatetime(DATETIME *dt)
{
	TIMESTAMP ts;
	int n;

	if(dt == NULL){
		return TS_INVALID;
	}

	/* start with year */
	timestamp_year(0,NULL); /* initializes tszero */
	if(dt->year < YEAR0 || dt->year >= YEAR0 + sizeof(tszero) / sizeof(tszero[0]) ){
		return TS_INVALID;
	}
	ts = tszero[dt->year-YEAR0];

	if(dt->month > 12 || dt->month < 1)
	{
		output_fatal("Invalid month provided in datetime");
		return TS_INVALID;
	}
	else if(dt->day > daysinmonth[dt->month-1] || dt->day < 1)
	{
		if(ISLEAPYEAR(dt->year) && dt->month == 2 && dt->day == 29){
			;
		} else {
			output_fatal("Invalid day provided in datetime");
			return TS_INVALID;
		}
	}

	/* add month */
	for (n=1; n<dt->month; n++){
		if(ISLEAPYEAR(dt->year) && n == 2){
			ts += (daysinmonth[n - 1] + 1) * DAY;
		} else {
			ts += (daysinmonth[n - 1]) * DAY;
		}
		// ts += (daysinmonth[n - 1] + (n == 2 && ISLEAPYEAR(dt->year) ? 1 : 0)) * DAY;
	}

	if(dt->hour < 0 || dt->hour > 23 || dt->minute < 0 || dt->minute > 60 || dt->second < 0 || dt->second > 62){
		output_fatal("Invalid time of day provided in datetime");
		return ts;
	}
	/* add day, hour, minute, second, usecs */
	ts += (dt->day - 1) * DAY + dt->hour * HOUR + dt->minute * MINUTE + dt->second * SECOND + dt->nanosecond/1.0e9;

	if(dt->tz[0] == 0){
		strcpy(dt->tz, (isdst(ts) ? tzdst : tzstd));
	}
	/* adjust for GMT (or unspecified) */
	if (strcmp(dt->tz, "GMT") == 0){
		return ts;
	} else if(strcmp(dt->tz, tzstd) == 0 || ((strcmp(dt->tz, "")==0 && !isdst(ts) && (ts < dststart[dt->year - YEAR0] || ts >= dstend[dt->year - YEAR0])))){
		/* adjust to standard local time */
		return ts + tzoffset;
	} else if(strcmp(dt->tz, tzdst) == 0 || ((strcmp(dt->tz, "")==0 && !isdst(ts) && ts >= dststart[dt->year - YEAR0] && ts < dstend[dt->year - YEAR0]))){
		/* adjust to daylight local time */
		return ts + tzoffset - HOUR;
	} else {
		/* not a valid timezone */
		return TS_INVALID;
	}
	return TS_INVALID; // heading off 'no return path' warnings
}

/** Convert a datetime struct to a string
 **/
int strdatetime(DATETIME *t, char *buffer, int size){
	int len;
	char tbuffer[1024];

	if(t == NULL){
		output_error("strdatetime: null DATETIME pointer passed in");
		return 0;
	}

	if(buffer == NULL){
		output_error("strdatetime: null string buffer passed in");
		return 0;
	}

	/* choose best format */
	if(global_dateformat == DF_ISO){
		if(t->nanosecond != 0){
			len = sprintf(tbuffer, "%04d-%02d-%02d %02d:%02d:%02d.%09d %s",
				t->year, t->month, t->day, t->hour, t->minute, t->second, t->nanosecond, t->tz);
		} else {
			len = sprintf(tbuffer, "%04d-%02d-%02d %02d:%02d:%02d %s",
				t->year, t->month, t->day, t->hour, t->minute, t->second, t->tz);
		}
	} else if(global_dateformat == DF_US){
		if(t->nanosecond != 0){
			len = sprintf(tbuffer, "%02d-%02d-%04d %02d:%02d:%02d.%09d %s",
				t->month, t->day, t->year, t->hour, t->minute, t->second, t->nanosecond, t->tz);
		} else {
			len = sprintf(tbuffer, "%02d-%02d-%04d %02d:%02d:%02d %s",
				t->month, t->day, t->year, t->hour, t->minute, t->second, t->tz);
		}
	} else if(global_dateformat == DF_EURO){
		if(t->nanosecond != 0){
			len = sprintf(tbuffer,"%02d-%02d-%04d %02d:%02d:%02d.%09d %s",
				t->day, t->month, t->year, t->hour, t->minute, t->second, t->nanosecond,t->tz);
		} else {
			len = sprintf(tbuffer,"%02d-%02d-%04d %02d:%02d:%02d %s",
				t->day, t->month, t->year, t->hour, t->minute, t->second,t->tz);
		}
	} else {
		THROW("global_dateformat=%d is not valid", global_dateformat);
		/* TROUBLESHOOT
			The value of the global variable 'global_dateformat' is not valid.
			Check for attempts to set this variable and make sure that is one of the valid
			values (e.g., DF_ISO, DF_US, DF_EURO)
		 */
	}

	if(len < size){
		strncpy(buffer, tbuffer, len+1);
		return len;
	} else {
		output_error("strdatetime: timestamp larger than provided buffer");
		/*	TROUBLESHOOT
			The buffer provided to strdatetime was insufficiently large, or was otherwise packed with
			data beforehand.  The relevant code should use a larger buffer, or the output data set should
			be smaller.
		*/
		return 0;
	}
}

/** Computes the GMT time of a DST event
	Offset indicates the time offset to include
 **/
TIMESTAMP compute_dstevent(int year, SPEC *spec, time_t offset){
	TIMESTAMP t = TS_INVALID;
	int y, m, d, ndays = 0, day1;

	if(spec == NULL){
		output_error("compute_dstevent: null SPEC* pointer passed in");
		return -1;
	}

	/* check values */
	if (spec->day<0 || spec->day>7
		|| spec->hour<0 || spec->hour>23
		|| spec->minute<0 || spec->minute>59
		|| spec->month<0 || spec->month>11
		|| spec->nth<0 || spec->nth>5){
		output_error("compute_dstevent: date/time values are not valid");
		return -1;
	}

	/* calculate days */
	for (y = YEAR0; y < year; y++){
		ndays += 365 + (ISLEAPYEAR(y) ? 1 : 0);
	}

	for (m = 0; m < spec->month - 1; m++){
		ndays += daysinmonth[m] + ((m==1&&ISLEAPYEAR(y))?1:0);
	}

	day1 = (ndays + DOW0+7)%7; /* weekday of first day of month */
	d = ((7 - day1)%7+1 + (spec->nth - 1) * 7);

	while(d > daysinmonth[m] + ((m == 1 && ISLEAPYEAR(y)) ? 1 : 0)){
		d -= 7;
	}

	ndays += d-1;
	t = (ndays * 86400 + spec->hour * 3600 + spec->minute * 60);

	return t * TS_SECOND + tzoffset;
}

/** Extract information from an ISO timezone specification
 **/
int tz_info(char *tzspec, char *tzname, char *std, char *dst, time_t *offset){
	int hours = 0, minutes = 0;
	char buf1[32], buf2[32];
	int rv = 0;
	memset(buf1,0,sizeof(buf1));
	memset(buf2,0,sizeof(buf2));


	if ((strchr(tzspec, ':') != NULL ) && (sscanf(tzspec, "%[A-Z]%d:%d%[A-Z]", buf1, &hours, &minutes, buf2) < 3)){
		output_error("tz_info: \'%s\' not a timezone-format string", tzspec);
		return 0;
	}

	rv =  sscanf(tzspec, "%[A-Z]%d%[A-Z]", buf1, &hours, buf2);
	if(rv < 2){
		output_error("tz_info: \'%s\' not a timezone-format string", tzspec);
		return 0;
	}

	if (hours < -12 || hours > 12){
		output_error("timezone %s (%s) has out-of-bounds hour offset of %i", tzname, std, hours);
		return 0;
	}

	if (minutes < 0 || minutes > 59){
		output_error("timezone %s (%s) has out-of-bounds minutes offset of %i", tzname, std, minutes);
		return 0;
	}

	if ( std!=NULL )
	{
		strcpy(std, buf1);
	}
	
	if ( rv>2 && dst!=NULL )
	{
		strcpy(dst, buf2);
	}

	if(minutes == 0) {
		if(tzname){
			sprintf(tzname, "%s%d%s", buf1, hours, (rv == 2 ? "" : buf2));
		}

		if(offset){
			*offset = hours * 3600;
		}

		return 1;
	} else {
		if ( tzname!=NULL )
		{
			sprintf(tzname, "%s%d:%02d%s", buf1, hours, minutes, buf2);
		}
		
		if ( offset!=NULL )
		{
			*offset = hours * 3600 + minutes * 60;
		}

		return 2;
	}
}

char *tz_locale(char *country, char *province, char *city)
{
	extern char *tz_name(char *tzspec);
	static char tzname[256]="";
	char filepath[1024];
	FILE *fp = NULL;
	char buffer[1024];
	char target[256];
	int len = sprintf(target,"%s/%s/%s",country,province,city);

	if(find_file(TZFILE, NULL, R_OK,filepath,sizeof(filepath)) == NULL){
		THROW("timezone specification file %s not found in GLPATH=%s: %s", TZFILE, getenv("GLPATH"), strerror(errno));
		/* TROUBLESHOOT
			The system could not locate the timezone file <code>tzinfo.txt</code>.
			Check that the <code>etc</code> folder is included in the '''GLPATH''' environment variable and try again.
		 */
	}
	fp = fopen(filepath,"r");
	if(fp == NULL){
		THROW("%s: access denied: %s", filepath, strerror(errno));
		/* TROUBLESHOOT
			The system was unable to read the timezone file.  Check that the file has the correct permissions and try again.
		 */
	}
	while( fgets(buffer,sizeof(buffer),fp)!=NULL )
	{
		char *locale = buffer;
		if ( locale[0]==';' || locale[0]=='\0' ) continue;
		if ( iswspace(locale[0]) )
		{
			while ( iswspace(locale[0]) && locale[0]!='\0' ) locale++; /* trim left white */
			if ( strnicmp(locale,target,len)==0 )
			{
				fclose(fp);
				return tz_name(tzname);
			}
		}
		else
			sscanf(buffer, "%[^,]", tzname);
	}
	THROW("tz_locale(char *country='%s', char *province='%s', char *city='%s'): not tzinfo entry found", country, province, city);
	return NULL;
}

/** Converts a timezone spec into a standard timezone name
	Populate tzspec if provided, otherwise returns a static buffer
 **/
char *tz_name(char *tzspec)
{
	static char name[32] = "GMT";
	char country[64], province[64], city[64];

	if ( sscanf(tzspec,"%[^/]/%[^/]/%[^/]",country,province,city)==3 )
		return tz_locale(country,province,city);

	if(tz_info(tzspec, name, NULL, NULL, NULL))
	{
		return name;
	} 
	else 
	{
		//output_error("unable to find timezone name for tzspec \'%s\'", tzspec);
		/*	don'tTROUBLESHOOT
			The timezone specification was not found in the timezone subsystem.  Double-check the spelling and format of the specification.
		*/
		return NULL;
	}
	//return (tz_info(tzspec, name, NULL, NULL, NULL) ? name : NULL);
}

/** Compute the offset of a tz spec
 **/
time_t tz_offset(char *tzspec){
	time_t offset;

	if(tz_info(tzspec, NULL, NULL, NULL, &offset)){
		return offset;
	} else {
		return -1;
	}
	//return tz_info(tzspec,NULL,NULL,NULL,&offset)?offset:-1;
}

/** Get the std timezone name
 **/
char *tz_std(char *tzspec){
	static char std[32] = "GMT";

	if(tz_info(tzspec, NULL, std, NULL, NULL)){
		return std;
	} else {
		return "GMT";
	}
	//return tz_info(tzspec, NULL, std, NULL, NULL) ? std : "GMT";
}

/** Get the std timezone name
 **/
char *tz_dst(char *tzspec){
	static char dst[32]="GMT";

	if(tz_info(tzspec,NULL,NULL,dst,NULL)){
		return dst;
	} else {
		return "GMT";
	}
	//return tz_info(tzspec,NULL,NULL,dst,NULL)?dst:"GMT";
}

/** Apply a timezone spec to the current tz rules
 **/
void set_tzspec(int year, char *tzname, SPEC *pStart, SPEC *pEnd){
	int y;


	for (y = year - YEAR0; y < sizeof(tszero) / sizeof(tszero[0]); y++)
	{
		if (pStart!=NULL && pEnd!=NULL) // no DST events (cf. ticket:372)
		{
			//Look for southern hemisphere items (or reversed DST, in general)
			if (pStart->month > pEnd->month)
			{
				dststart[y] = compute_dstevent(y + YEAR0, pStart, tzoffset);
				dstend[y] = compute_dstevent(y + YEAR0 + 1, pEnd, tzoffset);
			}
			else	//"Standard" northern hemisphere rules
			{
				dststart[y] = compute_dstevent(y + YEAR0, pStart, tzoffset);
				dstend[y] = compute_dstevent(y + YEAR0, pEnd, tzoffset);
			}
		}
		else
			dststart[y] = dstend[y] = -1;
	}
}

/** Load a timezone from the timezone info file
 **/
void load_tzspecs(char *tz){
	char filepath[1024];
	char *pTzname = 0;
	FILE *fp = NULL;
	char buffer[1024];
	int linenum = 0;
	int year = YEAR0;
	int y;
	int found;

	found = 0;
	tzvalid = 0;
	pTzname = tz_name(tz);

	if(pTzname == 0){
		THROW("timezone '%s' was not understood by tz_name.", tz);
		/* TROUBLESHOOT
			The specific timezone is not valid.
			Try using a valid timezone or add the desired timezone to the timezone file <code>.../etc/tzinfo.txt</code> and try again.
		 */
	}

	strncpy(current_tzname, pTzname, sizeof(current_tzname));
	tzoffset = tz_offset(current_tzname);
	strncpy(tzstd, tz_std(current_tzname), sizeof(tzstd));
	strncpy(tzdst, tz_dst(current_tzname), sizeof(tzdst));

	if(find_file(TZFILE, NULL, R_OK,filepath,sizeof(filepath)) == NULL){
		THROW("timezone specification file %s not found in GLPATH=%s: %s", TZFILE, getenv("GLPATH"), strerror(errno));
		/* TROUBLESHOOT
			The system could not locate the timezone file <code>tzinfo.txt</code>.
			Check that the <code>etc</code> folder is included in the '''GLPATH''' environment variable and try again.
		 */
	}

	fp = fopen(filepath,"r");

	if(fp == NULL){
		THROW("%s: access denied: %s", filepath, strerror(errno));
		/* TROUBLESHOOT
			The system was unable to read the timezone file.  Check that the file has the correct permissions and try again.
		 */
	}

	// zero previous DST start/end times
	for (y = 0; y < sizeof(tszero) / sizeof(tszero[0]); y++){
		dststart[y] = dstend[y] = -1;
	}

	while(fgets(buffer,sizeof(buffer),fp)){
		char *p = NULL;
		char tzname[32];
		SPEC start, end;
		int form = -1;

		linenum++;

		/* wipe comments */
		p = strchr(buffer,';');

		if(p != NULL){
			*p = '\0';
		}

		/* remove trailing whitespace */
		p = buffer + strlen(buffer) - 1;

		while (iswspace(*p) && p > buffer){
			*p-- = '\0';
		}

		/* ignore blank lines or lines starting with white space*/
		if (buffer[0] == '\0' || iswspace(buffer[0])){
			continue;
		}

		/* year section */
		if(sscanf(buffer, "[%d]", &year) == 1){
			continue;
		}

		/* TZ spec */
		form = sscanf(buffer, "%[^,],M%d.%d.%d/%d:%d,M%d.%d.%d/%d:%d", tzname,
			&start.month, &start.nth, &start.day, &start.hour, &start.minute,
			&end.month, &end.nth, &end.day, &end.hour, &end.minute);

		/* load only TZ requested */
		pTzname = tz_name(tzname);

		if (tz != NULL && pTzname != NULL && strcmp(pTzname,current_tzname) != 0){
			continue;
		}

		if(form == 1){ /* no DST */
			set_tzspec(year, current_tzname, NULL, NULL);
			found = 1;
		} else if(form == 11) { /* full DST spec */
			set_tzspec(year, current_tzname, &start, &end);
			found = 1;
		} else {
			THROW("%s(%d): %s is not a valid timezone spec", filepath, linenum, buffer);
			/* TROUBLESHOOT
				The timezone specification is not valid.  Verify the syntax of the timezone spec and that it is defined in the timezone file
				<code>.../etc/tzinfo.txt</code> or add it, and try again.
			 */
		}
	}

	if(found == 0){
		output_warning("%s(%d): timezone spec '%s' not found in 'tzinfo.txt', will include no DST information", filepath, linenum, current_tzname);
	}

	if(ferror(fp)){
		output_error("%s(%d): %s", filepath, linenum, strerror(errno));
	} else {
		output_verbose("%s loaded ok", filepath);
	}

	fclose(fp);
	tzvalid = 1;
}

/** Establish the default timezone for time conversion.
	\p NULL \p tzname uses \p TZ environment for default
 **/
char *timestamp_set_tz(char *tz_name)
{
	if (tz_name == NULL){
		tz_name=getenv("TZ");
	}

	if(tz_name == NULL)
	{
		static char guess[64];
		static unsigned int tzlock=0;

		if (strcmp(_tzname[0], "") == 0){
			THROW("timezone not identified");
			/* TROUBLESHOOT
				An attempt to use timezones was made before the timezome has been specified.  Try adding or moving the
				timezone spec to the top of the <code>clock</code> directive and try again.  Alternatively, you can set the '''TZ''' environment
				variable.

			 */
		}

		wlock(&tzlock);
		if (_timezone % 60 == 0){
			sprintf(guess, "%s%d%s", _tzname[0], (int)(_timezone / 3600), _daylight?_tzname[1]:"");
		} else {
			sprintf(guess, "%s%d:%d%s", _tzname[0], (int)(_timezone / 3600), (int)(_timezone / 60), _daylight?_tzname[1]:"");
		}
		if (_timezone==0 && _daylight==0)
			tz_name="UTC0";
		else
			tz_name = guess;
		wunlock(&tzlock);
	}

	load_tzspecs(tz_name);

	return current_tzname;
}

/** Convert from a timestamp to a string - no delta time
	Created to avoid having to replace gl_printtime calls everywhere
**/
int convert_from_timestamp(TIMESTAMP ts, char *buffer, int size)
{
	return convert_from_timestamp_delta(ts,0,buffer,size);
}

/** Convert from a timestamp to a string
	Delta-compatible version
 **/
int convert_from_timestamp_delta(TIMESTAMP ts, DELTAT delta_t, char *buffer, int size)
{
	double dt_time;
	unsigned int nano_seconds;
	char temp[64]="INVALID";
	int len=(int)strlen(temp);

	if (delta_t != 0)
	{
		/* Figure out the offset the delta time represents */
		dt_time = (double)ts + (double)delta_t/(double)DT_SECOND;

		/* Convert to integer */
		ts = (int64)dt_time;

		/* Figure out the nanosecond portion - bias slightly - similar code in recorders*/
		nano_seconds = (unsigned int)((dt_time - (double)(ts))*1e9 + 0.5);
	}
	else
		nano_seconds = 0;

	if (ts>=365*DAY)
	{	DATETIME t;
		if (ts>=0)
		{
			if (ts<TS_NEVER)
			{
				if (local_datetime(ts,&t))
				{
					if (nano_seconds != 0)
					{
						t.nanosecond = nano_seconds;
					}
					len = strdatetime(&t,temp,sizeof(temp));
				}
				else
					THROW("%"FMT_INT64"d is an invalid timestamp", ts);
					/* TROUBLESHOOT
						An attempt to convert a timestamp to a date/time string has failed because the timezone isn't valid.
						This is most likely an internal error and should be reported.
					 */
			}
			else
				len=sprintf(temp,"%s","NEVER");
		}
	}
	else if (ts>=DAY)
		len=sprintf(temp,"%lfd",(double)ts/DAY);
	else if (ts>=HOUR)
		len=sprintf(temp,"%lfh",(double)ts/HOUR);
	else if (ts>=MINUTE)
		len=sprintf(temp,"%lfm",(double)ts/MINUTE);
	else if (ts>=SECOND)
		len=sprintf(temp,"%lfs",(double)ts/SECOND);
	else if (ts==0)
		len=sprintf(temp,"%s","INIT");
	else
		len=sprintf(temp,"%"FMT_INT64"d",ts);
	if (len<size)
	{
		if(ts == TS_NEVER){
			strcpy(buffer, "NEVER");
			return (int)strlen("NEVER");
		}
		strcpy(buffer,temp);
		return len;
	}
	else
		return 0;
}

/** Convert from a timestamp to a string -- deltamode
 **/
int convert_from_delta_timestamp(double ts_v, char *buffer, int size)
{
	TIMESTAMP ts;
	unsigned int nano_seconds;
	char temp[64]="INVALID";
	int len=(int)strlen(temp);

	/* Convert to integer */
	ts = (int64)ts_v;

	/* Figure out the nanosecond portion - bias slightly*/
	nano_seconds = (unsigned int)((ts_v - (double)(ts))*1e9 + 0.5);

	if (ts>=365*DAY)
	{	DATETIME t;
		if (ts>=0)
		{
			if (ts<TS_NEVER)
			{
				if (local_datetime(ts,&t))
				{
					t.nanosecond = nano_seconds;
					len = strdatetime(&t,temp,sizeof(temp));
				}
				else
					THROW("%"FMT_INT64"d is an invalid timestamp", ts);
					/* TROUBLESHOOT
						An attempt to convert a timestamp to a date/time string has failed because the timezone isn't valid.
						This is most likely an internal error and should be reported.
					 */
			}
			else
				len=sprintf(temp,"%s","NEVER");
		}
	}
	else if (ts>=DAY)
		len=sprintf(temp,"%lfd",(double)ts/DAY);
	else if (ts>=HOUR)
		len=sprintf(temp,"%lfh",(double)ts/HOUR);
	else if (ts>=MINUTE)
		len=sprintf(temp,"%lfm",(double)ts/MINUTE);
	else if (ts>=SECOND)
		len=sprintf(temp,"%lfs",(double)ts/SECOND);
	else if (ts==0)
		len=sprintf(temp,"%s","INIT");
	else
		len=sprintf(temp,"%"FMT_INT64"d",ts);
	if (len<size)
	{
		if(ts == TS_NEVER){
			strcpy(buffer, "NEVER");
			return (int)strlen("NEVER");
		}
		strcpy(buffer,temp);
		return len;
	}
	else
		return 0;
}

/** Convert from a string to a timestamp
 **/
TIMESTAMP convert_to_timestamp(const char *value)
{
	/* try date-time format */
	int Y=0,m=0,d=0,H=0,M=0,S=0;
	char tz[5]="";
	if (*value=='\'' || *value=='"') value++;
	/* scan ISO format date/time */
	if (sscanf(value,"%d-%d-%d %d:%d:%d %[-+:A-Za-z0-9]",&Y,&m,&d,&H,&M,&S,tz)>=3)
	{
		int isdst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		DATETIME dt = {Y,m,d,H,M,S,0,isdst}; /* use GMT if tz is omitted */
		strncpy(dt.tz,tz,sizeof(dt.tz));
		return mkdatetime(&dt);
	}
	/* scan ISO format date/time */
	else if (global_dateformat==DF_ISO && sscanf(value,"%d/%d/%d %d:%d:%d %[-+:A-Za-z0-9]",&Y,&m,&d,&H,&M,&S,tz)>=3)
	{
		int isdst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		DATETIME dt = {Y,m,d,H,M,S,0,isdst}; /* use locale TZ if tz is omitted */
		strncpy(dt.tz,tz,sizeof(dt.tz));
		return mkdatetime(&dt);
	}
	/* scan US format date/time */
	else if (global_dateformat==DF_US && sscanf(value,"%d/%d/%d %d:%d:%d %[-+:A-Za-z0-9]",&m,&d,&Y,&H,&M,&S,tz)>=3)
	{
		int isdst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		DATETIME dt = {Y,m,d,H,M,S,0,isdst}; /* use locale TZ if tz is omitted */
		strncpy(dt.tz,tz,sizeof(dt.tz));
		return mkdatetime(&dt);
	}
	/* scan EURO format date/time */
	else if (global_dateformat==DF_EURO && sscanf(value,"%d/%d/%d %d:%d:%d %[-+:A-Za-z0-9]",&d,&m,&Y,&H,&M,&S,tz)>=3)
	{
		int isdst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		DATETIME dt = {Y,m,d,H,M,S,0,isdst}; /* use locale TZ if tz is omitted */
		strncpy(dt.tz,tz,sizeof(dt.tz));
		return mkdatetime(&dt);
	}
	/* @todo support European format date/time using some kind of global flag */
	else if (strcmp(value,"INIT")==0)
		return 0;
	else if (strcmp(value, "NEVER")==0)
		return TS_NEVER;
	else if (strcmp(value, "NOW") == 0)
		return global_clock;
	else if (isdigit(value[0]))
	{	/* timestamp format */
		double t = atof(value);
		char *p=value;
		while (isdigit(*p) || *p=='.') p++;
		switch (*p) {
		case 's':
		case 'S':
			t *= SECOND;
			break;
		case 'm':
		case 'M':
			t *= MINUTE;
			break;
		case 'h':
		case 'H':
			t *= HOUR;
			break;
		case 'd':
		case 'D':
			t *= DAY;
			break;
		default:
			return TS_NEVER;
			break;
		}
		return (TIMESTAMP)(t+0.5);
	}
	else
		return TS_NEVER;
}

/** Convert from a string to a timestamp -- delta compatibility
 **/
TIMESTAMP convert_to_timestamp_delta(const char *value, unsigned int *nanoseconds, double *dbl_time_value)
{
	/* Declarations - inline ones make angry (since we're technically in C) */
	DATETIME dt;
	double seconds_w_nano, t;
	char *p;
	char tz[5]="";
	TIMESTAMP output_value;

	if (*value=='\'' || *value=='"') value++;

	/* By default, nanoseconds is set to 0 */
	*nanoseconds = 0;

	/* Init double value */
	*dbl_time_value = 0.0;

	/* Zero the DATETIME structure */
	dt.year = 0;
	dt.month = 0;
	dt.day = 0;
	dt.hour = 0;
	dt.minute = 0;
	dt.second = 0;
	dt.nanosecond = 0;
	dt.is_dst = 0;
	dt.tz[0] = '\0';
	dt.tzoffset = 0;
	dt.timestamp = 0;

	/* Default to TS_NEVER */
	output_value = TS_NEVER;

	/* scan ISO format date/time -- nanosecond inclusive */
	if (sscanf(value,"%d-%d-%d %d:%d:%lf %[-+:A-Za-z0-9]",&dt.year,&dt.month,&dt.day,&dt.hour,&dt.minute,&seconds_w_nano,tz)>=3)
	{
		dt.second = (unsigned int)seconds_w_nano;
		dt.nanosecond = (unsigned int)(1e9*(seconds_w_nano-(double)dt.second)+0.5);
		dt.is_dst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		*nanoseconds = dt.nanosecond;
		strncpy(dt.tz,tz,sizeof(dt.tz));
		output_value = mkdatetime(&dt);
	}
	/* scan ISO format date/time -- nanosecond inclusive */
	else if (global_dateformat==DF_ISO && sscanf(value,"%d/%d/%d %d:%d:%lf %[-+:A-Za-z0-9]",&dt.year,&dt.month,&dt.day,&dt.hour,&dt.minute,&seconds_w_nano,tz)>=3)
	{
		dt.second = (unsigned int)seconds_w_nano;
		dt.nanosecond = (unsigned int)(1e9*(seconds_w_nano-(double)dt.second)+0.5);
		dt.is_dst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		*nanoseconds = dt.nanosecond;
		strncpy(dt.tz,tz,sizeof(dt.tz));
		output_value = mkdatetime(&dt);
	}
	/* scan US format date/time -- nanosecond inclusive */
	else if (global_dateformat==DF_US && sscanf(value,"%d/%d/%d %d:%d:%lf %[-+:A-Za-z0-9]",&dt.month,&dt.day,&dt.year,&dt.hour,&dt.minute,&seconds_w_nano,tz)>=3)
	{
		dt.second = (unsigned int)seconds_w_nano;
		dt.nanosecond = (unsigned int)(1e9*(seconds_w_nano-(double)dt.second)+0.5);
		dt.is_dst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		*nanoseconds = dt.nanosecond;
		strncpy(dt.tz,tz,sizeof(dt.tz));
		output_value = mkdatetime(&dt);
	}
	/* scan EURO format date/time -- nanosecond inclusive */
	else if (global_dateformat==DF_EURO && sscanf(value,"%d/%d/%d %d:%d:%lf %[-+:A-Za-z0-9]",&dt.day,&dt.month,&dt.year,&dt.hour,&dt.minute,&seconds_w_nano,tz)>=3)
	{
		dt.second = (unsigned int)seconds_w_nano;
		dt.nanosecond = (unsigned int)(1e9*(seconds_w_nano-(double)dt.second)+0.5);
		dt.is_dst = (strcmp(tz,tzdst)==0) ? 1 : 0;
		*nanoseconds = dt.nanosecond;
		strncpy(dt.tz,tz,sizeof(dt.tz));
		output_value = mkdatetime(&dt);
	}
	/* @todo support European format date/time using some kind of global flag */
	else if (strcmp(value,"INIT")==0)
		output_value = 0;
	else if (strcmp(value, "NEVER")==0)
		output_value = TS_NEVER;
	else if (strcmp(value, "NOW") == 0)
		output_value = global_clock;
	else if (isdigit(value[0]))
	{	/* timestamp format */
		t = atof(value);
		p=value;
		while (isdigit(*p) || *p=='.') p++;
		switch (*p) {
			case 's':
			case 'S':
				t *= SECOND;
				break;
			case 'm':
			case 'M':
				t *= MINUTE;
				break;
			case 'h':
			case 'H':
				t *= HOUR;
				break;
			case 'd':
			case 'D':
				t *= DAY;
				break;
			default:
				return TS_NEVER;
				break;
		}
		output_value = (TIMESTAMP)(t+0.5);
	}
	else
		output_value = TS_NEVER;

	/* Perform double conversion */
	*dbl_time_value = (double)(output_value) + ((double)(dt.nanosecond)/1000000000.0);

	return output_value;
}

double timestamp_to_days(TIMESTAMP t)
{
	return (double)t/DAY;
}

double timestamp_to_hours(TIMESTAMP t)
{
	return (double)t/HOUR;
}

double timestamp_to_minutes(TIMESTAMP t)
{
	return (double)t/MINUTE;
}

double timestamp_to_seconds(TIMESTAMP t)
{
	return (double)t/SECOND;
}

/** Test the daylight saving time calculations
	@return the number of test the failed
 **/
int timestamp_test(void)
{
#define NYEARS 50
	int year;
	static DATETIME last_t;
	TIMESTAMP step = SECOND;
	TIMESTAMP ts;
	char buf1[64], buf2[64];
	char steptxt[32];
	TIMESTAMP *event[]={dststart,dstend};
	int failed=0, succeeded=0;

	output_test("BEGIN: daylight saving time event test for TZ=%s...", current_tzname);
	convert_from_timestamp(step,steptxt,sizeof(steptxt));
	for (year=0; year<NYEARS; year++)
	{
		int test;
		for (test=0; test<2; test++)
		{
			for (ts=(event[test])[year]-2*step; ts<(event[test])[year]+2*step;ts+=step)
			{
				DATETIME t;
				if (local_datetime(ts,&t))
				{
					if (last_t.is_dst!=t.is_dst)
						output_test("%s + %s = %s", strdatetime(&last_t,buf1,sizeof(buf1))?buf1:"(invalid)", steptxt, strdatetime(&t,buf2,sizeof(buf2))?buf2:"(invalid)");
					last_t = t;
					succeeded++;
				}
				else
				{
					output_test("FAILED: unable to convert ts=%"FMT_INT64"d to local time", ts);
					failed++;
				}
			}
		}
	}
	output_test("END: daylight saving time event test");

	step=HOUR;
	convert_from_timestamp(step,steptxt,sizeof(steptxt));
	output_test("BEGIN: round robin test at %s timesteps",steptxt);
	for (ts=DAY+tzoffset; ts<DAY*365*NYEARS; ts+=step)
	{
		DATETIME t;
		if (local_datetime(ts,&t))
		{
			TIMESTAMP tt = mkdatetime(&t);
			convert_from_timestamp(ts,buf1,sizeof(buf1));
			convert_from_timestamp(tt,buf2,sizeof(buf2));
			if (tt==TS_INVALID)
			{
				output_test("FAILED: unable to extract %04d-%02d-%02d %02d:%02d:%02d %s (dow=%s, doy=%d)", t.year,t.month,t.day,t.hour,t.minute,t.second,t.tz,dow[t.weekday],t.yearday);
				failed++;
			}
			else if (tt!=ts)
			{
				output_test("FAILED: unable to match %04d-%02d-%02d %02d:%02d:%02d %s (dow=%s, doy=%d)\n    from=%s, to=%s", t.year,t.month,t.day,t.hour,t.minute,t.second,t.tz,dow[t.weekday],t.yearday,buf1,buf2);
				failed++;
			}
			else if (convert_to_timestamp(buf1)!=ts)
			{
				output_test("FAILED: unable to convert %04d-%02d-%02d %02d:%02d:%02d %s (dow=%s, doy=%d) back to a timestamp\n    from=%s, to=%s", t.year,t.month,t.day,t.hour,t.minute,t.second,t.tz,dow[t.weekday],t.yearday,buf1,buf2);
				output_test("        expected %" FMT_INT64 "d but got %" FMT_INT64 "d", ts, convert_to_timestamp(buf1));
				failed++;
			}
			else
				succeeded++;
		}
		else
		{
			output_test("FAILED: timestamp_test: unable to convert ts=%"FMT_INT64"d to local time", ts);
			failed++;
		}
	}
	output_test("END: round robin test",steptxt);
	output_test("END: daylight saving time tests for %d to %d", YEAR0, YEAR0+NYEARS);
	output_verbose("daylight saving time tests: %d succeeded, %d failed (see '%s' for details)", succeeded, failed, global_testoutputfile);
	return failed;
}

double timestamp_get_part(void *x, char *name)
{
	TIMESTAMP ts = *(TIMESTAMP*)x;
	DATETIME dt;
	if ( strcmp(name,"seconds")==0 ) return (double)ts;
	if ( strcmp(name,"minutes")==0 ) return (double)ts/60;
	if ( strcmp(name,"hours")==0 ) return (double)ts/3600;
	if ( strcmp(name,"days")==0 ) return (double)ts/86400;
	if ( local_datetime(ts,&dt) )
	{
		if ( strcmp(name,"second")==0 ) return (double)dt.second;
		if ( strcmp(name,"minute")==0 ) return (double)dt.minute;
		if ( strcmp(name,"hour")==0 ) return (double)dt.hour;
		if ( strcmp(name,"day")==0 ) return (double)dt.day;
		if ( strcmp(name,"month")==0 ) return (double)dt.month;
		if ( strcmp(name,"year")==0 ) return (double)dt.year;
		if ( strcmp(name,"weekday")==0 ) return (double)dt.weekday;
		if ( strcmp(name,"yearday")==0 ) return (double)dt.yearday;
		if ( strcmp(name,"isdst")==0 ) return (double)dt.is_dst;
	}
	return QNAN;
}

/** Compute the absolute timestamp (removes soft/hard time distinction)
 **/
TIMESTAMP absolute_timestamp(TIMESTAMP t)
{
	/* only valid soft times are converted */
	return (t>=-TS_MAX && t<0 ? -t : t);
}
/** Determine which timestamp reflects the earliest time
 **/
TIMESTAMP earliest_timestamp(TIMESTAMP t, ...)
{
	TIMESTAMP t1 = t, t2;
	TIMESTAMP at1 = absolute_timestamp(t1), at2;
	va_list ptr;
	va_start(ptr,t);
	while ( (t2=va_arg(ptr,TIMESTAMP)) != 0 )
	{
		at2 = absolute_timestamp(t2);
		if ( at2<at1 )
		{
			t1 = t2;
			at1 = at2;
		}
	};
	va_end(ptr);
	return t1;
}

/**@}*/
