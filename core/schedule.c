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
#include <pthread.h>

#include "platform.h"
#include "object.h"
#include "output.h"
#include "schedule.h"
#include "transform.h"
#include "exception.h"
#include "lock.h"

static SCHEDULE *schedule_list = NULL;
static uint32 n_schedules = 0;
static int interpolated_schedules = FALSE;

#ifdef _DEBUG
unsigned int schedule_checksum(SCHEDULE *sch)
{
	unsigned int sum = 0;
	unsigned int *ptr = &(sch->magic1) + 1;
	while ( ptr < &(sch->magic2) )
		sum ^= *ptr++;
	return sum;
}
#endif

/** Iterate through the schedule list
	@return the next schedule pointer (or first schedule)
 **/
SCHEDULE *schedule_getnext(SCHEDULE *sch) /**< the schedule (or NULL to get first) */
{
	return sch==NULL ? schedule_list : sch->next;
}

/** Find a schedule by its name 
	@return the schedule pointer
 **/
SCHEDULE *schedule_find_byname(char *name) /**< the name of the schedule */
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
int schedule_matcher(char *pattern, unsigned char *table, int max, int base)
{
	int go=0;
	int start=0;
	int stop=-1;
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
			start=base; stop=max; go=1;
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
		if (go && stop>=0)
		{	int i;

			/* check under limit */
			if (start<base)
			{
				output_warning("schedule_matcher(char *pattern='%s',...) start before min of %d", pattern,base);
				start = base;
			}

			/* check over limit */
			if (stop>max)
			{
				output_warning("schedule_matcher(char *pattern='%s',...) end exceed max of %d", pattern,max);
				stop = max;
			}

			/* go fill */
			if (start>stop) /* wraparound */
			{
				for (i=start; i<=max; i++)
					table[i-base] = 1;
				for (i=base; i<=stop; i++)
					table[i-base] = 1;
			}
			else
			{
				for (i=start; i<=stop; i++)
					table[i-base] = 1;
			}

			/* reset */
			start = range = go = 0;
			stop = -1;
		}
		if (*p=='\0')
			break;
	}

	return 1;
}

/// find_value_index -- search for a value in a schedule index
/// @return the index number
int find_value_index (SCHEDULE *sch, /// schedule to search
					  unsigned char block, /// block to search
					  double value) /// value to find
{
	int ndx;
	for (ndx=0; ndx<(int)(sch->count[block]); ndx++)
	{
		if ((float)(sch->data[block*MAXVALUES+ndx]) == (float)value)
			return ndx;
	}
	return -1;
}

/* compiles a single schedule block and report errors
   returns 1 on success, 0 on failure 
 */

