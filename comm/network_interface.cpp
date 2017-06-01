/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_interface.cpp
	@addtogroup network_interface Performance network_interface object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network_interface.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* network_interface::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
network_interface::network_interface(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"network_interface",sizeof(network_interface),PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_interface class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration, "duplex_mode", PADDR(duplex_mode),
				PT_KEYWORD, "FULL_DUPLEX", DXM_FULL_DUPLEX,
				PT_KEYWORD, "HALF_DUPLEX", DXM_HALF_DUPLEX,
				PT_KEYWORD, "TRANSMIT_ONLY", DXM_XMIT_ONLY,
				PT_KEYWORD, "RECEIVE_ONLY", DXM_RECV_ONLY,
			PT_object, "to", PADDR(to),
			PT_object, "destination", PADDR(destination),
			PT_double, "size[MB]", PADDR(size),
			PT_double, "send_rate[MB/s]", PADDR(send_rate),
			PT_double, "recv_rate[MB/s]", PADDR(recv_rate),
			PT_int64, "update_rate", PADDR(update_rate),
			PT_enumeration, "status", PADDR(status), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "NONE", NIS_NONE,
				PT_KEYWORD, "QUEUED", NIS_QUEUED,
				PT_KEYWORD, "REJECTED", NIS_REJECTED,
				PT_KEYWORD, "SENDING", NIS_SENDING,
				PT_KEYWORD, "COMPLETE", NIS_COMPLETE,
				PT_KEYWORD, "FAILED", NIS_FAILED,
			PT_double, "bandwidth_used[MB/s]", PADDR(bandwidth_used), PT_ACCESS, PA_REFERENCE,
			PT_double, "send_rate_used[MB/s]", PADDR(send_rate_used), PT_ACCESS, PA_REFERENCE,
			PT_double, "recv_rate_used[MB/s]", PADDR(recv_rate_used), PT_ACCESS, PA_REFERENCE,
			PT_int32, "buffer_size", PADDR(buffer_size),
			PT_char1024, "buffer", PADDR(data_buffer),
			PT_char32, "property", PADDR(prop_str), PT_DESCRIPTION, "the name of the property to write incoming messages to",
			PT_bool, "ignore_size", PADDR(ignore_size), PT_DESCRIPTION, "informs the model that the size of the target property may not represent the number of bytes available for writing",
			PT_enumeration, "send_mode", PADDR(send_message_mode), PT_DESCRIPTION, "determines when the interface will write the data buffer into a message and send it",
				PT_KEYWORD, "UPDATE", NSM_UPDATE,
				PT_KEYWORD, "INTERVAL", NSM_INTERVAL,
				PT_KEYWORD, "UPDATE_PERIOD", NSM_UPDATE_PERIOD,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_interface properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int network_interface::create() 
{
	return 1;
}

int network_interface::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	// input validation checks
	if(to == 0){
		GL_THROW("network interface is not connected to a network");
	}
	if(!gl_object_isa(to, "network", "comm")){
		GL_THROW("network interface is connected to a non-network object");
	} else {
		pNetwork = OBJECTDATA(to, network);
	}
	if(send_rate <= 0.0){
		GL_THROW("network interface has a non-positive send rate");
	}
	if(recv_rate <= 0.0){
		GL_THROW("network interface has a non-positive recv rate");
	}

	if(prop_str[0] == 0){
		GL_THROW("network interface does not specify data destination buffer");
	}
	target = gl_get_property(parent, prop_str);
	if(target == 0){
		GL_THROW("network interface parent does not contain property '%s'", prop_str);
	}
	// note that there is no type-checking on this: the target property can be a string, FP number, integer, or struct.
	//   all the interface does is copy data to where it's told.  note that this is "dangerous".
	// size-checking vs target->size may be useful at run-time, to prevent buffer overflows

	// operational variable defaults
	bandwidth_used = 0.0;
	//size = 0;

	//buffer_size = prev_buffer_size = 1024;
	if(ignore_size){
		prev_buffer_size = buffer_size;
	} else {
		buffer_size = prev_buffer_size = target->width;
	}
	pNetwork->attach(this);
	// success
	return 1;
}

int network_interface::isa(char *classname){
	return strcmp(classname,"network_interface")==0;
}

TIMESTAMP network_interface::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

TIMESTAMP network_interface::presync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP rv = TS_NEVER;
	if(has_inbound()){
		rv = handle_inbox(t1);
	}
	return rv; 
}

TIMESTAMP network_interface::postsync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}


/**
 * ni::commit() is where data is pulled from its parent, copied into the interface,
 *	and written to a message to be sent to another interface.
 */
TIMESTAMP network_interface::commit(TIMESTAMP t1, TIMESTAMP t2){
	return 1;
}

/*
//return my->notify(update_mode, prop);
int network_interface::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	if(update_mode == NM_POSTUPDATE){
		if(strcmp(prop->name, "buffer") == 0){
			// new data has been received
			// NOTE: network should update 'buffer_size' before updating 'buffer'
			// * lock parent's destination property
			// * memcpy(pTarget, data_buffer, buffer_size)
			// * memset(data_buffer, 0, buffer_size)
			// * buffer_size = 0;
			;
		}
	}
	return 1;
}*/

// there isn't a good way to set_value with straight binary data, so going about it using
//	direct sets, and 'notify'ing with a follow-up function call.
int network_interface::on_message(){
	return 0;
}

