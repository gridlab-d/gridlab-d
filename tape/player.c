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
		strcpy(my->property,"(undefined)");
		my->next.ts = TS_ZERO;
		strcpy(my->next.value,"");
		my->loopnum = 0;
		my->loop = 0;
		my->status = TS_INIT;
		my->target = gl_get_property(*obj,my->property);
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
	if (sscanf(my->file,"%32[^:]:%1024[^:]:%[^:]",type,fname,flags)==1)
	{
		/* filename is file by default */
		strcpy(fname,my->file);
		strcpy(type,"file");
	}

	/* if no filename given */
	if (strcmp(fname,"")==0)

		/* use object name-id as default file name */
		sprintf(fname,"%s-%d.%s",obj->parent->oclass->name,obj->parent->id, my->filetype);

	/* if type is file or file is stdin */
	tf = get_ftable(type);
	if(tf == NULL)
		return 0;
	my->ops = tf->player;
	if(my->ops == NULL)
		return 0;
	else return (my->ops->open)(my, fname, flags);
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

static TIMESTAMP player_read(OBJECT *obj)
{
	char buffer[64];
	char timebuf[64], valbuf[64], tbuf[64];

	int Y,m,d,H,M,S;
	char tz[6];
	struct player *my = OBJECTDATA(obj,struct player);
	char unit[2];
	TIMESTAMP t1;
	char *result=NULL;
	char32 value;
	int voff=0;
Retry:
	result = my->ops->read(my, buffer, sizeof(buffer));

	memset(timebuf, 0, 64);
	memset(valbuf, 0, 64);
	memset(tbuf, 0, 64);
	memset(value, 0, 32);
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
			goto Done;
		}
	}
	if (result[0]=='#' || result[0]=='\n') /* ignore comments and blank lines */
		goto Retry;

	if(sscanf(result, "%[^,],%[^\n\r]", tbuf, valbuf) == 2){
		trim(tbuf, timebuf);
		trim(valbuf, value);
		if (sscanf(timebuf,"%d-%d-%d %d:%d:%d %4s",&Y,&m,&d,&H,&M,&S, tz)==7){
			//struct tm dt = {S,M,H,d,m-1,Y-1900,0,0,0};
			DATETIME dt;
			dt.year = Y;
			dt.month = m;
			dt.day = d;
			dt.hour = H;
			dt.minute = M;
			dt.second = S;
			strcpy(dt.tz, tz);
			t1 = (TIMESTAMP)gl_mktime(&dt);
			if (t1!=TS_INVALID && my->loop==my->loopnum){
				my->next.ts = t1;
				while(value[voff] == ' '){
					++voff;
				}
				strcpy(my->next.value, value+voff);
			}
		}
		else if (sscanf(timebuf,"%d-%d-%d %d:%d:%d",&Y,&m,&d,&H,&M,&S)>=4)
		{
			//struct tm dt = {S,M,H,d,m-1,Y-1900,0,0,0};
			DATETIME dt;
			dt.year = Y;
			dt.month = m;
			dt.day = d;
			dt.hour = H;
			dt.minute = M;
			dt.second = S;
			dt.tz[0] = 0;
			t1 = (TIMESTAMP)gl_mktime(&dt);
			if (t1!=TS_INVALID && my->loop==my->loopnum){
				my->next.ts = t1;
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
		else if (sscanf(timebuf,"%" FMT_INT64 "d", &t1)==1)
		{
			if (my->loop==my->loopnum) {
				my->next.ts = t1;
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
	return my->next.ts;
}

EXPORT TIMESTAMP sync_player(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct player *my = OBJECTDATA(obj,struct player);
	TIMESTAMP t1 = (TS_OPEN == my->status) ? my->next.ts : TS_NEVER;
	if (my->status==TS_INIT){

		/* get local target if remote is not used and "value" is defined by the user at runtime */
		if (my->target==NULL && obj->parent == NULL)
			my->target = gl_get_property(obj,"value");

		if(player_open(obj) == 0)
		{
			gl_error("sync_player: Unable to open player file '%s' for object '%s'", my->file, obj->name?obj->name:"(anon)");
		}
		else
		{
			t1 = player_read(obj);
		}
	}
	while (my->status==TS_OPEN && t1<=t0)
	{	/* post this value */
		if (my->target==NULL)
			my->target = gl_get_property(obj->parent,my->property);
		if (my->target==NULL){
			gl_error("sync_player: Unable to find property \"%s\" in object %s", my->property, obj->name?obj->name:"(anon)");
			my->status = TS_ERROR;
		}
		if (my->target!=NULL)
		{
			OBJECT *target = obj->parent ? obj->parent : obj; /* target myself if no parent */
			gl_set_value(target,GETADDR(target,my->target),my->next.value,my->target); /* pointer => int64 */
		}
		t1 = player_read(obj);
	}
	obj->clock = t0;
	return t1;
}

/**@}*/
