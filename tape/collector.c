/** $Id: collector.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file collector.c
	@addtogroup collector Collectors
	@ingroup tapes

	Tape collectors are different from recorders in that they aggregate multiple
	object properties into a single value.  They do not use the parent property, but
	instead use the 'group' property to form a collection of objects over which the
	aggregate is taken.  The following properties are also available for collectors
	- \p group specifies the grouping rule for creating the collection.  Groups
	   may be specified using \p property \p condition \p value, where \p property is one
	   of \p class, \p size, \p parent, \p id, \p rank, or any registered property of the object.
	- \p property value of collectors must be in the form \p aggregator(property) where
	the \p aggregator is one of \p min, \p max, \p count, \p avg, \p std, \p mean, \p var, 
	\p mbe, \p kur. If a \p | is used instead of parentheses, then the absolute value 
	of the property is used.
	If the property is a complex number, the property must be specified in the form
	\p property.part, where \p part is one of \p real, \p imag, \p mag, \p ang, or \p arg.  
	Angles are in degrees, and \p arg is in radians. 
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

CLASS *collector_class = NULL;
static OBJECT *last_collector = NULL;

EXPORT int create_collector(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(collector_class);
	if (*obj!=NULL)
	{
		struct collector *my = OBJECTDATA(*obj,struct collector);
		last_collector = *obj;
		gl_set_parent(*obj,parent);
		strcpy(my->file,"");
		strcpy(my->filetype,"txt");
		strcpy(my->delim,",");
		strcpy(my->property,"(undefined)");
		strcpy(my->group,"");
		my->interval = TS_NEVER; /* transients only */
		my->last.ts = -1;
		strcpy(my->last.value,"");
		my->limit = 0;
		my->samples = 0;
		my->status = TS_INIT;
		my->trigger[0]='\0';
		my->format = 0;
		my->aggr = NULL;
		return 1;
	}
	return 0;
}

static int collector_open(OBJECT *obj)
{
	char32 type="file";
	char1024 fname="";
	char32 flags="w";
	struct collector *my = OBJECTDATA(obj,struct collector);
	
	/* if prefix is omitted (no colons found) */
	if (sscanf(my->file,"%32[^:]:%1024[^:]:%[^:]",type,fname,flags)==1)
	{
		/* filename is file by default */
		strcpy(fname,my->file);
		strcpy(type,"file");
	}

	/* if no filename given */
	if (strcmp(fname,"")==0)
	{
		char *p;
		/* use group spec as default file name */
		sprintf(fname,"%s.%s",my->group,my->filetype);

		/* but change disallowed characters to _ */
		for (p=fname; *p!='\0'; p++)
		{
			if (!isalnum(*p) && *p!='-' && *p!='.')
				*p='_';
		}
	}

	/* if type is file or file is stdin */
	my->ops = get_ftable(type)->collector;
	if(my->ops == NULL)
		return 0;
	return my->ops->open(my, fname, flags);
	/*
	if (strcmp(type,"file")==0)
		return file_open_collector(my,fname,flags);
	else if (strcmp(type,"odbc")==0)
		return odbc_open_collector(my,fname,flags);
	else if (strcmp(type,"memory")==0)
		return memory_open_collector(my,fname,flags);
	else
		return 0;
	*/
}

static int write_collector(struct collector *my, char *ts, char *value)
{
	return my->ops->write(my, ts, value);

	switch (my->type) {
	case FT_FILE:
		return file_write_collector(my,ts,value);
		break;
	case FT_ODBC:
		return odbc_write_collector(my,ts,value);
		break;
	case FT_MEMORY:
		return memory_write_collector(my,ts,value);
		break;
	default:
		return 0;
		break;
	}
}