int schedule_compile_block(SCHEDULE *sch, char *blockname, char *blockdef)
{
	char *token = NULL;
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

	/* first index is always default value 0 */
	sch->count[sch->block]=1;
	while ( (token=strtok(token==NULL?blockdef:NULL,";\r\n"))!=NULL )
	{
		struct {
			char *name;
			int base;
			int max;
			char pattern[256];
			char table[60];
		} matcher[] = {
			{"minute",0,59},
			{"hour",0,23},
			{"day",1,31},
			{"month",1,12},
			{"weekday",0,8},
		}, *match;
		unsigned int calendar;
		double value=1.0; /* default value is 1.0 */
		int ndx;

		/* remove leading whitespace */
		while (isspace(*token)) token++;
		if (strcmp(token,"")==0)
			continue;
		if ( strcmp(token,"nonzero")==0 )
			sch->flags |= SN_NONZERO;
		else if ( strcmp(token,"positive")==0 )
			sch->flags |= SN_POSITIVE;
		else if ( strcmp(token,"boolean")==0 )
			sch->flags |= SN_BOOLEAN;
		else if ( strcmp(token,"normal")==0 )
			sch->flags |= SN_NORMAL;
		else if ( strcmp(token,"weighted")==0 )
			sch->flags |= SN_NORMAL|SN_WEIGHTED;
		else if ( strcmp(token,"absolute")==0 )
			sch->flags |= SN_NORMAL|SN_ABSOLUTE;
		else if ( strcmp(token,"interpolated")==0 )
		{
			sch->flags |= SN_INTERPOLATED;
			interpolated_schedules = TRUE;
		}
		else if (sscanf(token,"%s%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%s%*[ \t]%lf",matcher[0].pattern,matcher[1].pattern,matcher[2].pattern,matcher[3].pattern,matcher[4].pattern,&value)<5) /* value can be missing -> defaults to 1.0 */
		{
			output_error("schedule_compile(SCHEDULE *sch='{name=%s, ...}') ignored an invalid definition '%s'", sch->name, token);
			/* TROUBLESHOOT
			   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
			 */
			continue;
		}
		else
		{
			if ((ndx=find_value_index(sch,sch->block,value))==-1)
			{	
				ndx = sch->count[sch->block]++;
				// bound checking
				if(ndx > MAXVALUES-1)
				{
					output_error("schedule_compile(SCHEDULE *sch='{name=%s, ...}') maximum number of values reached in block %i", sch->name, sch->block);
					return 0;
				}
				sch->data[sch->block*MAXVALUES+ndx] = value;
			}
			sch->sum[sch->block] += value;
			sch->abs[sch->block] += (value<0?-value:value); // check to see if the value already exists in the value array, if so, don't ++index and use existing indexed value
		}

		/* compile matching tables */
		for (match=matcher; match<matcher+sizeof(matcher)/sizeof(matcher[0]); match++)
		{
			/* get match tables */
			if (!schedule_matcher(match->pattern,match->table,match->max,match->base))
			{
				output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) %s pattern syntax error in item '%s'", sch->name, match->name, token);
				/* TROUBLESHOOT
					The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
				*/
				return 0;
			}
		}

		/* load schedule based on weekday of Jan 1 */
		for (calendar=0; calendar<14; calendar++) // don't do holidays (requires a special case that's not supported yet)
		{
			unsigned int weekday = calendar%7;
			unsigned int is_leapyear = (calendar>=7?1:0);
			unsigned int calendar = weekday*2+is_leapyear;
			unsigned int month;
			unsigned int days[] = {31,(is_leapyear?29:28),31,30,31,30,31,31,30,31,30,31};
			unsigned int n = sch->block*MAXVALUES + ndx;
			minute = 0;
			for (month=0; month<12; month++)
			{
				unsigned int day;
				if (!matcher[3].table[month])
				{
					minute+=60*24*days[month];
					weekday+=days[month];
					continue;
				}
				for (day=0; day<days[month]; weekday++,day++)
				{
					unsigned int hour;
					weekday%=7; /* wrap day of week */
					if (!matcher[4].table[weekday] || !matcher[2].table[day])
					{
						minute+=60*24;
						continue;
					}
					for (hour=0; hour<24; hour++)
					{
						unsigned int stop = minute+60;
						if (!matcher[1].table[hour])
						{
							minute = stop;
							continue;
						}
						while (minute<stop)
						{
							if (matcher[0].table[minute%60])
							{
								if (sch->index[calendar][minute]>0)
								{
									char *dayofweek[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat","Sun","Hol"};
									output_error("schedule_compile(SCHEDULE *sch={name='%s', ...}) '%s' in block '%s' has a conflict with value %g on %s %d/%d %02d:%02d", sch->name, token, blockname, sch->data[sch->index[calendar][minute]], dayofweek[weekday], month+1, day+1, hour, minute%60);
									/* TROUBLESHOOT
									   The schedule definition is not valid and has been ignored.  Check the syntax of your schedule and try again.
									 */
									return 0;
								}
								else
								{
									/* associate this time with the current value */
									sch->index[calendar][minute] = n;
									sch->weight[n]++;
									sch->minutes[sch->block]++;

								}
							}
							minute++;
						}
					}
				}
			}
		}
	}
	strcpy(sch->blockname[sch->block],blockname);
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
		/* remove leading whitespace */
		while (isspace(*p)) p++;
		strcpy(blockdef,p);
		if (schedule_compile_block(sch,"*",blockdef))
		{
			sch->block++;
			return 1;
		}
		else
			return 0;
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
						Up to 4 schedule blocks are allowed.  Define your schedule so it only uses four blocks and try again.
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
			else if (*p=='{' || *p==';')
			{
				state = OPEN;
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
						The name given the schedule is too long to be used.  Use a name that is less than 64 characters and try again.
					 */
					return 0;
				}
			}
			break;
		case OPEN:
			if (*p==';') /* option */
			{
				if (strcmp(blockname,"weighted")==0)
					sch->flags |= SN_WEIGHTED;
				else if (strcmp(blockname,"absolute")==0)
					sch->flags |= SN_ABSOLUTE;
				else if (strcmp(blockname,"normal")==0)
					sch->flags |= SN_NORMAL;
				else if (strcmp(blockname,"positive")==0)
					sch->flags |= SN_POSITIVE;
				else if (strcmp(blockname,"nonzero")==0)
					sch->flags |= SN_NONZERO;
				else if (strcmp(blockname,"boolean")==0)
					sch->flags |= SN_BOOLEAN;
				else if (strcmp(blockname,"interpolated")==0)
				{
					sch->flags |= SN_INTERPOLATED;
					interpolated_schedules = TRUE;
				}
				else
					output_error("schedule %s: block option '%s' is not recognized", sch->name, blockname);
				state = CLOSE;
				p++;
			}
			else if (*p=='{') /* open block */
			{
				state = BLOCK;
				q = blockdef;
				p++;
			}
			else if (!isspace(*p) && !iscntrl(*p)) /* non-white/control */
			{
				output_error("schedule %s: unexpected text before block start", sch->name);
				/* TROUBLESHOOT
					The schedule syntax is not valid.  Remove the unexpected or invalid text before the block and try again.
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
				if (schedule_compile_block(sch,blockname,blockdef))
					sch->block++;
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
						The definition given the schedule is too long to be used.  Use a definition that is less than 1024 characters and try again.
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

static pthread_cond_t sc_active = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t sc_activelock = PTHREAD_MUTEX_INITIALIZER;
static STATUS sc_status = SUCCESS;
static sc_running=0, sc_started=0, sc_done=0;

void *schedule_createproc(void *args)
{
	STATUS status = SUCCESS;
	void *rv = 0;
	SCHEDULE *sch = (SCHEDULE *)args;

	pthread_mutex_lock(&sc_activelock);
	while ( sc_running>=global_threadcount )
	{
		output_debug("schedule '%s' creation waiting (%d of %d active)", sch->name, sc_running, global_threadcount); 
		pthread_cond_wait(&sc_active,&sc_activelock);
	}
	sc_running++;
	output_debug("deferred schedule '%s' creation starting (%d of %d active)", sch->name, sc_running, global_threadcount); 
	pthread_cond_broadcast(&sc_active);
	pthread_mutex_unlock(&sc_activelock);

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
				{	
					/* if we haven't reached the maximum delta-t (for unsigned char dtnext) */
					if (sch->dtnext[calendar][t+1]<255)

						/* add 1 minute to next values time */
						sch->dtnext[calendar][t] = sch->dtnext[calendar][t+1] + 1;
					else

						/* start the time over at 1 minute (to next value) */
						sch->dtnext[calendar][t] = 1;
				}
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

		/* check for gaps in the schedule */
		else
		{
			int ngaps = 0;
			int ingap = 0;
			for (calendar=0; calendar<14; calendar++)
			{
				int t;
				for ( t=0; t<sizeof(sch->dtnext[calendar])/sizeof(sch->dtnext[calendar][0])-1; t++ )
				{
					if ( sch->dtnext[calendar][t] == 0 && !ingap)
					{
						int day = t/60/24;
						int hour = t/60 - day*24;
						int minute = t - hour*60 - day*24*60;
						output_debug("schedule '%s' gap in calendar %d at day %d, hour %d, minute %d lasting %d minutes", sch->name, day, hour, minute);
						ingap = 1;
						ngaps++;
					}
					else if ( sch->dtnext[calendar][t] != 0 && ingap )
						ingap = 0;
				}
			}
			if (ngaps>0)
				output_error("schedule '%s' has %d gaps which may cause erroneous results", sch->name, ngaps);
				/* TROUBLESHOOT
					The definition given the schedule has missing data that will cause time synchronization problems.
					Make sure that all the time covered by the schedule has values given.
				 */
		}

		/* normalize */
		if (sch->flags!=0)
			schedule_normalize(sch,sch->flags);

		/* validate */
		if ((sch->flags&(SN_POSITIVE|SN_NONZERO|SN_BOOLEAN)) != 0 && ! schedule_validate(sch,sch->flags))
		{
			status = FAILED;
			goto Done;
		}

