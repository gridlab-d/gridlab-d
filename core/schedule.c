/** $Id$
 	Copyright (C) 2008 Battelle Memorial Institute
	@file schedule.c
	@addtogroup schedule

**/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#include "platform.h"
#include "output.h"
#include "schedule.h"
#include "exception.h"

#ifndef QNAN
#define QNAN sqrt(-1)
#endif

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
int schedule_matcher(char *pattern, unsigned char *table, int max)
{
	int go=0;
	int start=0;
	int stop=0;
	int range=0;
	char *p;
	memset(table,0,max);
	for (p=pattern; ; p++)
	{
		switch (*p) {
		case '\0':
			go = 1;
			break;
		case '*':
			/* full range and go fill */
			start=0; stop=max-1; go=1;
			break;
		case ',':
			/* go fill */
			go = 1;
			break;
		case '-':
			/* partial range */
			range = 1; 
			stop = 0;
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
				stop = stop*10 + (*p-'0');
			else
				stop = start = start*10 + (*p-'0');
			break;
		default:
			return 0;
			break;
		}
		if (go)
		{	int i;

			/* check over limit */
			if (stop>=max)
			{
				output_warning("schedule_matcher(char *pattern='%s',...) end exceed max of %d", pattern,max);
				stop = max-1;
			}

			/* go fill */
			if (start>stop) /* wraparound */
			{
				for (i=stop; i<max; i++)
					table[i] = 1;
				for (i=0; i<=start; i++)
					table[i] = 1;
			}
			else
			{
				for (i=start; i<=stop; i++)
					table[i] = 1;
			}
			/* reset */
			start = stop = range = go = 0;
		}
		if (*p=='\0')
			break;
	}

	return 1;
}

/* compiles a single schedule block and report errors
   returns 1 on success, 0 on failure 
 */
int schedule_compile_block(SCHEDULE *sch, char *blockdef)
{
	char *token = NULL;
	unsigned char index=0;
	unsigned int minute=0;

	/* check block count */
	if (sch->block>=MAXBLOCKS)
	{
		output_error("schedule_compile(SCHEDULE *sch='{name=%s, ...}') maximum number of blocks reached", sch->name);
		/* TROUBLESHOOT
		   The schedule definition has too many blocks to compile.  Consolidate your schedule and try again.
		 */
		return 0;
	}

	/* first index is always value 0 */
	for (index=1; (token=strtok(token==NULL?blockdef:NULL,";\r\n"))!=NULL; index++)
	{
		char moh[256], hod[256], dom[256], moy[256], dow[256];
		unsigned char minute_match[60], hour_match[24], day_match[31], month_match[12], weekday_match[8];
		unsigned int weekday;
		double value=1.0; /* default value is 1.0 */
		if (sscanf(token,"%s %s %s %s %s %lf",moh,hod,dom,moy,dow,&value)<5) /* value can be missing -> defaults to 1.0 */
		{
			output_error("schedule_compile(SCHEDULE *sch='{name=%s, ...}') ignored an invalid definition '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			continue;
		}
		else
		{
			sch->data[sch->block*MAXBLOCKS+index] = value;
			sch->sum[sch->block] += value;
			sch->abs[sch->block] += (value<0?-value:value);
			sch->count[sch->block]++;
		}

		/* get match tables */
		if (!schedule_matcher(moh,minute_match,60))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) minute syntax error in item '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			return 0;
		}
		if (!schedule_matcher(hod,hour_match,24))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) hour syntax error in item '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			return 0;
		}
		if (!schedule_matcher(dom,day_match,31))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) day syntax error in item '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			return 0;
		}
		if (!schedule_matcher(moy,month_match,12))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) month syntax error in item '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			return 0;
		}
		if (!schedule_matcher(dow,weekday_match,8))
		{
			output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) weekday syntax error in item '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
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
				unsigned int days[] = {31,(is_leapyear?29:28),31,30,31,30,31,31,30,31,30,31};
				minute = 0;
				for (month=0; month<12; month++)
				{
					unsigned int day;
					if (!month_match[month])
					{
						minute+=60*24*days[month];
						continue;
					}
					for (day=0; day<days[month]; day++)
					{
						unsigned int hour;
						if (!day_match[day])
						{
							minute+=60*24;
							continue;
						}
						for (hour=0; hour<24; hour++)
						{
							unsigned int stop = minute+60;
							if (!hour_match[hour])
							{
								minute = stop;
								continue;
							}
							while (minute<stop)
							{
								if (minute_match[minute%60])
								{
									if (sch->index[calendar][minute]>0)
									{
										char *dayofweek[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat","Sun","Hol"};
										output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) %s has a conflict with value %g on %s %d/%d %02d:%02d", sch->name, token, sch->data[sch->index[calendar][minute]], dayofweek[weekday], month+1, day+1, hour, minute%60);
										/* TROUBLESHOOT
										   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
										 */
										return 0;
									}
									else
										sch->index[calendar][minute] = sch->block*MAXBLOCKS + index;
								}
								minute++;
							}
						}
					}
				}
			}
		}
	}
	sch->block++;
	return 1;
}

