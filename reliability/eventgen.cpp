/** $Id: eventgen.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file eventgen.cpp
	@class eventgen Event generator
	@ingroup reliability

	The event generation object synthesizing reliability events for a target
	group of object in a GridLAB-D model.  The following parameters are used
	to generate events

	- \p group is used to identify the characteristics of the target group.
	  See find_mkpgm for details on \p group syntax.
	- \p targets identifies the target properties that will be changed when
	  an event is generated.
	- \p values identifies the values to be written to the target properties
	  when an event is generated.
	- \p frequency specifies the frequency with which events are generated
	- \p duration specifies the duration of events when generated.

	The current implementation generates events with an exponential distribution.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "metrics.h"
#include "eventgen.h"

CLASS *eventgen::oclass = NULL;			/**< a pointer to the CLASS definition in GridLAB-D's core */
eventgen *eventgen::defaults = NULL;	/**< a pointer to the default values used when creating new objects */

static PASSCONFIG passconfig = PC_PRETOPDOWN;
static PASSCONFIG clockpass = PC_PRETOPDOWN;

/* Class registration is only called once to register the class with the core */
eventgen::eventgen(MODULE *module)
{
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"eventgen",sizeof(eventgen),passconfig);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
			PT_char256, "group", PADDR(group),
			PT_char256, "targets", PADDR(targets),
			PT_double, "frequency[1/yr]", PADDR(frequency),
			PT_double, "duration[s]", PADDR(duration),
			PT_char256, "values", PADDR(values),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		defaults = this;
		memset(defaults,0,sizeof(eventgen));
	}
}

/* Object creation is called once for each object that is created by the core */
int eventgen::create(void)
{
	memcpy(this,defaults,sizeof(eventgen));
	return 1; /* return 1 on success, 0 on failure */
}

/* Object initialization is called once after all object have been created */
int eventgen::init(OBJECT *parent)
{
	objectlist = gl_find_objects(FL_GROUP,group);
	if (objectlist==NULL)
		gl_error("eventgen:%d group '%s' is empty", OBJECTHDR(this)->id, group);
	if (strcmp(parent->oclass->name,"metrics")!=0)
		gl_error("eventgen:%d parent must be a metrics object", OBJECTHDR(this)->id);
	if (duration<=1) /* it may be that duration must also be less 2 */
		gl_error("eventgen:%d duration must be greater than 1", OBJECTHDR(this)->id);
	return 1; /* return 1 on success, 0 on failure */
}

/* Presync is called when the clock needs to advance on the first top-down pass */
TIMESTAMP eventgen::presync(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP t2 = TS_NEVER;
	metrics *pMetrics = (metrics*)OBJECTDATA(OBJECTHDR(this)->parent,metrics);
	EVENT *pEvent = pMetrics->get_event();
	if (pEvent==NULL) // need to create the next event
	{
MakeAnother:
		pEvent = new EVENT;
		pMetrics->add_event(pEvent);
		pEvent->event_time = t1 + (TIMESTAMP)(gl_random_exponential(1/(1/frequency*86400*365.24))*TS_SECOND); /* l=1/mean_time */
		do {
			pEvent->ri = gl_random_pareto(1,1/(duration-1)); /* k=1/(mean-1) */
		} while (pEvent->ri>86400*5); /* forbid outages more than 5 days */
		t2 = pEvent->event_time;
		DATETIME dt;
		char buffer[64];
		gl_localtime(t2,&dt);
		if (gl_strtime(&dt,buffer,sizeof(buffer)))
			gl_verbose("event group '%s' will occur at %s", group, buffer);
		else
			gl_verbose("event group '%s' is scheduled to occur at an invalid time (t=%"FMT_INT64"d)", t2);
	}
	else if (event_object) // an event is under way
	{ 
		if (t1>=pEvent->event_time+(TIMESTAMP)(pEvent->ri*TS_SECOND)) // the event has ended
		{
			pMetrics->end_event(event_object,targets,old_values);
			event_object = NULL;
			goto MakeAnother;
		}
		else // the event has not yet ended
			t2 = pEvent->event_time+(TIMESTAMP)(pEvent->ri)*TS_SECOND;
	}
	else if (t1>=pEvent->event_time) // the event is now starting
	{
		event_object = pMetrics->start_event(pEvent,objectlist,targets,values,old_values,sizeof(old_values));
		if (event_object)
		{
			gl_verbose("event lasts %.1f seconds", pEvent->ri);
			t2 = t1 + (TIMESTAMP)(pEvent->ri*TS_SECOND);
		}
		else
		{
			gl_error("unable to implement event!");
			pMetrics->del_event();
			delete pEvent;
		}			
	}
	return -t2; /* return t2>t1 on success, t2=t1 for retry, t2<t1 on failure */
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_eventgen(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(eventgen::oclass);
		if (*obj!=NULL)
		{
			eventgen *my = OBJECTDATA(*obj,eventgen);
			gl_set_parent(*obj,parent);
			return my->create();
		}
	}
	catch (char *msg)
	{
		gl_error("create_eventgen: %s", msg);

	}
	return 1;
}

EXPORT int init_eventgen(OBJECT *obj, OBJECT *parent)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,eventgen)->init(parent);
	}
	catch (char *msg)
	{
		gl_error("init_eventgen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return 1;
}

EXPORT TIMESTAMP sync_eventgen(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	TIMESTAMP t2 = TS_NEVER;
	eventgen *my = OBJECTDATA(obj,eventgen);
	try
	{
		switch (pass) {
		case PC_PRETOPDOWN:
			t2 = my->presync(obj->clock,t1);
			if ((t2<0?-t2:t2)<=t1)
				gl_warning("eventgen did not advanced the clock!");
			break;
		default:
			GL_THROW("invalid pass request (%d)", pass);
			break;
		}
		if (pass==clockpass)
			obj->clock = t1;
	}
	catch (char *msg)
	{
		gl_error("sync_eventgen(obj=%d;%s): %s", obj->id, obj->name?obj->name:"unnamed", msg);
	}
	return t2;
}

/**@}*/