#ifdef _DEBUG
		/* calculate checksum */
		sch->checksum = schedule_checksum(sch);
		output_debug("schedule '%s' checksum is %0x08d", sch->name, sch->checksum);
#endif
		status = SUCCESS;
	}
	else
		status = FAILED;
Done:
	pthread_mutex_lock(&sc_activelock);
	sc_running--;
	sc_done++;
	if ( status==FAILED ) sc_status=status;
	pthread_cond_broadcast(&sc_active);
	pthread_mutex_unlock(&sc_activelock);
	if ( status==SUCCESS )
	{
		output_debug("deferred creation of schedule '%s' completed", sch->name);
	}
	else
	{
		output_error("deferred creation of schedule '%s' failed", sch->name);
	}
	rv = (void *)status;
	return rv;
}

/** Wait for deferred schedule creations to finish 
    @return the global status of schedule creation
 **/
int schedule_createwait(void)
{
	if ( sc_running==0 && sc_done==sc_started ) return sc_status;
	pthread_mutex_lock(&sc_activelock);
	while ( sc_running>0 || sc_done<sc_started )
	{
		output_debug("waiting for deferred schedule creations to complete (%d of %d active)", sc_running, global_threadcount);
		pthread_cond_wait(&sc_active,&sc_activelock);
	}
	output_debug("all deferred schedule creations completed %s", sc_status==SUCCESS ? "successfully" : "with at least one failure");
	pthread_mutex_unlock(&sc_activelock);
	return sc_status;
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
	STATUS result;
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

	/* create a new schedule */
	sch = schedule_new();
	if (sch==NULL)
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') memory allocation failed)", name, definition);
		/* TROUBLESHOOT
			The schedule module could not allocate enough memory to create a schedule item.  Try freeing system memory and try again.
		 */
		return NULL;
	}
	output_debug("schedule '%s' uses %.1f MB of memory", name, sizeof(SCHEDULE)/1000000.0);
	if (strlen(name)>=sizeof(sch->name))
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') name too long)", name, definition);
		/* TROUBLESHOOT
			The name given the schedule is too long to be used.  Use a name that is less than 64 characters and try again.
		 */
		free(sch);
		return NULL;
	}
	strcpy(sch->name,name);
	if (strlen(definition)>=sizeof(sch->definition))
	{
		output_error("schedule_create(char *name='%s', char *definition='%s') definition too long)", name, definition);
		/* TROUBLESHOOT
			The definition given the schedule is too long to be used.  Use a definition that is less than 1024 characters and try again.
		 */
		free(sch);
		return NULL;
	}
	strcpy(sch->definition,definition);

	/* attach to schedule list */
	schedule_add(sch);

	/* singlethreaded creation */
	if ( global_threadcount<=1 )
	{
		result = (STATUS)schedule_createproc(sch);
		if (SUCCESS == result)
		{
			return sch;
		}
		else
		{
			/* error message should be given by schedule_compile */
			free(sch);
			sch = NULL;
			return NULL;
		}
	}

	/* multithreaded creation */
	else
	{
		static unsigned int n_threads = 0;
		static pthread_t thread_id;
		if ( pthread_create(&thread_id,NULL,schedule_createproc,(void*)sch)!=0 )
		{
			/* fails so do it inline */
			output_warning("schedule_createproc failed, schedule '%s' created inline instead", sch->name);
			return schedule_createproc(sch) ? sch : NULL;
		}
		sc_started++;
		return sch;
	}
}

