/** $Id: comm.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file comm.cpp
	@defgroup plc_comm Communication networks (comm)
	@ingroup plc

	Messages are sent from one PLC object to another using PLC code.  The following
	functions are available to send and receive messages.

	- \code void SNDMSG(char *destination, char *string); \endcode is used to send a 
	string message to a PLC object.

	- \code void SNDDAT(char *destination, void *buffer, unsigned int size); \endcode 
	is used to send a binary message to a PLC object.

	- \code int RCVMSG(char source[], char buffer[]); \endcode is used 
	receive a string messages from a PLC object.  The buffer must be an array for
	which sizeof() returns the correct maximum length.

	- \code int RCVDAT(char source[], void *buffer, unsigned int size); \endcode 
	is used to receive a binary message from a PLC object.

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "gridlabd.h"
#include "comm.h"

unsigned int64 Message::next_id = 0;

/////////////////////////////////////////////////////////
// Queue implementation
Queue::Queue(void)
{
	gl_verbose("creating queue %x", this);
	first = last = NULL;
}
Queue::~Queue(void)
{
	gl_verbose("destroying queue %x", this);
	while (first!=NULL)
	{
		Message *next = first->get_next();
		delete first;
		first = next;
	}
}
void Queue::add(Message *msg)
{
	// @todo message need to be sorted by delivery time
	if (first==NULL)
		first = last = msg;
	else
	{
		msg->set_prev(last);
		last->set_next(msg);
	}
	msg->set_next(NULL);
	gl_verbose("message %"FMT_INT64"d '%-8.8s%s' added to queue %x", msg->get_id(), msg->get_data(), msg->get_size()>8?"...":"", this);
}
Message *Queue::peek(void)
{
	return first;
}
Message *Queue::take(Message *msg)
{
	if (msg==NULL)
		msg = first;
	if (msg==NULL)
		return NULL;
	Message *prev = msg->get_prev();
	Message *next = msg->get_next();
	if (prev!=NULL)
		prev->set_next(next);
	else
		first = next;
	if (next!=NULL)
		next->set_prev(prev);
	else
		last = prev;
	return msg;
}
Message *Queue::next(Message *msg)
{
	return msg?msg->get_next():first;
}

Message *Queue::drop(Message *msg, bool reverse)
{
	Message *prev = msg->get_prev();
	Message *next = msg->get_next();
	if (msg!=NULL)
		delete take(msg);
	return reverse?prev:next;
}

TIMESTAMP Queue::next_delivery_time(void)
{
	return first ? first->get_deliverytime() : TS_NEVER;
}

//////////////////////////////////////////////////
// comm implementation
CLASS *comm::oclass = NULL; ///< a pointer to the registered object class definition
comm *comm::defaults = NULL; ///< a pointer to the default values for new objects

void comm::route(Message *msg)
{
	msg->set_deliverytime((TIMESTAMP)(gl_randomvalue(rtype,a,b)/TS_SECOND)+OBJECTHDR(this)->clock);
	DATETIME dt;
	char ts[64]="unknown time";
	gl_localtime(msg->get_deliverytime(),&dt);
	gl_strtime(&dt,ts,sizeof(ts));
	gl_verbose("message %"FMT_INT64"d from %x to %x will be delivered at %s", msg->get_id(), msg->get_src(), msg->get_dst(), ts);
	queue.add(msg);
}

void comm::route(comm *net, Message *msg)
{
	net->queue.add(msg);
}

/** Register the new object class and construct the default object properties */
comm::comm(MODULE *mod)
: queue()
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"comm",sizeof(comm),PC_PRETOPDOWN|PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class comm";
		else
			oclass->trl = TRL_PROOF;

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_char256,"latency", PADDR(latency),
			PT_double,"reliability[pu]", PADDR(reliability),
			PT_double,"bitrate[b/s]",PADDR(bitrate),
			PT_double,"timeout[s]",PADDR(timeout),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);

		// set defaults
		strcpy(latency,"random.degenerate(0)"); // instant delivery always
		reliability=1.0; // 100% reliable delivery
		bitrate=0.0; // not rate limit
		timeout=60; // 1 minute timeout on message delivery
		defaults = this;
	}
}

/** Create a new object */
int comm::create()
{
	// copy the defaults
	memcpy(this,defaults,sizeof(comm));

	// set object specific defaults
	return 1;
}

/** Initialize the new object */
int comm::init(OBJECT *parent)
{
	char rt[33];
	if (sscanf(latency.get_string(),"%32[^(](%lf,%lf)",rt,&a,&b)==3)
		rtype = gl_randomtype(rt);
	else if (sscanf(latency.get_string(),"%32[^(](%f)",rt,&a)==2)
	{
		b=0;
		rtype = gl_randomtype(rt);
	}
	else
	{
		gl_error("latency distribution '%s' is not valid", latency.get_string());
		return 0;
	}
	
	return 1; // return 0 on failure
}

/** Synchronize the object */
TIMESTAMP comm::sync(TIMESTAMP t0)
{
	if (queue.is_empty())
		return TS_NEVER;
	Message *msg=NULL;
	while ((msg=queue.next(msg))!=NULL)
	{
		if (msg->get_deliverytime()<=t0)
		{
			DATETIME dt;
			char ts[64]="unknown time";
			gl_localtime(t0,&dt);
			gl_strtime(&dt,ts,sizeof(ts));
			gl_verbose("message %"FMT_INT64"d delivered at %s", msg->get_id(), ts);
			queue.take(msg)->get_dst()->deliver(msg);
		}
		else if (msg->get_deliverytime()<=t0+timeout*TS_SECOND)
		{
			DATETIME dt;
			char ts[64];
			gl_localtime(t0,&dt);
			gl_strtime(&dt,ts,sizeof(ts));
			gl_warning("message %"FMT_INT64"d delivery timeout at %s", msg->get_id(), ts);
			msg = queue.drop(msg,true);
		}
	}
	return queue.next_delivery_time(); // return TS_ZERO on failure, or TIMESTAMP for pending event
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////
EXPORT int create_comm(OBJECT **obj, OBJECT *parent)
{
	try
	{
		*obj = gl_create_object(comm::oclass);
		if (*obj!=NULL)
			return OBJECTDATA(*obj,comm)->create();
		else
			return 0;
	}
	CREATE_CATCHALL(comm);
}

EXPORT int init_comm(OBJECT *obj)
{
	try
	{
		if (obj!=NULL)
			return OBJECTDATA(obj,comm)->init(obj->parent);
		else
			return 0;
	}
	INIT_CATCHALL(comm);
}

EXPORT TIMESTAMP sync_comm(OBJECT *obj, TIMESTAMP t0)
{
	try
	{
		TIMESTAMP t1 = OBJECTDATA(obj,comm)->sync(t0);
		obj->clock = t0;
		return t1;
	}
	SYNC_CATCHALL(comm);
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF COMM LINKAGE
//////////////////////////////////////////////////////////////////////////

/**@}*/
