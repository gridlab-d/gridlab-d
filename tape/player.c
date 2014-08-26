/** $Id: player.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file player.c
	@addtogroup player Players
	@ingroup tapes

	Tape players use the following properties
	- \p file specifies the source of the data.  The format of a file specs is
	[\p type]\p name[:\p flags]
	The default \p type is \p file
	The default \p name is the target (parent) objects \p classname-\p id
	The default \p flags is \p "r"
	- \p filetype specifies the source file extension, default is \p "txt".  Valid types are \p txt, \p odbc, and \p memory.
	- \p property is the target (parent) that is written to
	- \p loop is the number of times the tape is to be repeated

	The following is an example of a typical tape:
	\verbatim
	2000-01-01 0:00:00,-1.00-0.2j
	+1h,-1.1-0.1j
	+1h,-1.2-0.0j
	+1h,-1.3-0.1j
	+1h,-1.2-0.2j
	+1h,-1.1-0.3j
	+1h,-1.0-0.2j
	\endverbatim
	The first line specifies the starting time of the tape and the initial value.  The value must be formatted as a
	string that is readable for the type of the data that is to receive the recording.  The remaining lines may
	have either absolute timestamps or relative times (indicated by a leading + sign).  Relative times are useful
	if the \p loop parameter is used.  When a loop is performed, only lines with relative timestamps are read and all
	absolute times are ignored.
 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include "gridlabd.h"
#include "object.h"
#include "aggregate.h"

#include "tape.h"
#include "file.h"
#include "odbc.h"

CLASS *player_class = NULL;
static OBJECT *last_player = NULL;

extern TIMESTAMP delta_mode_needed;

EXPORT int create_player(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(player_class);
	if (*obj!=NULL)
	{
		struct player *my = OBJECTDATA(*obj,struct player);
		last_player = *obj;
		gl_set_parent(*obj,parent);
		strcpy(my->file,"");
		strcpy(my->filetype,"txt");
		strcpy(my->mode, "file");
		strcpy(my->property,"(undefined)");
		my->next.ts = TS_ZERO;
		strcpy(my->next.value,"");
		my->loopnum = 0;
		my->loop = 0;
		my->status = TS_INIT;
		my->target = gl_get_property(*obj,my->property,NULL);
		my->delta_track.ns = 0;
		my->delta_track.ts = TS_NEVER;
		my->delta_track.value[0] = '\0';
		return 1;
	}
	return 0;
}

static int player_open(OBJECT *obj)
{
	char32 type="file";
	char1024 fname="";
	char32 flags="r";
	struct player *my = OBJECTDATA(obj,struct player);
	TAPEFUNCS *tf = 0;

	/* if prefix is omitted (no colons found) */
//	if (sscanf(my->file,"%32[^:]:%1024[^:]:%[^:]",type,fname,flags)==1)
//	{
//		/* filename is file by default */
	strcpy(fname,my->file);
//		strcpy(type,"file");
//	}

	/* if no filename given */
	if (strcmp(fname,"")==0)

		/* use object name-id as default file name */
		sprintf(fname,"%s-%d.%s",obj->parent->oclass->name,obj->parent->id, my->filetype);

	/* if type is file or file is stdin */
	tf = get_ftable(my->mode);
	if(tf == NULL)
		return 0;
	my->ops = tf->player;
	if(my->ops == NULL)
		return 0;

	/* access the input stream to the player */
	if ( (my->ops->open)(my, fname, flags)==1 )
	{
		/* set up the delta_mode recorder if enabled */
		if ( (obj->flags)&OF_DELTAMODE )
		{
			extern void delta_add_player(OBJECT *);
			delta_add_player(obj);
		}
		return 1; /* success */
	}
	else
		return 0; /* failure */
}

static void rewind_player(struct player *my)
{
	(*my->ops->rewind)(my);
}

static void close_player(struct player *my)
{
	(my->ops->close)(my);
}

static void trim(char *str, char *to){
	int i = 0, j = 0;
	if(str == 0)
		return;
	while(str[i] != 0 && isspace(str[i])){
		++i;
	}
	while(str[i] != 0){
		to[j] = str[i];
		++j;
		++i;
	}
	--j;
	while(j > 0 && isspace(to[j])){
		to[j] = 0; // remove trailing whitespace
		--j;
	}
}

