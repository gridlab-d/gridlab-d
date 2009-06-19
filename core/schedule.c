/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.c
	@addtogroup schedule
**/

#include "platform.h"
#include "schedule.h"

static SCHEDULE *schedule_list = NULL;

/* finds a schedule by its name */
SCHEDULE *schedule_find_byname(char *name)
{
	SCHEDULE *sch;
	for (sch=schedule_list; sch!=NULL; sch=sch->next)
	{
		if (strcmp(sch->name,name)==0)
			return sch;
	}
	return NULL;
}

/* performs a schedule pattern match 
   patterns:
     *
	 #
	 #-#
	 ...,...
	 
 */
int schedule_matcher(char *pattern, unsigned char *table)
{
	int go=0;
	int start=0;
	int stop=0;
	int range=0;
	char *p = pattern;
	while (*p!='\0')
	{
		switch (*p) {
		case '*':
			/* full range and go fill */
			start=0; stop=60; go=1;
			break;
		case ',':
			/* go fill */
			go = 1;
			break;
		case '-':
			/* partial range */
			range = 1;
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (range)
				stop = start*10 + (*p-'0');
			else
				stop = start = start*10 + (*p-'0');
			break;
		default:
			return 0;
			break;
		}
		if (go)
		{	/* go fill */
			int i;
			for (i=start; i<stop; i++)
				table[i] = 1;
			/* reset */
			start = stop = range = go = 0;
		}
	}

	return 1;
}

/* compiles the schedule and report errors
   returns 1 on success, 0 on failure 
 */
int schedule_compile(SCHEDULE *sch)
{
	char *token = NULL;
	unsigned char index=0;
	unsigned int minute=0;
	memset(sch->data,0,sizeof(sch->data));
	memset(sch->index,0,sizeof(sch->index));
	/* first index is always value 0 */
	for (index=1; (token=strtok(token==NULL?sch->definition:NULL,";"))!=NULL; index++)
	{
		char moh[256], hod[256], dom[256], moy[256], dow[256];
		unsigned char *minute_match, *hour_match, *day_match, *month_match, *weekday_match;
		unsigned int weekday;
		double value=1.0; /* default value is 1.0 */
		if (sscanf(token,"%s %s %s %s %s %f",moh,hod,dom,moy,dow,&value))<5) /* value can be missing -> defaults to 1.0 */
		{
			output_error("schedule_compile(SCHEDULE *sch='{name=%s, ...}') ignored an invalid definition '%s'", token);
			continue;
		}
		else
			sch->data[index] = value;

		/* get match tables */
		if (!schedule_matcher(moh,minute_match))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) minute syntax error in item '%s'", sch->name, token);
			return 0;
		}
		if (!schedule_matcher(hod,hour_match))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) hour syntax error in item '%s'", sch->name, token);
		if (!schedule_matcher(dom,day_match))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) day syntax error in item '%s'", sch->name, token);
			return 0;
		}
		if (!schedule_matcher(moy,month_match))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) month syntax error in item '%s'", sch->name, token);
			return 0;
		}
		if (!schedule_matcher(dow,weekday_match))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) weekday syntax error in item '%s'", sch->name, token);
			return 0;
		}

		/* load schedule */
		for (weekday=0; weekday<7; weekday++)
		{
			unsigned int is_leapyear;
			if (!weekday_match[weekday])
				continue;
			for (is_leapyear=0; is_leapyear<2; is_leapyear++)
			{
				unsigned int calendar = weekday*2+is_leapyear;
				unsigned int month;
				unsigned int days = {31,(is_leapyear?29:28),31,30,31,30,31,31,30,31,30,31};
				minute = 0;
				for (month=0; month<12; month++)
				{
					unsigned int day;
					if (!month_match[month])
						continue;
					for (day=0; day<days; day++)
					{
						unsigned int hour;
						if (!day_match[day])
							continue;
						for (hour=0; hour<24; hour++)
						{
							if (!hour_match[hour])
								continue;
							do {
								if (minute_match[minute])
									sch->index[calendar][minute] = index;
								minute++;
							} while (minute%60>0);
						}
					}
				}
			}
		}
	}
}

/** Create a schedule. Example:
	<code>schedule_create("weekdays 8am-5pm 100%, weekends 9-noon 50%","* 8-17 * * 1-5; * 9-12 * * 0,6 0.5");</code>
	@return a pointer to the new schedule, NULL if failed
 **/
SCHEDULE *schedule_create(char *name,		/**< the name of the schedule */
						  char *definition)	/**< the definition of the schedule (using crontab format with semicolon delimiters) */
{
	SCHEDULE *sch = schedule_find_byname(name);
	if (sch!=NULL) 
		return sch;

	/* create the schedule */
	sch = (SCHEDULE*)malloc(sizeof(SCHEDULE));
	if (sch==NULL)
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') memory allocation failed)", name, definition);
		/* TROUBLESHOOT
			The schedule module could not allocate enough memory to create a schedule item.  Try freeing system memory and try again.
		 */
		return NULL;
	}
	if (strlen(name)>=sizeof(sch->name))
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') memory allocation failed)", name, definition);
		/* TROUBLESHOOT
			The name given the schedule is too long to be used.  Use a name that is less than 64 characters and try again.
		 */
		free(sch);
		return NULL;
	}
	strcpy(sch->name,name);
	if (strlen(definition)>=sizeof(sch->definition))
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') memory allocation failed)", name, definition);
		/* TROUBLESHOOT
			The definition given the schedule is too long to be used.  Use a definition that is less than 1024 characters and try again.
		 */
		free(sch);
		return NULL;
	}
	strcpy(sch->definition,definition);

	/* compile the schedule */
	if (schedule_compile(sch))
		return sch;
	else
	{
		/* error message should be given by schedule_compile */
		free(sch);
		return NULL;
	}
}

/** normalizes a schedule, if possible
	@note the sum of the values is equal to 1.0, not the sum of the absolute values
 **/
void schedule_normalize(SCHEDULE *sch)
{
	unsigned int i;
	double sum=0;
	for (i=0; i<sizeof(sch->index)/sizeof(sch->index[0]); i++)
		sum += sch->data[i];
	if (sum!=0)
	{
		for (i=0; i<sizeof(sch->index)/sizeof(sch->index[0]); i++)
			sch->data[i]/=sum;
	}
}

#define ISLEAPYEAR(Y) ((Y)%4==0 && ((Y)%100!=0 || (Y)%400==0))

/** reads the value on the schedule for the given timestamp 
 **/
double schedule_read(SCHEDULE *sch, TIMESTAMP ts)
{
	int calendar = 0;
	int minute = 0;
	DATETIME dt;
	
	/* determine the local time */
	if (!local_datetime(ts,&dt))
	{
		char tsbuf[256];
		output_error("schedule_read(SCHEDULE *schedule={name='%s',...}, TIMESTAMP ts=%"FMT_INT64"d) unable to determine local time", sch->name, ts);
		/* TROUBLESHOOT
			The schedule could not be read because the local time could not be determined.  
			Fix the problem causing the local time system failure and try again.
		 */
		return NaN;
	}

	/* determine which calendar is used based on the weekday of Jan 1 and LY status */
	calendar = dt.weekday + ISLEAPYAR(dt.year);

	/* compute the minute of year */
	minute = (dt.yearday*24 + dt.hour)*60 + dt.minute;

	/* got it */
	return sch->data[index[calendar][minute]];
}
