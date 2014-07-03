/** $Id: shaper.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file shaper.c
	@addtogroup shaper Shapers
	@ingroup tapes

	Shapers produce boundary conditions based on a shape, either
	by playing a scaled result that conforms to the defined shape, or
	producing a series of pulse-width modulated events of a set amplitude
	that aggregate over time to the given shape.

	Shape files differ from tapes in that they must define the conditions
	that give rise to the shape.  Entries for different shapes are given in 
	shape blocks.  
	
	\verbatim
	# Winter weekday sample
	sample-winter-weekday {
		* 0-8 * 10-3 1-5,0.1
		* 9-16 * 10-3 1-5,0.4
		* 16-0 * 10-3 1-5,0.2
	}
	\endverbatim

	Each shape block is a collection of Posix-style \p cron 
	entries followed by a numeric value.  The values are normalized
	when loaded before being scaled and applied to the target object.

	Multiple blocks having different shapes can be defined, provided they
	do not define more than one value for any given hour of the year.  The
	shaper will examine the shape when loading it to verify that all hours
	of the year are defined exactly once.
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


CLASS *shaper_class = NULL;
static OBJECT *last_shaper = NULL;

EXPORT int create_shaper(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(shaper_class);
	if (*obj!=NULL)
	{
		struct shaper *my = OBJECTDATA(*obj,struct shaper);
		last_shaper = *obj;
		gl_set_parent(*obj,parent);
		strcpy(my->file,"");
		strcpy(my->filetype,"txt");
		strcpy(my->property,"");
		strcpy(my->group,"");
		strcpy(my->mode, "file");
		my->loopnum = 0;
		my->status = TS_INIT;
		my->targets = NULL;
		memset(my->shape,0,sizeof(my->shape));
		my->scale = 1.0;	/* scale of shape values */
		my->magnitude = 1.0;	/* magnitude of 1 event (only used when QUEUE is enabled) */
		my->step = 3600;	/* one hour shape steps */
		my->interval = 24; /* one day unit integral of shape */
		my->events = 0;		/* no events by default (disables QUEUE) */
		memset(my->lasterr,0,sizeof(my->lasterr));
		return 1;
	}
	return 0;
}

static int shaper_open(OBJECT *obj)
{
	char32 type="file";
	char1024 fname="";
	char32 flags="r";
	TAPEFUNCS *fns;
	struct shaper *my = OBJECTDATA(obj,struct shaper);
	
	/* if prefix is omitted (no colons found) */
//	if (sscanf(my->file,"%32[^:]:%1024[^:]:%[^:]",type,fname,flags)==1)
//	{
		/* filename is file by default */
		strcpy(fname,my->file);
//		strcpy(type,"file");
//	}

	/* if no filename given */
	if (strcmp(fname,"")==0)

		/* use object name-id as default file name */
		sprintf(fname,"%s-%d.%s",obj->parent->oclass->name,obj->parent->id, my->filetype);

	/* if type is file or file is stdin */
	fns = get_ftable(my->mode);
	if (fns==NULL)
		return 0;
	my->ops = fns->shaper;
	if(my->ops == NULL)
		return 0;
	if (my->ops->open(my, fname, flags))
		return 1;
	gl_error("%s",my->lasterr[0]?my->lasterr:"unknown error");
	return 0;
}

static void rewind_shaper(struct shaper *my)
{
	my->ops->rewind(my);
}

static void close_shaper(struct shaper *my)
{
	my->ops->close(my);
}

static TIMESTAMP shaper_read(OBJECT *obj, TIMESTAMP t0, unsigned int n)
{
	struct shaper *my = OBJECTDATA(obj,struct shaper);
	TIMESTAMP t1 = TS_NEVER;

	/* determine shape time */
	time_t t = (time_t)(t0/TS_SECOND);
	struct tm *tval = localtime(&t); /* TODO: this should use machine local time, but sim local time */

	/* set the value at that time */
	if (my->events<=0) /* direct shape */
	{
		/* value is directly injected */
		my->targets[n].value = my->magnitude * my->shape[tval->tm_mon][tval->tm_mday][tval->tm_wday][tval->tm_hour] * my->scale;

		/* determine time of next change in shape */
		t1 = my->targets[n].ts = ((t0 / my->step) + 1)*my->step;
	}
	else /* shape queue */
	{
		/* value is added to queue */ 
		my->targets[n].value += my->shape[tval->tm_mon][tval->tm_mday][tval->tm_wday][tval->tm_hour] * my->scale * my->events;

		/* compute time to the start of the event */
		/** @todo shaper queues shouldn't be uniform over the time step, but cumulative uniform (tape, medium priority) */
		my->targets[n].ts = t0 + (TIMESTAMP)gl_random_uniform(&(obj->rng_state),0,my->step);
	}

	return my->targets[n].ts;
}