TIMESTAMP player_read(OBJECT *obj)
{
	char buffer[256];
	char timebuf[64], valbuf[256], tbuf[64];
	char tz[6];
	int Y=0,m=0,d=0,H=0,M=0;
	double S=0;
	struct player *my = OBJECTDATA(obj,struct player);
	char unit[2];
	TIMESTAMP t1;
	char *result=NULL;
	char256 value;
	int voff=0;

	/* TODO move this to tape.c and make the variable available to all classes in tape */
	static enum {UNKNOWN,ISO,US,EURO} dateformat = UNKNOWN;
	if ( dateformat==UNKNOWN )
	{
		static char global_dateformat[8]="";
		gl_global_getvar("dateformat",global_dateformat,sizeof(global_dateformat));
		if (strcmp(global_dateformat,"ISO")==0) dateformat = ISO;
		else if (strcmp(global_dateformat,"US")==0) dateformat = US;
		else if (strcmp(global_dateformat,"EURO")==0) dateformat = EURO;
		else dateformat = ISO;
	}

Retry:
	result = my->ops->read(my, buffer, sizeof(buffer));

	memset(timebuf, 0, 64);
	memset(valbuf, 0, 256);
	memset(tbuf, 0, 64);
	memset(value, 0, 256);
	memset(tz, 0, 6);
	if (result==NULL)
	{
		if (my->loopnum>0)
		{
			rewind_player(my);
			my->loopnum--;
			goto Retry;
		}
		else {
			close_player(my);
			my->status=TS_DONE;
			my->next.ts = TS_NEVER;
			my->next.ns = 0;
			goto Done;
		}
	}
	if (result[0]=='#' || result[0]=='\n') /* ignore comments and blank lines */
		goto Retry;

	if(sscanf(result, "%32[^,],%256[^\n\r;]", tbuf, valbuf) == 2){
		trim(tbuf, timebuf);
		trim(valbuf, value);
		if (sscanf(timebuf,"%d-%d-%d %d:%d:%lf %4s",&Y,&m,&d,&H,&M,&S, tz)==7){
			//struct tm dt = {S,M,H,d,m-1,Y-1900,0,0,0};
			DATETIME dt;
			switch ( dateformat ) {
			case ISO:
				dt.year = Y;
				dt.month = m;
				dt.day = d;
				break;
			case US:
				dt.year = d;
				dt.month = Y;
				dt.day = m;
				break;
			case EURO:
				dt.year = d;
				dt.month = m;
				dt.day = Y;
				break;
			}
			dt.hour = H;
			dt.minute = M;
			dt.second = (unsigned short)S;
			dt.nanosecond = (unsigned int)(1e9*(S-dt.second));
			strcpy(dt.tz, tz);
			t1 = (TIMESTAMP)gl_mktime(&dt);
			if ((obj->flags & OF_DELTAMODE)==OF_DELTAMODE)	/* Only request deltamode if we're explicitly enabled */
				enable_deltamode(dt.nanosecond==0?TS_NEVER:t1);
			if (t1!=TS_INVALID && my->loop==my->loopnum){
				my->next.ts = t1;
				my->next.ns = dt.nanosecond;
				while(value[voff] == ' '){
					++voff;
				}
				strcpy(my->next.value, value+voff);
			}
		}
		else if (sscanf(timebuf,"%d-%d-%d %d:%d:%lf",&Y,&m,&d,&H,&M,&S)>=4)
		{
			//struct tm dt = {S,M,H,d,m-1,Y-1900,0,0,0};
			DATETIME dt;
			switch ( dateformat ) {
			case ISO:
				dt.year = Y;
				dt.month = m;
				dt.day = d;
				break;
			case US:
				dt.year = d;
				dt.month = Y;
				dt.day = m;
				break;
			case EURO:
				dt.year = d;
				dt.month = m;
				dt.day = Y;
				break;
			}
			dt.hour = H;
			dt.minute = M;
			dt.second = (unsigned short)S;
			dt.tz[0] = 0;
			dt.nanosecond = (unsigned int)(1e9*(S-dt.second));
			t1 = (TIMESTAMP)gl_mktime(&dt);
			if ((obj->flags & OF_DELTAMODE)==OF_DELTAMODE)	/* Only request deltamode if we're explicitly enabled */
				enable_deltamode(dt.nanosecond==0?TS_NEVER:t1);
			if (t1!=TS_INVALID && my->loop==my->loopnum){
				my->next.ts = t1;
				my->next.ns = dt.nanosecond;
				while(value[voff] == ' '){
					++voff;
				}
				strcpy(my->next.value, value+voff);
			}
		}
		else if (sscanf(timebuf,"%" FMT_INT64 "d%1s", &t1, unit)==2)
		{
			{
				int64 scale=1;
				switch(unit[0]) {
				case 's': scale=TS_SECOND; break;
				case 'm': scale=60*TS_SECOND; break;
				case 'h': scale=3600*TS_SECOND; break;
				case 'd': scale=86400*TS_SECOND; break;
				default: break;
				}
				t1 *= scale;
				if (result[0]=='+'){ /* timeshifts have leading + */
					my->next.ts += t1;
					while(value[voff] == ' '){
						++voff;
					}
					strcpy(my->next.value, value+voff);
				} else if (my->loop==my->loopnum){ /* absolute times are ignored on all but first loops */
					my->next.ts = t1;
					while(value[voff] == ' '){
						++voff;
					}
					strcpy(my->next.value, value+voff);
				}
			}
		}
		else if (sscanf(timebuf,"%lf", &S)==1)
		{
			if (my->loop==my->loopnum) {
				my->next.ts = (unsigned short)S;
				my->next.ns = (unsigned int)(1e9*(S-my->next.ts));
				if ((obj->flags & OF_DELTAMODE)==OF_DELTAMODE)	/* Only request deltamode if we're explicitly enabled */
					enable_deltamode(my->next.ns==0?TS_NEVER:t1);
				while(value[voff] == ' '){
					++voff;
				}
				strcpy(my->next.value, value+voff);
			}
		}
		else
		{
			gl_warning("player was unable to parse timestamp \'%s\'", result);
		}
	} else {
		gl_warning("player was unable to split input string \'%s\'", result);
	}

Done:
	return my->next.ns==0 ? my->next.ts : (my->next.ts+1);
}