static TIMESTAMP collector_write(OBJECT *obj)
{
	struct collector *my = OBJECTDATA(obj,struct collector);
	char ts[64];
	if (my->format==0)
	{
		time_t t = (time_t)(my->last.ts*TS_SECOND);
		strftime(ts,sizeof(ts),timestamp_format, gmtime(&t));
	}
	else
		sprintf(ts,"%" FMT_INT64 "d", my->last.ts);
	if ((my->limit>0 && my->samples>my->limit) /* limit reached */
		|| write_collector(my,ts,my->last.value)==0) /* write failed */
	{
		switch (my->type) {
		case FT_FILE:
			file_close_collector(my);
			break;
		case FT_ODBC:
			odbc_close_collector(my);
			break;
		case FT_MEMORY:
			memory_close_collector(my);
			break;
		default:
			break;
		}
		my->status = TS_DONE;
	}
	else
		my->samples++;
	return TS_NEVER;
}

AGGREGATION *link_aggregates(char *aggregate_list, char *group)
{
	char *item;
	AGGREGATION *first=NULL, *last=NULL;
	char1024 list;
	strcpy(list,aggregate_list); /* avoid destroying orginal list */
	for (item=strtok(list,","); item!=NULL; item=strtok(NULL,","))
	{
		AGGREGATION *aggr = gl_create_aggregate(item,group);
		if (aggr!=NULL)
		{
			/* TODO: ideally the aggregation group program from the previous should be reused */
			if (first==NULL) first=aggr; else last->next=aggr;
			last=aggr;
			aggr->next = NULL;
		}
		else
			return NULL;
	}
	return first;
}

int read_aggregates(AGGREGATION *aggr, char *buffer, int size)
{
	AGGREGATION *p;
	int offset=0;
	int count=0;
	for (p=aggr; p!=NULL && offset<size-33; p=p->next)
	{
		if (offset>0) strcpy(buffer+offset++,",");
		offset+=sprintf(buffer+offset,"%lg",gl_run_aggregate(p));
		buffer[offset]='\0';
		count++;
	}
	return count;
}

EXPORT TIMESTAMP sync_collector(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct collector *my = OBJECTDATA(obj,struct collector);
	typedef enum {NONE='\0', LT='<', EQ='=', GT='>'} COMPAREOP;
	COMPAREOP comparison;
	char1024 buffer = "";
	
	if (my->status==TS_DONE)
		return TS_NEVER;

	/* connect to property */
	if (my->aggr==NULL)
		my->aggr = link_aggregates(my->property,my->group);

	/* read property */
	if (my->aggr==NULL)
	{
		sprintf(buffer,"'%s' contains an aggregate that is not found in the group '%s'", my->property, my->group);
		my->status = TS_ERROR;
	}
	else if (read_aggregates(my->aggr,buffer,sizeof(buffer))==0)
	{
		sprintf(buffer,"unable to read aggregate '%s' of group '%s'", my->property, my->group);
		my->status = TS_ERROR;
	}

	/* check trigger, if any */
	comparison = (COMPAREOP)my->trigger[0];
	if (comparison!=NONE)
	{
		int desired = comparison==LT ? -1 : (comparison==EQ ? 0 : (comparison==GT ? 1 : -2));

		/* if not trigger or can't get access */
		int actual = strcmp(buffer,my->trigger+1);
		if (actual!=desired || (my->status==TS_INIT && !collector_open(obj)))

			/* better luck next time */
			return TS_NEVER;
	}
	else if (my->status==TS_INIT && !collector_open(obj))
		return TS_NEVER;

	/* write tape */
	if (my->status==TS_OPEN)
	{	
		if (my->interval==0 /* sample on every pass */
			|| ((my->interval==-1) && my->last.ts!=t0 && strcmp(buffer,my->last.value)!=0) /* sample only when value changes */
			|| (my->interval>0 && my->last.ts+my->interval<=t0)) /* sample regularly */
		{
			my->last.ts = t0;
			strncpy(my->last.value,buffer,sizeof(my->last.value));
			collector_write(obj);
		}
	}
	else if (my->status==TS_ERROR)
	{
		gl_error("collector %d %s\n",obj->id, buffer);
		my->status=TS_DONE;
		return 0; /* failed */
	}

	return (my->interval==0 || my->interval==-1) ? TS_NEVER : my->last.ts+my->interval;
}

/**@}*/
