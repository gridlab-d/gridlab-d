/** $Id: recorder.c 1208 2009-01-12 22:48:16Z d3p988 $
	Copyright (C) 2008 Battelle Memorial Institute
	@file tape.c
	@addtogroup recorder Recorders
	@ingroup tapes

	Tape recorders use the following additional properties
	- \p format specifies whether to use raw timestamp instead of date-time format
	- \p interval specifies the sampling interval to use (0 means every pass, -1 means only transients)
	- \p limit specifies the maximum length limit for the number of samples taken
	- \p trigger specifies a trigger condition on a property to start recording
	the condition is specified in the format \p property \p comparison \p value
	- \p The \p loop property is not available in recording.
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

CLASS *recorder_class = NULL;
static OBJECT *last_recorder = NULL;

EXPORT int create_recorder(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(recorder_class);
	if (*obj!=NULL)
	{
		struct recorder *my = OBJECTDATA(*obj,struct recorder);
		last_recorder = *obj;
		gl_set_parent(*obj,parent);
		strcpy(my->file,"");
		strcpy(my->filetype,"txt");
		strcpy(my->delim,",");
		strcpy(my->property,"(undefined)");
		my->interval = -1; /* transients only */
		my->last.ts = -1;
		strcpy(my->last.value,"");
		my->limit = 0;
		my->samples = 0;
		my->status = TS_INIT;
		my->trigger[0]='\0';
		my->format = 0;
		strcpy(my->plotcommands,"");
		my->target = gl_get_property(*obj,my->property);
		return 1;
	}
	return 0;
}

static int recorder_open(OBJECT *obj)
{
	char32 type="file";
	char1024 fname="";
	char32 flags="w";
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	
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
	my->ops = get_ftable(type)->recorder;
	if(my->ops == NULL)
		return 0;
	return my->ops->open(my, fname, flags);
}

static int write_recorder(struct recorder *my, char *ts, char *value)
{
	return my->ops->write(my, ts, value);
}

static void close_recorder(struct recorder *my)
{
	if (my->ops) my->ops->close(my);
}

static TIMESTAMP recorder_write(OBJECT *obj)
{
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	char ts[64]="0"; /* 0 = INIT */
	if (my->format==0)
	{
		if (my->last.ts>TS_ZERO)
		{
			DATETIME dt;
			gl_localtime(my->last.ts,&dt);
			gl_strtime(&dt,ts,sizeof(ts));
		}
		/* else leave INIT in the buffer */
	}
	else
		sprintf(ts,"%" FMT_INT64 "d", my->last.ts);
	if ((my->limit>0 && my->samples > my->limit) /* limit reached */
		|| write_recorder(my, ts, my->last.value)==0) /* write failed */
	{
		close_recorder(my);
		my->status = TS_DONE;
	}
	else
		my->samples++;
	return TS_NEVER;
}

PROPERTY *link_properties(OBJECT *obj, char *property_list)
{
	char *item;
	PROPERTY *first=NULL, *last=NULL;
	UNIT *unit = NULL;
	char1024 list;
	complex oblig;
	strcpy(list,property_list); /* avoid destroying orginal list */
	for (item=strtok(list,","); item!=NULL; item=strtok(NULL,","))
	{
		char256 pstr, ustr;
		char *cpart = 0;
		int64 cid = -1;
		PROPERTY *prop = NULL;
		PROPERTY *target = NULL;
		double scale = 1.0;

		// everything that looks like a property name, then read units up to ]
		while (isspace(*item)) item++;
		if(2 == sscanf(item,"%[A-Za-z0-9_.][%[^]\n,\0]", pstr, ustr)){
			unit = gl_find_unit(ustr);
			if(unit == NULL){
				gl_error("recorder:%d: unable to find unit '%s' for property '%s'",obj->id, ustr,pstr);
				return NULL;
			}
			item = pstr;
		}
		prop = (PROPERTY*)malloc(sizeof(PROPERTY));
		
		/* branch: test to see if we're trying to split up a complex property */
		/* must occur w/ *cpart=0 before gl_get_property in order to properly reformat the property name string */
		cpart = strchr(item, '.');
		if(cpart != NULL){
			if(strcmp("imag", cpart+1) == 0){
				cid = (int)((int64)&(oblig.i) - (int64)&oblig);
				*cpart = 0;
			} else if(strcmp("real", cpart+1) == 0){
				cid = (int)((int64)&(oblig.r) - (int64)&oblig);
				*cpart = 0;
			} else {
				;
			}
		}

		target = gl_get_property(obj,item);

		if (prop!=NULL && target!=NULL)
		{
			if(unit != NULL && target->unit == NULL){
				gl_error("recorder:%d: property '%s' is unitless, ignoring unit conversion", obj->id, item);
			}
			else if(unit != NULL && 0 == gl_convert_ex(target->unit, unit, &scale))
			{
				gl_error("recorder:%d: unable to convert property '%s' units to '%s'", obj->id, item, ustr);
				return NULL;
			}
			if (first==NULL) first=prop; else last->next=prop;
			last=prop;
			memcpy(prop,target,sizeof(PROPERTY));
			if(unit) prop->unit = unit;
			prop->next = NULL;
		}
		else
		{
			gl_error("recorder: property '%s' not found", item);
			return NULL;
		}
		if(cid >= 0){ /* doing the complex part thing */
			prop->ptype = PT_double;
			(prop->addr) = (int64)(prop->addr) + cid;
		}
	}
	return first;
}