SCHEDULE *schedule_new(void)
{
	/* create the schedule */
	SCHEDULE *sch = (SCHEDULE*)malloc(sizeof(SCHEDULE));
	if (sch==NULL) return NULL;

	/* initialize */
	memset(sch,0,sizeof(SCHEDULE));
#ifdef _DEBUG
	sch->magic1 = sch->magic2 = SCHEDULE_MAGIC; 
#endif
	sch->since = TS_ZERO;
	sch->next_t = TS_NEVER;
	return sch;
}
void schedule_add(SCHEDULE *sch)
{
	sch->next = schedule_list;
	schedule_list = sch;
	n_schedules++;
}

/** validate a schedule, if desired 
	See SN_* flags for validation options
 **/
int schedule_validate(SCHEDULE *sch, int flags)
{
	unsigned int b,i;
	uint32 nzct = 0; // nonzero count
	int failed=0;
	for (b=0; b<MAXBLOCKS; b++) {
		i = (flags & SN_NONZERO ? 0 : 1);
		for (; i<=sch->count[b]; i++)
		{
			double value = sch->data[b*MAXVALUES+i];
			int weight = sch->weight[b*MAXVALUES+i];
			int nonzero = (weight>0 && value!=0.0);
			int boolean = (weight>0 && (value==0.0 || value==1.0));
			int positive = (weight>0 && value>0.0);
			if ( weight==0 ) continue;
			if(value != 0.0)
				nzct += weight;
			if ((flags&SN_BOOLEAN) && !boolean)
			{
				output_error("schedule %s fails 'boolean' validation in block %s at schedule index %d", sch->name, sch->blockname[b], i);
				failed = 1;
			}
			else if ((flags&SN_POSITIVE) && !positive)
			{
				output_error("schedule %s fails 'positive' validation in block %s at schedule index %d", sch->name, sch->blockname[b], i);
				failed = 1;
			}
			else if ((flags&SN_NONZERO) && !nonzero)
			{
				output_error("schedule %s fails 'nonzero' validation in block %s at schedule index %d", sch->name, sch->blockname[b], i);
				failed = 1;
			}
		}
	}
	if((flags & SN_NONZERO) && (nzct != (7 * (365+366) * 24 * 60))){
		output_error("schedule %s fails 'nonzero' validation with unfilled entries", sch->name);
		failed = 1;
	}
	return !failed;
}

/** normalizes a schedule, if possible
	@note the sum of the values is equal to 1.0, not the sum of the absolute values
	@return number of block that could be normalized, 0 if none, -1 if already normalized
 **/
int schedule_normalize(SCHEDULE *sch,	/**< the schedule to normalize */
					   int flags)		/**< schedule normalization flag (see #SN_WEIGHTED and #SN_ABSOLUTE) */
{
	unsigned int b,i;
	int count=0;

	/* check if already normalized */
	if (sch->flags&flags)
		return -1;

	/* normalized */
	for (b=0; b<MAXBLOCKS; b++)
	{
		/* ignore empty blocks */
		if (sch->count[b]==0)
			continue;

		/* weighted normalization */
		if (flags&SN_WEIGHTED)
		{	
			double scale[MAXVALUES];
			unsigned int i;
			int nonzero = 0;
			memset(scale,0,sizeof(scale));
			for (i=1; i<=sch->count[b]; i++)
			{
				if (sch->weight[i]!=0)
				{
					nonzero = 1;
					scale[i] += sch->data[b*MAXVALUES+i] * sch->weight[i] / sch->minutes[b];
				}
			}
			if (nonzero)
			{
				for (i=1; i<=sch->count[b]; i++)
					sch->data[b*MAXVALUES+i]*=scale[i];
			}
		}

		/* unweighted normalization */
		else
		{
			double scale = (flags&SN_ABSOLUTE?sch->abs[b]:sch->sum[b]);

			/* if the coefficient is non-zero */
			if (scale!=0)
			{
				/* normalize the values */
				count++;
				for (i=1; i<=sch->count[b]; i++)
					sch->data[b*MAXVALUES+i]/=scale;
			}
		}
	}

	/* mark as normalized */
	sch->flags |= flags;

	return count;
}