/* compiles a multi-block schedule and report errors
   returns 1 on success, 0 on failure 
 */
int schedule_compile(SCHEDULE *sch)
{
	char *p = sch->definition, *q = NULL;
	char blockdef[65536];
	char blockname[64];
	enum {INIT, NAME, OPEN, BLOCK, CLOSE} state = INIT;
	int comment=0;
	
	/* check to see no blocks are defined */
	if (strchr(p,'{')==NULL && strchr(p,'}')==NULL)
	{
		/* this is single block unnamed schedule */
		strcpy(blockdef,p);
		return schedule_compile_block(sch,blockdef);
	}

	/* isolate each block */
	while (*p!='\0')
	{
		/* handle comments */
		if (*p=='#') 
		{
			comment=1;
			p++;
			continue;
		}
		else if (comment)
		{
			if (*p=='\n')
				comment=0;
			p++;
			continue;
		}

		switch (state) {
		case INIT:
		case CLOSE:
			if (!isspace(*p) && !iscntrl(*p)) 
			{
				if (sch->block>=MAXBLOCKS)
				{
					output_error("maximum number of allowed schedule blocks exceeded");
					/* TROUBLESHOOT
						
					 */
					return 0;
				}
				state = NAME;
				q = blockname;
				/* do not accept character yet */
			}
			else /* space/control */
				p++;
			break;
		case NAME:
			if (isspace(*p) || iscntrl(*p)) 
			{
				state = OPEN;
				p++;
			}
			else /* valid text */
			{
				if (q<blockname+sizeof(blockname)-1)
				{
					*q++ = *p++;
					*q = '\0';
				}
				else
				{
					output_error("schedule name is too long");
					/* TROUBLESHOOT
						
					 */
					return 0;
				}
			}
			break;
		case OPEN:
			if (*p=='{') /* open block */
			{
				state = BLOCK;
				q = blockdef;
				p++;
			}
			else if (!isspace(*p) && !iscntrl(*p)) /* non-white/control */
			{
				output_error("unexpected text before block start");
				/* TROUBLESHOOT
					
				 */
				return 0;
			}
			else /* space/control */
				p++;
			break;
		case BLOCK:
			if (*p=='}')
			{	/* end block */
				state = CLOSE;
				q = NULL;
				p++;
				if (schedule_compile_block(sch,blockdef))
				{
					strcpy(sch->blockname[sch->block],blockname);
					sch->block++;
				}
				else
					return 0;
			}
			else 
			{
				if (q<blockdef+sizeof(blockdef)-1)
				{
					*q++ = *p++;
					*q = '\0';
				}
				else
				{
					output_error("schedule name is too long");
					/* TROUBLESHOOT
						
					 */
					return 0;
				}
			}
			break;
		default:
			break;
		}
	}
	return 1;
}

/** Create a schedule. 
	If the schedule has already been defined, the existing structure is returned, otherwise a new one is created. 
	If the definition is not provided, then the named schedule is searched and NULL is returned if it is not found.
	
	Example:
	<code>schedule_create("weekdays 8am-5pm 100%, weekends 9-noon 50%","* 8-17 * * 1-5; * 9-12 * * 0,6 0.5");</code>
	
	@return a pointer to the new schedule, NULL if failed
 **/
SCHEDULE *schedule_create(char *name,		/**< the name of the schedule */
						  char *definition)	/**< the definition of the schedule (using crontab format with semicolon delimiters), NULL is only a search */
{
	/* find the schedule is already defined (by name) */
	SCHEDULE *sch = schedule_find_byname(name);
	if (sch!=NULL) 
	{
		if (definition!=NULL && strcmp(sch->definition,definition)!=0)
		{
			output_error("schedule_create(char *name='%s', char *definition='%s') definition does not match previous definition of schedule '%s')", name, definition, name);
			/* TROUBLESHOOT
				There is more than 1 schedule with a given name, but they have different definitions.  Under certain circumstances, this can 
				lead to unpredictable simulation results and should be remedied by using a distinct name for each distinct schedule.
			 */
		}
		return sch;
	}

	/* create without a definition is simply a search */
	else if (definition==NULL)
	{
		return NULL;
	}

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

	/* clear arrays */
	sch->block = 0;
	memset(sch->blockname,0,sizeof(sch->blockname));
	memset(sch->data,0,sizeof(sch->data));
	memset(sch->dtnext,0,sizeof(sch->dtnext));
	memset(sch->index,0,sizeof(sch->index));
	memset(sch->sum,0,sizeof(sch->sum));
	memset(sch->abs,0,sizeof(sch->abs));
	memset(sch->count,0,sizeof(sch->count));
	sch->next_t = TS_NEVER;
	sch->value = 0.0;
	sch->flags = 0;
	sch->duration = 0.0;
	sch->next = NULL;

	/* compile the schedule */
	if (schedule_compile(sch))
	{
		/* construct the dtnext array */
		unsigned char calendar;
		int invariant = 1;
		for (calendar=0; calendar<14; calendar++)
		{
			/* number of minutes that are indexed */
			int t = sizeof(sch->dtnext[calendar])/sizeof(sch->dtnext[calendar][0])-1;

			/* assume that loopback results in a value change in 1 minute */
			sch->dtnext[calendar][t] = 1; 

			/* scan backwards through time */
			for (t--; t>=0; t--)
			{
				/* get this and the next index to values */
				int index0 = sch->index[calendar][t];
				int index1 = sch->index[calendar][t+1];

				/* if the values are the same */
				if (sch->data[index0]==sch->data[index1])

					/* add 1 minute to next values time */
					sch->dtnext[calendar][t] = sch->dtnext[calendar][t+1] + 1;
				else
				{
					/* start the time over at 1 minute (to next value) */
					sch->dtnext[calendar][t] = 1;
					invariant = 0;
				}
			}
		}

		/* special case for invariant schedule */
		if (invariant)
			memset(sch->dtnext,0,sizeof(sch->dtnext)); /* zero means never */

		/* attach to schedule list */
		sch->next = schedule_list;
		schedule_list = sch;
		return sch;
	}
	else
	{
		/* error message should be given by schedule_compile */
		free(sch);
		return NULL;
	}
}