int read_properties(OBJECT *obj, PROPERTY *prop, char *buffer, int size)
{
	PROPERTY *p;
	int offset=0;
	int count=0;
	for (p=prop; p!=NULL && offset<size-33; p=p->next)
	{
		if (offset>0) strcpy(buffer+offset++,",");
		offset+=gl_get_value(obj,GETADDR(obj,p),buffer+offset,size-offset-1,p); /* pointer => int64 */
		buffer[offset]='\0';
		count++;
	}
	return count;
}

EXPORT TIMESTAMP sync_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct recorder *my = OBJECTDATA(obj,struct recorder);
	typedef enum {NONE='\0', LT='<', EQ='=', GT='>'} COMPAREOP;
	COMPAREOP comparison;
	char1024 buffer = "";
	
	if (my->status==TS_DONE)
	{
		close_recorder(my); /* note: potentially called every sync pass for multiple timesteps, catch fp==NULL in tape ops */
		return TS_NEVER;
	}

	if(obj->parent == NULL){
		char tb[32];
		sprintf(buffer, "'%s' lacks a parent object", obj->name ? obj->name : tb, sprintf(tb, "recorder:%i", obj->id));
		close_recorder(my);
		my->status = TS_ERROR;
		goto Error;
	}

	if(my->last.ts < 1 && my->interval != -1)
		my->last.ts = t0;

	/* connect to property */
	if (my->target==NULL){
		my->target = link_properties(obj->parent,my->property);
	}
	if (my->target==NULL)
	{
		sprintf(buffer,"'%s' contains a property of %s %d that is not found", my->property, obj->parent->oclass->name, obj->parent->id);
		close_recorder(my);
		my->status = TS_ERROR;
		goto Error;
	}

	// update clock
	if ((my->status==TS_OPEN) && (t0 > obj->clock)) 
	{	
		obj->clock = t0;
		// if the recorder is clock-based, write the value
		if((my->interval > 0) && (my->last.ts+my->interval <= t0) && (my->last.value[0] != 0)){
			my->last.ts = t0;
			recorder_write(obj);
		}
	}

	/* update property value */
	if ((my->target != NULL) && (read_properties(obj->parent,my->target,buffer,sizeof(buffer))==0))
	{
		sprintf(buffer,"unable to read property '%s' of %s %d", my->property, obj->parent->oclass->name, obj->parent->id);
		close_recorder(my);
		my->status = TS_ERROR;
	}

	/* check trigger, if any */
	comparison = (COMPAREOP)my->trigger[0];
	if (comparison!=NONE)
	{
		int desired = comparison==LT ? -1 : (comparison==EQ ? 0 : (comparison==GT ? 1 : -2));

		/* if not trigger or can't get access */
		int actual = strcmp(buffer,my->trigger+1);
		if (actual!=desired || (my->status==TS_INIT && !recorder_open(obj)))
		{
			/* better luck next time */
			return (my->interval==0 || my->interval==-1) ? TS_NEVER : t0+my->interval;
		}
	}
	else if (my->status==TS_INIT && !recorder_open(obj))
	{
		close_recorder(my);
		return TS_NEVER;
	}

	if(my->last.ts < 1 && my->interval != -1)
		my->last.ts = t0;

	/* write tape */
	if (my->status==TS_OPEN)
	{	
		if (my->interval==0 /* sample on every pass */
			|| ((my->interval==-1) && my->last.ts!=t0 && strcmp(buffer,my->last.value)!=0) /* sample only when value changes */
			)
//			|| (my->interval>0 && my->last.ts+my->interval<=t0)) /* sample regularly */

		{
			strncpy(my->last.value,buffer,sizeof(my->last.value));
			my->last.ts = t0;
			//my->last.ts = obj->parent;
//			strncpy(my->last.value,buffer,sizeof(my->last.value));
			recorder_write(obj);
		} else if (my->interval > 0){
			strncpy(my->last.value,buffer,sizeof(my->last.value));
		}
	}
Error:
	if (my->status==TS_ERROR)
	{
		gl_error("recorder %d %s\n",obj->id, buffer);
		close_recorder(my);
		my->status=TS_DONE;
		return TS_NEVER;
	}

	if (my->interval==0 || my->interval==-1) 
	{
		//close_recorder(my);
		return TS_NEVER;
	}
	else
		return my->last.ts+my->interval;
}

/**@}*/