/** get the index value for the given timestamp 
    @return negative on error, 0 or positive on success
 **/
SCHEDULEINDEX schedule_index(SCHEDULE *sch, TIMESTAMP ts)
{
	SCHEDULEINDEX ref = 0;
	DATETIME dt;
	unsigned int cal, min;
	
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
	cal = ((dt.weekday-dt.yearday+53*7)%7)*2 + ISLEAPYEAR(dt.year);

	/* compute the minute of year */
	min=(dt.yearday*24 + dt.hour)*60 + dt.minute;

	if ( cal>=14 || min>=60*24*366 )
		output_error("schedule_index(): timestamp %" FMT_INT64 "d has calendar %d minute %d which is invalid", ts, cal, min);

	SET_CALENDAR(ref, cal);
	SET_MINUTE(ref, min);

	/* got it */
	return ref;
}

/** reads the value on the schedule
    @return current value on schedule
 **/
double schedule_value(SCHEDULE *sch,		/**< the schedule to read */
					  SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	int32 cal = GET_CALENDAR(index);
	int32 min = GET_MINUTE(index);
	if ( cal>=14 || min>=60*24*366 )
		output_error("schedule_index(): index %d has calendar %d minute %d which is invalid", index, cal, min);
	return sch->data[sch->index[cal][min]];
}

/** reads the time until the next change in the schedule 
	@return time until next value change (in minutes)
 **/
int32 schedule_dtnext(SCHEDULE *sch,			/**< the schedule to read */
					 SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	int32 cal = GET_CALENDAR(index);
	int32 min = GET_MINUTE(index);
	if ( cal>=14 || min>=60*24*366 )
		output_error("schedule_dtnext(): index %d has calendar %d minute %d which is invalid", index, cal, min);
	return sch->dtnext[cal][min];
}

int32 schedule_duration(SCHEDULE *sch,			/**< the schedule to read */
					   SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	int32 cal = GET_CALENDAR(index);
	int32 min = GET_MINUTE(index);
	int block;
	if ( cal>=14 || min>=60*24*366 )
		output_error("schedule_duration(): index %d has calendar %d minute %d which is invalid", index, cal, min);
	block = (sch->index[cal][min]>>6)&MAXBLOCKS; // these change if MAXVALUES or MAXBLOCKS changes
	return sch->minutes[block];
}

double schedule_weight(SCHEDULE *sch,			/**< the schedule to read */
					   SCHEDULEINDEX index)	/**< the index of the value to read (see schedule_index) */
{
	int32 cal = GET_CALENDAR(index);
	int32 min = GET_MINUTE(index);
	if ( cal>=14 || min>=60*24*366 )
		output_error("schedule_weight(): index %d has calendar %d minute %d which is invalid", index, cal, min);
	return sch->weight[sch->index[cal][min]];
}

/** synchronize the schedule to the time given
    @return the time of the next schedule change
 **/