EXPORT TIMESTAMP sync_player(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct player *my = OBJECTDATA(obj,struct player);
	TIMESTAMP t1 = (TS_OPEN == my->status) ? my->next.ts : TS_NEVER;
	TIMESTAMP temp_t;

	if (my->status==TS_INIT){

		/* get local target if remote is not used and "value" is defined by the user at runtime */
		if (my->target==NULL && obj->parent == NULL)
			my->target = gl_get_property(obj,"value",NULL);

		if(player_open(obj) == 0)
		{
			gl_error("sync_player: Unable to open player file '%s' for object '%s'", my->file, obj->name?obj->name:"(anon)");
		}
		else
		{
			t1 = player_read(obj);
		}
	}
	while (my->status==TS_OPEN && t1<=t0
		&& my->next.ns==0 ) /* only use this method when not operating in subsecond mode */
	{	/* post this value */
		if (my->target==NULL)
			my->target = gl_get_property(obj->parent,my->property,NULL);
		if (my->target==NULL){
			gl_error("sync_player: Unable to find property \"%s\" in object %s", my->property, obj->name?obj->name:"(anon)");
			my->status = TS_ERROR;
		}
		if (my->target!=NULL)
		{
			OBJECT *target = obj->parent ? obj->parent : obj; /* target myself if no parent */
			gl_set_value(target,GETADDR(target,my->target),my->next.value,my->target); /* pointer => int64 */
		}
		
		/* Copy the current value into our "tracking" variable */
		my->delta_track.ns = my->next.ns;
		my->delta_track.ts = my->next.ts;
		memcpy(my->delta_track.value,my->next.value,sizeof(char256));

		t1 = player_read(obj);
	}

	/* Apply an intermediate value, if necessary - mainly for "DELTA players in non-delta situations" */
	if ((my->target!=NULL) && (my->delta_track.ts<t0) && (my->delta_track.ns!=0))
	{
		OBJECT *target = obj->parent ? obj->parent : obj; /* target myself if no parent */
		gl_set_value(target,GETADDR(target,my->target),my->delta_track.value,my->target); /* pointer => int64 */
	}

	/* Delta-mode catch - if we're not explicitly in delta mode and a nano-second values pops up, try to advance past it */
	if (((obj->flags & OF_DELTAMODE)!=OF_DELTAMODE) && (my->next.ns != 0) && (my->status==TS_OPEN))
	{
		/* Initial value for next time */
		temp_t = t1;

		/* Copied verbatim from above - just in case it still needs to be done - if a first timestep is a deltamode timestep */
		if (my->target==NULL)
			my->target = gl_get_property(obj->parent,my->property,NULL);
		if (my->target==NULL){
			gl_error("sync_player: Unable to find property \"%s\" in object %s", my->property, obj->name?obj->name:"(anon)");
			my->status = TS_ERROR;
		}

		/* Advance to a zero */
		while ((my->next.ns != 0) && (my->next.ts<=t0))
		{
			/* Post the value as we go, so the "final" is correct */
			/* Apply the "current value", if it is relevant (player_next_value contains the previous, so this will override it) */
			if ((my->target!=NULL) && (my->next.ts<t0))
			{
				OBJECT *target = obj->parent ? obj->parent : obj; /* target myself if no parent */
				gl_set_value(target,GETADDR(target,my->target),my->next.value,my->target); /* pointer => int64 */
			}

			/* Copy the value into the tracking variable */
			my->delta_track.ns = my->next.ns;
			my->delta_track.ts = my->next.ts;
			memcpy(my->delta_track.value,my->next.value,sizeof(char256));

			/* Perform the update */
			temp_t = player_read(obj);
		}

		/* Apply the update */
		t1 = temp_t;
	}
	else if (((obj->flags & OF_DELTAMODE)==OF_DELTAMODE) && (delta_mode_needed<t0))	/* Code to catch when gets stuck in deltamode (exits early) */
	{
		if (my->next.ts<t0)
		{
			/* Advance to a value bigger than t0 */
			while ((my->next.ns != 0) && (my->next.ts<=t0))
			{
				/* Post the value as we go, so the "final" is correct */
				/* Apply the "current value", if it is relevant (player_next_value contains the previous, so this will override it) */
				if ((my->target!=NULL) && (my->next.ts<t0))
				{
					OBJECT *target = obj->parent ? obj->parent : obj; /* target myself if no parent */
					gl_set_value(target,GETADDR(target,my->target),my->next.value,my->target); /* pointer => int64 */
				}

				/* Copy the value into the tracking variable */
				my->delta_track.ns = my->next.ns;
				my->delta_track.ts = my->next.ts;
				memcpy(my->delta_track.value,my->next.value,sizeof(char256));

				/* Perform the update */
				temp_t = player_read(obj);
			}

			/* Do the adjustment of return */
			if (t1 < t0)	/* Sometimes gets stuck */
			{
				/* Force it forward */
				t1 = temp_t;
			}
			else	/* Equal or greater than next time */
			{
				if ((temp_t<t1) && (temp_t>t0))
					t1 = temp_t;
			}
		}
		else if (my->next.ts == t0)
		{
			/* Roll it once */
			/* Copy the value into the tracking variable */
			my->delta_track.ns = my->next.ns;
			my->delta_track.ts = my->next.ts;
			memcpy(my->delta_track.value,my->next.value,sizeof(char256));

			/* Perform the update */
			temp_t = player_read(obj);
		}
	}

	/* See if we need to push a sooner update (DELTA round)*/
	if ((my->delta_track.ts!=TS_NEVER) && (my->delta_track.ns != 0))
	{
		/* Round the timestep up */
		temp_t = my->delta_track.ts+1;

		/* Make sure it is still valid - don't want to get stuck iterating */
		if ((temp_t<t1) && (temp_t>t0))
			t1 = temp_t;
	}

	/* Catch for next delta instances that may occur -- if doesn't pass a reiteration, gets missed when nothing driving*/
	if ((obj->flags & OF_DELTAMODE) == OF_DELTAMODE)
	{
		if ((t1>t0) && (my->next.ts==t0) && (my->next.ns != 0))
		{
			/* Return us here, deltamode should have been flagged */
			t1 = t0;
		}

		/* Check for a final stoppping point -- if player value goes ns beyond current stop time, it will get stuck and iterate */
		if ((my->next.ts == gl_globalstoptime) && (my->next.ns != 0) && (t1 == gl_globalstoptime))
		{
			/* Push it one second forward, just so GLD thinks it's done and can get out */
			t1 = t1 + 1;
		}
	}

	obj->clock = t0;
	return t1;
}

/**@}*/
