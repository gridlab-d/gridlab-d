/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_message.cpp
	@addtogroup network_message Performance network_message object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include "network_message.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* network_message::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
network_message::network_message(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"network_message",sizeof(network_message),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_message class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_object, "from", PADDR(from), PT_DESCRIPTION, "the network interface this message is being sent from",
			PT_object, "to", PADDR(to), PT_DESCRIPTION, "the network interface this message is being sent to",
			PT_double, "size[MB]", PADDR(size), PT_DESCRIPTION, "the number of bytes being sent across the network object",
			PT_char1024, "message", PADDR(message), PT_DESCRIPTION, "the contents of the data being sent, whether binary, ASCII, or faked",
			PT_int16, "buffer_size", PADDR(buffer_size), PT_DESCRIPTION, "number of bytes held within the message buffer",
			PT_int64, "start_time", PADDR(start_time), PT_DESCRIPTION, "the simulation time that the message started to be sent",
			PT_int64, "end_time", PADDR(end_time), PT_DESCRIPTION, "the simulation time that the message finished sending",
			PT_double, "start_time_frac[s]", PADDR(start_time_frac), PT_DESCRIPTION, "the subsecond time that the message started to be sent, based on the sending network interface",
			PT_double, "end_time_frac[s]", PADDR(end_time_frac), PT_DESCRIPTION, "the subsecond time that the message finished sending, based on the receiving network interface",
			PT_double, "bytes_sent[MB]", PADDR(bytes_sent), PT_DESCRIPTION, "the number of bytes sent by the transmitting interface",
			PT_double, "bytes_resent[MB]", PADDR(bytes_resent), PT_DESCRIPTION, "the number of bytes that had to be resent across the network",
			PT_double, "byte_received[MB]", PADDR(bytes_recv),  PT_DESCRIPTION, "the number of bytes received by the receiving interface",
			PT_enumeration, "msg_state", PADDR(msg_state), PT_DESCRIPTION, "the current state of the message",
				PT_KEYWORD, "NONE", NMS_NONE,
				PT_KEYWORD, "PENDING", NMS_PENDING,
				PT_KEYWORD, "SENDING", NMS_SENDING,
				PT_KEYWORD, "DELIVERED", NMS_DELIVERED,
				PT_KEYWORD, "REJECTED", NMS_REJECTED,
				PT_KEYWORD, "FAILED", NMS_FAILED,
				PT_KEYWORD, "ERROR", NMS_ERROR,
				PT_KEYWORD, "DELAYED", NMS_DELAYED,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_message properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

network_message::network_message(){
	from = to = 0;
	size = buffer_size = 0;
	bytes_sent = bytes_resent = bytes_recv = 0;
	memset(message, 0, 1024);
	start_time = end_time = -1;
	start_time_frac = end_time_frac = 0.0;
}

int network_message::send_message(network_interface *nif, TIMESTAMP ts, double latency){
	return send_message(OBJECTHDR(nif), nif->destination, nif->data_buffer, nif->buffer_size, 
		nif->size, ts, nif->bandwidth_used/nif->send_rate, latency);
}

int network_message::send_message(OBJECT *f, OBJECT *t, char *m, int len, double sz, TIMESTAMP ts, double frac, double latency){
	char name_buf[64];
	from = f;
	to = t;
	size = sz;
	buffer_size = len;
	memcpy(message, m, len);
	// old
	start_time = ts;
	start_time_frac = frac;
	// new
	tx_start_sec = ts;
	tx_start_frac = frac;
	rx_start_sec = ts+floor(latency+frac);
	rx_start_frac = fmod(latency+frac, 1.0);
	// /new
	msg_state = NMS_NONE;
	last_update = ts;
	// validate to & from interfaces
	if(from == 0){
		gl_error("send_message(): no 'from' interface");
		msg_state = NMS_ERROR;
		return 0;
	} else if(gl_object_isa(from, "network_interface")){
		pFrom = OBJECTDATA(from, network_interface);
		switch(pFrom->duplex_mode){
			case DXM_FULL_DUPLEX:
			case DXM_HALF_DUPLEX:
			case DXM_XMIT_ONLY:
				break;
			default:
				gl_error("send_message(): 'from' interface '%s' not configured to send messages", gl_name(from, name_buf, 63));
				msg_state = NMS_ERROR;
				return 0;
		}
	} else {
		gl_error("send_message(): 'from' object is not a network_interface");
		msg_state = NMS_ERROR;
		return 0;
	}
	
	if(to == 0){
		gl_error("send_message(): no 'to' interface");
		msg_state = NMS_ERROR;
		return 0;
	} else if(gl_object_isa(to, "network_interface")){
		pTo = OBJECTDATA(to, network_interface);
		switch(pTo->duplex_mode){
			case DXM_FULL_DUPLEX:
			case DXM_HALF_DUPLEX:
			case DXM_RECV_ONLY:
				break;
			default:
				gl_error("send_message(): 'to' interface '%s' not configured to receive messages", gl_name(to, name_buf, 63));
				msg_state = NMS_ERROR;
				return 0;
		}
	} else {
		gl_error("send_message(): 'to' object is not a network_interface");
		msg_state = NMS_ERROR;
		return 0;
	}
	if(sz < 0){
		gl_error("send_message(): size is negative");
		msg_state = NMS_ERROR;
		return 0;
	}
	if(len < 0){
		gl_error("send_message(): length is negative");
		msg_state = NMS_ERROR;
		return 0;
	}
	if(ts < 0){
		gl_error("send_message(): timestamp is nonsensical");
		msg_state = NMS_ERROR;
		return 0;
	}
	if(pTo->pNetwork != pFrom->pNetwork){
		gl_error("send_message(): network interfaces exist on different networks");
		msg_state = NMS_ERROR;
		return 0;
	}
	return 1; // success
}

// create is called every time a new object is loaded
int network_message::create() 
{
	gl_error("network_message is an internally used class and should not be instantiated by a model");
	return 0;
}

int network_message::isa(char *classname){
	return strcmp(classname,"network_message")==0;
}

int network_message::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_network_message(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(network_message::oclass);
	if (*obj!=NULL)
	{
		network_message *my = OBJECTDATA(*obj,network_message);
		gl_set_parent(*obj,parent);
		try {
			my->create();
		}
		catch (char *msg)
		{
			gl_error("%s::%s.create(OBJECT **obj={name='%s', id=%d},...): %s", (*obj)->oclass->module->name, (*obj)->oclass->name, (*obj)->name, (*obj)->id, msg);
			return 0;
		}
		return 1;
	}
	return 0;
}

EXPORT TIMESTAMP sync_network_message(OBJECT *obj, TIMESTAMP t1, PASSCONFIG pass){
	obj->clock = t1;
	return TS_NEVER;
}

EXPORT int isa_network_message(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,network_message)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT int notify_network_message(OBJECT *obj, int update_mode, PROPERTY *prop){
	network_message *my = OBJECTDATA(obj,network_message);
	return my->notify(update_mode, prop);
}
/**@}**/