TIMESTAMP schedule_sync(SCHEDULE *sch, /**< the schedule that is to be synchronized */
						TIMESTAMP t)	/**< the time to which the schedule is to be synchronized */
{
#ifdef _DEBUG
	if ( sch->magic1!=SCHEDULE_MAGIC || sch->magic2!=SCHEDULE_MAGIC ) // || sch->checksum!=schedule_checksum(sch) )
		output_warning("schedule '%s' may be corrupted", sch->name);
#endif

	if ((sch->flags & SN_INTERPOLATED) == SN_INTERPOLATED)
	{
		/* 
		 * In interpolation mode we call for the next sync when the schedule changes, but if any other
		 * sync operations occur (caused by other objects) we re-interpolate.
		 */

		if (sch->since == TS_ZERO || t >= sch->next_t)
		{
			/* first iteration */
			SCHEDULEINDEX index = schedule_index(sch,t);
			int32 dtnext = schedule_dtnext(sch,index)*60;
			sch->since = sch->since == TS_ZERO ? t : sch->next_t;
			sch->duration = schedule_duration(sch,index)/60.0;
			sch->next_t = (dtnext==0 ? TS_NEVER : t + dtnext -  t % 60);
			if (sch->next_t == TS_NEVER)
			{
				/* no next schedule will me we don't interpolate so we had better get the current value. */
				sch->value = schedule_value(sch,index);
			}
		}

		if (sch->next_t != TS_NEVER)
		{
			/* still in the same window, just re-interpolate. */
			SCHEDULEINDEX start_index = schedule_index(sch,sch->since);
			SCHEDULEINDEX end_index = schedule_index(sch,sch->next_t);
			double start_value = schedule_value(sch,start_index);
			double end_value = schedule_value(sch,end_index);
			sch->value = start_value + ((end_value - start_value) * ((double)(t - sch->since) / (double)(sch->next_t - sch->since))); 
		}
	}
	else
	{
		/* determine whether the current schedule is still valid */
		if ( sch->next_t==TS_NEVER || t >= sch->next_t )
		{
			/* move to the new schedule */
			SCHEDULEINDEX index = schedule_index(sch,t);
			int32 dtnext = schedule_dtnext(sch,index)*60;
			double value = schedule_value(sch,index);
#ifdef _DEBUG
			if ( dtnext==0 )
				output_debug("schedule_sync(SCHEDULE *sch={name: '%s',...}, TIMESTAMP t=%"FMT_INT64"d) has a dtnext==0", sch->name, t);
#endif
			if(sch->value != value){
				sch->since = t;
			}
			sch->value = value;
			sch->duration = schedule_duration(sch,index)/60.0;
			sch->next_t = (dtnext==0 ? TS_NEVER : t + dtnext -  t % 60);
#ifdef _DEBUG
			output_test("time %"FMT_INT64"d: schedule '%s', value %g, duration %g, dt_next %d, next_t %"FMT_INT64"d",
				t, sch->name, sch->value, sch->duration, dtnext, sch->next_t);
#endif
		}
	}

	/* compute the time of the next schedule change */
	return sch->next_t;
}

typedef struct s_schedulesyncdata {
	unsigned int n;
	pthread_t pt;
	bool ok;
	SCHEDULE *sch;
	unsigned int nsch;
	TIMESTAMP t0;
} SCHEDULESYNCDATA;

static pthread_cond_t start_sch = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t startlock_sch = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t done_sch = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t donelock_sch = PTHREAD_MUTEX_INITIALIZER;
static TIMESTAMP next_t1_sch;
static TIMESTAMP next_t2_sch = TS_ZERO;
static unsigned int donecount_sch;

clock_t schedule_synctime = 0;

void *schedule_syncproc(void *ptr)
{
	SCHEDULESYNCDATA *data = (SCHEDULESYNCDATA*)ptr;
	SCHEDULE *sch;
	unsigned int n;
	TIMESTAMP t2;

	// begin processing loop
	while ( data->ok )
	{
		// lock access to start condition
		pthread_mutex_lock(&startlock_sch);

		// wait for thread start condition
		while (data->t0==next_t1_sch) 
			pthread_cond_wait(&start_sch,&startlock_sch);
		
		// unlock access to start count
		pthread_mutex_unlock(&startlock_sch);

		// process the list for this thread
		t2 = TS_NEVER;
		for ( sch=data->sch, n=0 ; sch!=NULL, n<data->nsch ; sch=sch->next, n++ )
		{
			TIMESTAMP t = schedule_sync(sch,next_t1_sch);
			if (t<t2) t2 = t;
		}

		// signal completed condition
		data->t0 = next_t1_sch;

		// lock access to done condition
		pthread_mutex_lock(&donelock_sch);

		// signal thread is done for now
		donecount_sch--;
		if ( t2<next_t2_sch ) next_t2_sch = t2;

		// signal change in done condition
		pthread_cond_broadcast(&done_sch);

		// unlock access to done count
		pthread_mutex_unlock(&donelock_sch);
	}
	pthread_exit((void*)0);
	return (void*)0;
}

/** synchronized all the schedules to the time given
    @return the time of the next schedule change
 **/