static TIMESTAMP shaper_update(OBJECT *obj, TIMESTAMP t0, unsigned int n)
{
	struct shaper *my = OBJECTDATA(obj,struct shaper);
	TIMESTAMP t1 = TS_NEVER;
	if (my->events<=0) /* direct shaper */
	{
		/* value is directly injected */
		*(my->targets[n].addr) = my->targets[n].value;
	}
	else if (*(my->targets[n].addr)==0 && gl_random_bernoulli(&(obj->rng_state),my->targets[n].value)==1)	/* shape queue event starts */
	{	
		double event_size = gl_random_uniform(&(obj->rng_state),0,1);

		/* magnitude is fixed */
		*(my->targets[n].addr) = my->magnitude;

		/* integral determines time to end of 1 event */
		t1 = my->targets[n].ts = t0 + (TIMESTAMP)(event_size*my->step*my->interval/my->magnitude);
		my->targets[n].value-=event_size; /* remove event from queue */
	}
	else if (*(my->targets[n].addr)!=0 && t0==my->targets[n].ts) /* shape queue event ends */ 
	{	/* event ends */
		/* read next shape event */
		t1 = shaper_read(obj,t0,n);

		/* clear target value */
		*(my->targets[n].addr) = 0;
	}
	return t1;
}

EXPORT TIMESTAMP sync_shaper(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass)
{
	struct shaper *my = OBJECTDATA(obj,struct shaper);
	TIMESTAMP t1 = TS_NEVER;
	if (my->status==TS_INIT)
		shaper_open(obj);
	if (my->status==TS_OPEN)
	{	
		/* build target list */
		if (my->targets==NULL && my->group[0]!='\0')
		{
			FINDLIST *object_list = gl_find_objects(FL_GROUP,my->group);
			OBJECT *item=NULL;
			int n=0;
			if (object_list==NULL || object_list->hit_count<=0)
			{
				gl_warning("shaper group '%s' is empty", my->group);
				my->status=TS_DONE;
				return TS_NEVER;
			}
			my->n_targets = object_list->hit_count;
			my->targets = (SHAPERTARGET*)gl_malloc(sizeof(SHAPERTARGET)*my->n_targets);
			memset(my->targets,0,sizeof(SHAPERTARGET)*my->n_targets);
			if (my->targets==NULL)
			{
				gl_error("shaper memory allocation failed!");
				my->status=TS_DONE;
				return TS_NEVER;
			}
			for ( ;(item=gl_find_next(object_list,item))!=NULL; n++)
			{
				PROPERTY *prop=gl_get_property(item,my->property,NULL);
				if (prop!=NULL)
				{
					if (prop->ptype==PT_double)
					{
						TIMESTAMP tn;

						/* get the address of the property */
						my->targets[n].addr = (double*)(gl_get_addr(item,prop->name)); /* pointer => int64 */
						if (my->targets[n].addr<(double*)(item+1))
						{
							if (my->targets[n].addr>NULL)
								GL_THROW("gl_get_addr(OBJECT *obj=[%s (%s:%d)], char *name='%s') return an invalid non-NULL pointer", item->name?item->name:"unnamed object", item->oclass->name, obj->id,prop->name);
							else
								GL_THROW("property '%s' not found in %s (%s:%d)", prop->name, item->name?item->name:"unnamed object", item->oclass->name, item->id);
						}

						/* get the next event */
						tn = shaper_read(obj,t0,n);
						if (tn<t1) t1=tn;
					}
					else
						gl_warning("object %s:%d property %s is not a double", item->oclass->name,item->id, prop->name);
				} else {
					gl_error("object %s:%d property %s not found in object %s", obj->oclass->name,obj->id, my->property, item->oclass->name,item->id);
				}
			}
		}

		/* write to targets */
		if (my->targets!=NULL)
		{
			unsigned int n;
			for (n=0; n<my->n_targets; n++)
			{
				TIMESTAMP tn = TS_NEVER;

				/* get shape event/data */
				if (my->targets[n].ts==t0) 
				{
					/* post the new value */
					tn = shaper_update(obj,t0,n);

					/* read the next value */
					if (tn==TS_NEVER)
						tn = shaper_read(obj,t0,n);
				}

				/* make sure caller knows next event time */
				else if (my->targets[n].ts<t1)
					tn = my->targets[n].ts;

				/* keep track of "nextest" event */
				if (tn<t1) t1=tn;
			}
		}
	}
	obj->clock = t0;
	if(t1 == 0){
		gl_error("shaper:%i will return t1==0 ~ check the shaper's target property, \"%s\"", obj->id, my->property);
	}
	return t1!=TS_NEVER?-t1:TS_NEVER; /* negative indicates a "soft" event which is only considered for stepping, not for stopping */
}

/**@}*/