/** normalizes a schedule, if possible
	@note the sum of the values is equal to 1.0, not the sum of the absolute values
	@return number of block that could be normalized, 0 if none, -1 if already normalized
 **/
int schedule_normalize(SCHEDULE *sch,	/**< the schedule to normalize */
					   int use_abs)		/**< true if normalization should use absolute values */
{
	unsigned int b,i;
	int count=0;

	/* check if already normalized */
	if (sch->flags&SF_NORMALIZED)
		return -1;

	/* normalized */
	for (b=0; b<MAXBLOCKS; b++)
	{
		double scale = (use_abs?sch->abs[b]:sch->sum[b]);
		if (scale!=0)
		{
			count++;
			for (i=0; i<MAXVALUES; i++)
				sch->data[b*MAXBLOCKS+i]/=scale;
		}
	}

	/* mark as normalized */
	sch->flags |= SF_NORMALIZED;

	return count;
}

/** get the index value for the given timestamp 
    @return negative on error, 0 or positive on success
 **/
SCHEDULEINDEX schedule_index(SCHEDULE *sch, TIMESTAMP ts)
{
	SCHEDULEINDEX ref = {0};
	DATETIME dt;
	
	/* determine the local time */
	if (!local_datetime(ts,&dt))
	{
		throw_exception("schedule_read(SCHEDULE *schedule={name='%s',...}, TIMESTAMP ts=%"FMT_INT64"d) unable to determine local time", sch->name, ts);
		/* TROUBLESHOOT
			The schedule could not be read because the local time could not be determined.  
			Fix the problem causing the local time system failure and try again.
		 */
	}

	/* determine which calendar is used based on the weekday of Jan 1 and LY status */
	ref.calendar = dt.weekday + ISLEAPYEAR(dt.year);

	/* compute the minute of year */
	ref.minute = (dt.yearday*24 + dt.hour)*60 + dt.minute;

	/* got it */
	return ref;
}

/** reads the value on the schedule
    @return current value on schedule
 **/
double schedule_value(SCHEDULE *sch, /**< the schedule to read */
					  SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	return sch->data[sch->index[index.calendar][index.minute]];
}

/** reads the time until the next change in the schedule 
	@return time until next value change (in minutes)
 **/
long schedule_dtnext(SCHEDULE *sch, /**< the schedule to read */
					  SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	return sch->dtnext[index.calendar][index.minute];
}

/** synchronize the schedule to the time given
    @return the time of the next schedule change
 **/
TIMESTAMP schedule_sync(SCHEDULE *sch, /**< the schedule that is to be synchronized */
						TIMESTAMP t)	/**< the time to which the schedule is to be synchronized */
{
	static TIMESTAMP last_t = 0;
	static SCHEDULEINDEX index = {0};
	double value;
	long dtnext;
	
	/* get the current schedule status */
	if (t!=last_t) index = schedule_index(sch,t);
	value = schedule_value(sch,index);
	dtnext = schedule_dtnext(sch,index)*60;

	/* if the schedule is changing value */
	if (value!=sch->value)
	{
		/* record the next value and its duration */
		sch->value = value;
		sch->duration = (dtnext==0 ? (double)TS_NEVER : dtnext);
	}

	/* compute the time of the next schedule change */
	sch->next_t = (dtnext==0 ? TS_NEVER : t + dtnext);
	return sch->next_t;
}

/** synchronized all the schedules to the time given
    @return the time of the next schedule change
 **/
TIMESTAMP schedule_syncall(TIMESTAMP t1) /**< the time to which the schedule is synchronized */
{
	SCHEDULE *sch;
	TIMESTAMP t2 = TS_NEVER;
	for (sch=schedule_list; sch!=NULL; sch=sch->next)
	{
		TIMESTAMP t3 = schedule_sync(sch,t1);
		if (t3<t2) t2 = t3;
	}
	return t2;
}