TIMESTAMP schedule_syncall(TIMESTAMP t1) /**< the time to which the schedule is synchronized */
{
	static unsigned int n_threads_sch=0;
	static SCHEDULESYNCDATA *thread_sch = NULL;
	TIMESTAMP t2 = TS_NEVER;
	clock_t ts = exec_clock();

	// skip schedule_syncall if there's no schedule in the glm
	if (n_schedules == 0)
		return TS_NEVER;


	// number of threads desired
	if (n_threads_sch==0) 
	{
		SCHEDULE *sch;
		int n_items, schn=0;

		output_debug("loadshape_syncall setting up for %d shapes", n_schedules);

		// determine needed threads
		n_threads_sch = global_threadcount;
		if (n_threads_sch>1)
		{
			unsigned int n;
			if (n_schedules<n_threads_sch*4)
				n_threads_sch = n_schedules/4;

			// only need 1 thread if n_schedules is less than 4
			if (n_threads_sch == 0)
				n_threads_sch = 1;

			// determine shapes per thread
			n_items = n_schedules/n_threads_sch;
			n_threads_sch = n_schedules/n_items;
			if (n_threads_sch*n_items<n_schedules) // not enough slots yet
				n_threads_sch++; // add one underused threads

			output_debug("schedule_syncall is using %d of %d available threads", n_threads_sch, global_threadcount);
			output_debug("schedule_syncall is assigning %d shapes per thread", n_items);

			// allocate thread list
			thread_sch = (SCHEDULESYNCDATA*)malloc(sizeof(SCHEDULESYNCDATA)*n_threads_sch);
			memset(thread_sch,0,sizeof(SCHEDULESYNCDATA)*n_threads_sch);

			// assign starting shape for each thread
			for (sch=schedule_list; sch!=NULL; sch=sch->next)
			{
				if (thread_sch[schn].nsch==n_items)
					schn++;
				if (thread_sch[schn].nsch==0)
					thread_sch[schn].sch = sch;
				thread_sch[schn].nsch++;
			}

			// create threads
			for (n=0; n<n_threads_sch; n++)
			{
				thread_sch[n].ok = true;
				if ( pthread_create(&(thread_sch[n].pt),NULL,schedule_syncproc,&(thread_sch[n]))!=0 )
				{
					output_fatal("loadshape_sync thread creation failed");
					thread_sch[n].ok = false;
				}
				else
					thread_sch[n].n = n;
			}
		}
	}

	// don't update if no schedules ever expect to change again
	if (next_t2_sch == TS_NEVER)
		return TS_NEVER;

	// don't update if next_t2 < next_t1, but override this if there are interpolated schedules
	if (next_t2_sch > t1 && !interpolated_schedules)
		return next_t2_sch;

	// no threading required
	if (n_threads_sch<2) 
	{
		// process list directly
		SCHEDULE *sch;
		for (sch=schedule_list; sch!=NULL; sch=sch->next)
		{
			TIMESTAMP t3 = schedule_sync(sch,t1);
			if (t3<t2) t2 = t3;
		}
		next_t2_sch = t2;
	}
	else
	{
		// lock access to done count
		pthread_mutex_lock(&donelock_sch);

		// initialize wait count
		donecount_sch = n_threads_sch;

		// lock access to start condition
		pthread_mutex_lock(&startlock_sch);

		// update start condition
		next_t1_sch = t1;
		next_t2_sch = TS_NEVER;

		// signal all the threads
		pthread_cond_broadcast(&start_sch);

		// unlock access to start count
		pthread_mutex_unlock(&startlock_sch);

		// begin wait
		while (donecount_sch>0) 
			pthread_cond_wait(&done_sch,&donelock_sch);
		output_debug("passed donecount==0 condition");

		// unlock done count
		pthread_mutex_unlock(&donelock_sch);

		// process results from all threads
		if (next_t2_sch<t2) t2=next_t2_sch;
	}

	schedule_synctime += exec_clock() - ts;
	return t2;
}