/* check if it's time to send a message and poll the parent's target property for updated data */
int network_interface::check_buffer(){
	OBJECT *obj = OBJECTHDR(this);
	write_msg = false;
	//void *a = OBJECTDATA((void),obj->parent);
	void *b = (target->addr);
	void *c = ((void *)((obj->parent)?((obj->parent)+1):NULL));
	void *d = (void *)((int64)c + (int64)b);
	bool time_for_update = (last_message_send_time + update_rate <= gl_globalclock);

	// do interval first, to avoid the memcmp on on update_period
	// check if we should retreive and write data
	if(duplex_mode == DXM_RECV_ONLY){
		return 1;
	}
	if((send_message_mode == NSM_INTERVAL || send_message_mode == NSM_UPDATE_PERIOD) && time_for_update){
		write_msg = true;
	} else if(send_message_mode == NSM_UPDATE || send_message_mode == NSM_UPDATE_PERIOD){ // send on update
		// do we have a base case?
		if(curr_buffer_size == -1){
			write_msg = true;
		} else {
			// do we have new data?
			if(0 == memcmp(data_buffer, (void *)(d), buffer_size)){
				write_msg = false; // continue, no need to send the same data
			} else {
				write_msg = true;
			}
		}
	}

	if(write_msg){
		curr_buffer_size = buffer_size;
//		memset(prev_data_buffer, 0, buffer_size);
//		memcpy(prev_data_buffer, data_buffer, buffer_size);
		memset(data_buffer, 0, buffer_size);
		memcpy(data_buffer, (void *)(d), buffer_size);
		//gl_set_value(obj, data_buffer, (char *)c, 0);
		network_message *nm = (network_message *)malloc(sizeof(network_message));
		memset(nm, 0, sizeof(network_message));
		nm->send_message(this, gl_globalclock, pNetwork->latency); // this hooks the message in to the network
		nm->next = outbox;
		outbox = nm;
		last_message_send_time = gl_globalclock;
	}

	return 1;
}

TIMESTAMP network_interface::handle_inbox(TIMESTAMP t1){
	// process the messages in the inbox, in FIFO fashion, even though it's been stacked up.
	network_message *rv = handle_inbox(t1, inbox);

	inbox = rv;
	next_msg_time = TS_NEVER;
	for(network_message *nm = inbox; nm != 0; nm = nm->next){
		if(nm->end_time < next_msg_time)
			next_msg_time = nm->end_time;
	}
	return next_msg_time;
}

/**
 *	nm		- the message being processed (can be null)
 *	return	- the next message in the stack that is not ready to be processed (punted for later)
 *
 *	handle_inbox recursively checks to see if a message in the inbox is ready to be received,
 *		based on its 'arrival time', a function of the transmission time and network latency.
 *		Although messages are arranged in a stack, the function traverses to the end of the
 *		stack and works its way back up, returning the new stack as it processes through
 *		the messages.  It is possible that not all the messages marked as 'delivered' are
 *		ready to be processed at this point in time.
 */
network_message *network_interface::handle_inbox(TIMESTAMP t1, network_message *nm){
	OBJECT *my = OBJECTHDR(this);
	network_message *rv = 0;
	if(nm != 0){ // it's been stacked, process in reverse order
		rv = handle_inbox(t1, nm->next);
		if (rv != nm->next){
			free(nm->next);
			nm->next = rv;
		}
	} else {
		return 0;
	}
	// update interface, copy from nm
	if(t1 >= nm->rx_done_sec){
		LOCK_OBJECT(my);
		this->curr_buffer_size = nm->buffer_size;
		memcpy(this->data_buffer, nm->message, nm->buffer_size);
		UNLOCK_OBJECT(my);
		// update interface's parent
		void *addr = (void *)((int64)((my->parent)+1)+(int64)target->addr);
		LOCK_OBJECT(my->parent);
		memcpy(addr, data_buffer, curr_buffer_size);
		UNLOCK_OBJECT(my->parent);
		return nm->next;
	} else {
		return nm;
	}
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_network_interface(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(network_interface::oclass);
	if (*obj!=NULL)
	{
		network_interface *my = OBJECTDATA(*obj,network_interface);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (const char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT int init_network_interface(OBJECT *obj)
{
	network_interface *my = OBJECTDATA(obj,network_interface);
	try {
		return my->init(obj->parent);
	}
	catch (const char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_network_interface(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,network_interface)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_network_interface(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass)
{
	network_interface *my = OBJECTDATA(obj,network_interface);
	try {
		TIMESTAMP t2 = TS_NEVER;
		if(pass == PC_BOTTOMUP){
			t2 = my->sync(obj->clock, t1);
		} else if(pass == PC_POSTTOPDOWN){
			;
		} else if(pass == PC_PRETOPDOWN){
			t2 = my->presync(obj->clock, t1);
		}
		obj->clock = t1;
		return t2;
	}
	catch (const char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT TIMESTAMP commit_network_interface(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	network_interface *my = OBJECTDATA(obj,network_interface);
	return my->commit(t1, t2);
}

//EXPORT int notify_network_interface(OBJECT *obj, int update_mode, PROPERTY *prop){
//	network_interface *my = OBJECTDATA(obj,network_interface);
//	return my->notify(update_mode, prop);
//}
/**@}**/