int schedule_test(void)
{
	int failed = 0;
	int ok = 0;
	int errorcount = 0;
	char ts[64];

	/* tests */
	struct s_test {
		char *name, *def;
		char *t1, *t2;
		int normalize;
		double value;
	} *p, test[] = {
		/* schedule name	schedule definition							sync time				next time expected		normalize	value expected */
		{"empty",			"", 										"2000/01/01 00:00:00",	"NEVER",				0,			 0.0},
		{"halfday-binary",	"* 12-23 * * *",							"2001/02/03 01:30:00",	"2001/02/03 12:00:00",	0,			 0.0},
		{"halfday-binary",	NULL,										"2002/03/05 13:45:00",	"2002/03/06 00:00:00",	0,			 1.0},
		{"halfday-bimodal",	"* 0-11 * * * 0.25; * 12-23 * * * 0.75;",	"2003/04/07 01:15:00",	"2003/04/07 12:00:00",	0,			 0.25},
		{"halfday-bimodal",	"* 0-11 * * * 0.25; * 12-23 * * * 0.75;",	"2004/05/09 00:00:00",	"2004/05/09 12:00:00",	0,			 0.25},
		{"halfday-bimodal",	NULL,										"2005/06/11 13:20:00",	"2005/06/12 00:00:00",	0,			 0.75},
		{"halfday-bimodal",	NULL,										"2006/07/13 12:00:00",	"2006/07/14 00:00:00",	0,			 0.75},
		{"halfday-bimodal",	NULL,										"2007/08/15 00:00:00",	"2007/08/15 12:00:00",	0,			 0.25},
		{"quarterday-normal", "* 0-5 * * *; * 12-17 * * *;",			"2008/09/17 00:00:00",	"2008/09/17 06:00:00",	SN_WEIGHTED, 0.5},
		{"quarterday-normal", NULL,										"2009/10/19 06:00:00",	"2009/10/19 12:00:00",	SN_WEIGHTED, 0.0},
		{"quarterday-normal", NULL,										"2010/11/21 12:00:00",	"2010/11/21 18:00:00",	SN_WEIGHTED, 0.5},
		{"quarterday-normal", NULL,										"2011/12/23 18:00:00",	"2011/12/24 00:00:00",	SN_WEIGHTED, 0.0},
	};

	output_test("\nBEGIN: schedule tests");
	for (p=test;p<test+sizeof(test)/sizeof(test[0]);p++)
	{
		TIMESTAMP t1 = convert_to_timestamp(p->t1);
		int errors=0;
		SCHEDULE *s = schedule_create(p->name, p->def);
		output_test("Schedule %s { %s } sync to %s...", p->name, p->def?p->def:(s?s->definition:"???"), convert_from_timestamp(t1,ts,sizeof(ts))?ts:"???");
		if (s==NULL)
		{
			output_test(" ! schedule %s { %s } create failed", p->name, p->def);
			errors++;
		}
		else
		{
			TIMESTAMP t2;
			if (p->normalize) 
				schedule_normalize(s,p->normalize);
			t2 = s?schedule_sync(s,t1):TS_NEVER;
			if (s->value!=p->value)
			{
				output_test(" ! expected value %lg but found %lg",  p->value, s->value);
				errors++;
			}
			if (t2!=convert_to_timestamp(p->t2))
			{
				output_test(" ! expected next time %s but found %s", p->t2, convert_from_timestamp(t2,ts,sizeof(ts))?ts:"???");
				errors++;
			}
		}
		if (errors==0)
		{
			output_test("   test passed");
			ok++;
		}
		else
			failed++;
		errorcount+=errors;
	}

	/* report results */
	if (failed)
	{
		output_error("scheduletest: %d schedule tests failed--see test.txt for more information",failed);
		output_test("!!! %d schedule tests failed, %d errors found",failed,errorcount);
	}
	else
	{
		output_verbose("%d schedule tests completed with no errors--see test.txt for details",ok);
		output_test("scheduletest: %d schedule tests completed, %d errors found",ok,errorcount);
	}
	output_test("END: schedule tests");
	return failed;
}

void schedule_dumpall(char *file)
{
	SCHEDULE *sch;
	for (sch=schedule_list; sch!=NULL; sch=sch->next)
		schedule_dump(sch, file, sch==schedule_list?"w":"a");
}

void schedule_dump(SCHEDULE *sch, char *file, char *mode)
{
	FILE *fp = fopen(file,mode);
	int calendar;

	fprintf(fp,"schedule %s { %s }\n", sch->name, sch->definition);
	fprintf(fp,"sizeof(SCHEDULE) = %.3f MB\n", (double)sizeof(SCHEDULE)/1024/1024);
	for (calendar=0; calendar<14; calendar++)
	{
		int year=0, month, y;
		int daysinmonth[] = {31,((calendar&1)?29:28),31,30,31,30,31,31,30,31,30,31};
		char *monthname[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
		fprintf(fp,"\nYears:");
		for (y=1970; y<2039; y++)
		{
			DATETIME dt = {y,0,1,0,0,0};
			TIMESTAMP ts = mkdatetime(&dt);
			SCHEDULEINDEX ndx = schedule_index(sch,ts);
			if (GET_CALENDAR(ndx)==calendar)
			{
				fprintf(fp," %d",y);
				if (year==0) year=y;
			}
		}
		fprintf(fp," (calendar %d)\n",calendar);

		for (month=0; month<12; month++)
		{
			int day, hour;
			fprintf(fp,"     %s  ", monthname[month]);
			for (hour=0; hour<24; hour++)
			{
				fprintf(fp," %2d:00", hour);
			}
			fprintf(fp,"\n");
			for (day=0; day<daysinmonth[month]; day++)
			{
				int hour;
				char wd[] = "SMTWTFSH";
				DATETIME dt = {year,month,day,0,0,0,0,0,""};
				TIMESTAMP ts = mkdatetime(&dt);
				local_datetime(ts,&dt);
				fprintf(fp,"      %c %2d",wd[dt.weekday],day+1);
				for (hour=0; hour<24; hour++)
				{
					int minute=0;
					DATETIME dt = {year,month+1,day+1,hour,0,0};
					TIMESTAMP ts = mkdatetime(&dt);
					SCHEDULEINDEX ndx = schedule_index(sch,ts);
					unsigned int dtn = schedule_dtnext(sch,ndx);
					double value = schedule_value(sch,ndx);
					if ( dtn < 60 )
					{
						SCHEDULEINDEX ndx2 = schedule_index(sch,ts+dtn);
						double value2 = schedule_value(sch,ndx2);
						fprintf(fp,"%5g%c",schedule_value(sch,ndx),value!=value2?'~':' ');
					}
					else 
						fprintf(fp,"%5g ",schedule_value(sch,ndx));
				}
				fprintf(fp,"\n");
			}
		}
	}

	fclose(fp);
}

