/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.cpp
	@addtogroup network Performance network object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* network::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
network::network(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"network",sizeof(network),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			//PT_loadshape,"shape",PADDR(shape),
			PT_double, "latency[s]", PADDR(latency),
			PT_char32, "latency_mode", PADDR(latency_mode),
			PT_double, "latency_period[s]", PADDR(latency_period),
			PT_double, "latency_arg1", PADDR(latency_arg1),
			PT_double, "latency_arg2", PADDR(latency_arg2),
			PT_double, "bandwidth[MB/s]", PADDR(bandwidth),
			PT_enumeration, "queue_resolution", PADDR(queue_resolution),
				PT_KEYWORD, "REJECT", QR_REJECT,
				PT_KEYWORD, "QUEUE", QR_QUEUE,
			PT_double, "buffer_size[MB]", PADDR(buffer_size),
			PT_double, "bandwidth_used[MB/s]", PADDR(bandwidth_used), PT_ACCESS, PA_REFERENCE,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int network::create() 
{
	random_type = RT_INVALID;
	return 1;
}

void network::update_latency(){
	// NOTE: latency_period == 0 means that the latency will update every iteration, even though it is processed during 'commit'.
//	if(latency_last_update + latency_period <= gl_globalclock){
		if(random_type != RT_INVALID){
			latency = gl_randomvalue(random_type, latency_arg1, latency_arg2);
			latency_last_update = gl_globalclock;
			latency_next_update = gl_globalclock + (TIMESTAMP)latency_period;
		} else {
			latency_next_update = TS_NEVER; // latency does not use a distribution and does not require updating
		}
		if(latency < 0.0){
			gl_warning("random latency output of %f is reset to zero", latency);
		}
//	}
}

int network::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	// input validation checks
	if(latency_mode[0] != 0){
		random_type = gl_randomtype(latency_mode);
		if(random_type == RT_INVALID){
			GL_THROW("unrecognized random type \'%s\'", latency_mode);
		}
	} else {
		random_type = RT_INVALID; // prevents update_latency from updating the value
	}
	if(latency_period < 0.0){
		gl_warning("negative latency_period was reset to zero");
		latency_period = 0.0;
	}

	if(bandwidth <= 0.0){
		gl_error("non-positive bandwidth");
		return 0;
	}
	switch(queue_resolution){
		case QR_QUEUE:
			if(buffer_size <= 0.0){
				gl_error("non-positive buffer_size when using buffered queue_resolution");
			}
			break;
		case QR_REJECT:
			// requires no checks
			break;
		default:
			gl_error("unrecognized queue_resolution, defaulting to QR_REJECT");
			queue_resolution = QR_REJECT;
			break;
	}
	if(timeout < 0.0){
		gl_warning("negative timeout was reset to zero");
		timeout = 0.0;
	}
	// operational variable defaults
	bandwidth_used = 0.0;
	update_latency();
	latency_last_update = gl_globalclock;
	next_event = TS_NEVER;
	// success
	return 1;
}

int network::isa(char *classname){
	return strcmp(classname,"network")==0;
}

TIMESTAMP network::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	// update latency?
	if(latency_last_update + latency_period <= t1){
		update_latency();
	}
	if ((latency_period <= 0.001) || 
		(next_event != TS_NEVER) && (next_event < latency_next_update) ){
		return next_event;
	} else {
		return latency_next_update; 
	}
}

int network::attach(network_interface *nif){
	if(nif == 0){
		return 0;
	}
	if(first_if == 0){
		first_if = nif;
		last_if = nif;
		nif->next = 0;
	} else {
		last_if->next = nif;
		last_if = nif;
		nif->next = 0;
	}
	
	return 1;
}

//EXPORT int commit_network(OBJECT *obj){
TIMESTAMP network::commit(TIMESTAMP t1, TIMESTAMP t2){
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t0 = obj->clock;
	double net_bw = 0.0;
	double nif_bw = 0.0;
	double net_bw_used = 0.0;	// tallies how much network bandwidth has been used thus far in practice
	network_message *netmsg = 0;

	// prompt interfaces to check for data to send
	network_interface *nif = first_if;
	for(; nif != 0; nif = nif->next){
		nif->check_buffer();
	}

	//since we can't sync, calculate actual exchanges here
	for(nif = first_if; nif != 0; nif = nif->next){
		if(nif->has_outbound()){
			for(netmsg = nif->peek_outbox(); netmsg != 0; netmsg = netmsg->next){
				// increment message & interface counters
				double dt = (double)(gl_globalclock - netmsg->last_update);
				netmsg->bytes_sent += netmsg->send_rate * dt; // SR may be zero
				netmsg->bytes_buffered += netmsg->buffer_rate * dt; // BR may be zero
				netmsg->bytes_recv += netmsg->send_rate * dt;
				netmsg->last_update = gl_globalclock;
			}
		}
	}

	// scan interfaces for messages, determine network bandwidth requirements
	// not O(n^2) but limited to the count of messages that exist, could iterate on those iff messages were stored in a list
	// THIS SHOULD NOT USE nif->send_rate_used OR nif->recv_rate_used!! at the moment, this is
	//	making some guesses on how much bandwidth is wanted to send all the messages on the network
	for(nif = first_if; nif != 0; nif = nif->next){
		nif_bw = 0.0;
		nif->bandwidth_used = 0.0;
		nif->send_rate_used = 0;
		if(nif->has_outbound()){
			// check if interface is designed to send more than one message per second
			for(netmsg = nif->peek_outbox(); netmsg != 0; netmsg = netmsg->next){
				// note: if applicable, would check rx's bandwidth to limit tx's sent rate
				// constrain by interface send rate
				if(nif_bw + netmsg->size - netmsg->bytes_sent > nif->send_rate){
					nif_bw = nif->send_rate; // unable to send all the existing message AND the rest of this message
					break;
				} else {
					nif_bw += netmsg->size - netmsg->bytes_sent; // try to send the rest of this message
				}
			}
			net_bw += nif_bw;
		}
	}

	// determine what the network does with its traffic loading
	// meant as a mental exercise.  remove this. -mh
/*	if(net_bw > bandwidth){
		switch(queue_resolution){
			case QR_REJECT:
				bandwidth_used = 0;
				bytes_rejected = net_bw - bandwidth; // for reference, how far over we are
				break;
			case QR_QUEUE:
				bandwidth_used = bandwidth;
				break;
			case QR_SCALE:
				gl_error("communication network does not yet support interface transmission scaling");
				return 0;
			default:
				break;
		}
	} else {
		;
	}*/

	// "send" data
	// operate on the messages
	this->bandwidth_used = 0;
	for(nif = first_if; nif != 0; nif = nif->next){
		if(nif->has_outbound()){
			nif->bandwidth_used = 0;
			for(netmsg = nif->peek_outbox(); netmsg != 0; netmsg = netmsg->next){
				// note: if applicable, would check rx's bandwidth to limit tx's sent rate.  may involve another pass to pre-determine nif transmission constraints
				// determine how much is being sent
				netmsg->send_rate = 0.0;
				netmsg->buffer_rate = 0.0;
				switch(queue_resolution){
					// just needs to update send_rate and msg_state
					case QR_REJECT:
						// calculate available bandwidth for the message, if any
						if(bandwidth - bandwidth_used > nif->send_rate - nif->send_rate_used){
							// send
							netmsg->send_rate = nif->send_rate - nif->send_rate_used;
							netmsg->msg_state = NMS_SENDING;
						} else { // resolve congestion
							if(bandwidth == net_bw_used){
								// out of bandwidth, start rejecting
								netmsg->send_rate = 0;
								if(netmsg->bytes_sent > 0){
									netmsg->msg_state = NMS_DELAYED;
								} else {
									netmsg->msg_state = NMS_REJECTED;
								}
							} else {
								// last few bytes/sec, make 'em count
								netmsg->send_rate = bandwidth - bandwidth_used; // already know is l.t. NIF BW
								netmsg->msg_state = NMS_SENDING;
							}
						}
						break;
					case QR_QUEUE:
						if(bandwidth - bandwidth_used > nif->send_rate - nif->send_rate_used){
							// send
							netmsg->send_rate = nif->send_rate - nif->send_rate_used;
							netmsg->msg_state = NMS_SENDING;
						} else { // resolve congestion
							if(bandwidth == bandwidth_used && buffer_size == buffer_used){
								// out of bandwidth and full buffer, start rejecting
								netmsg->send_rate = 0;
								if(netmsg->bytes_sent > 0){
									netmsg->msg_state = NMS_DELAYED;
								} else {
									netmsg->msg_state = NMS_REJECTED;
								}
							} else {
								// use remain bandwidth (if any), buffer remainder
								netmsg->send_rate = bandwidth - bandwidth_used; // already know is l.t. NIF BW
								netmsg->buffer_rate = nif->send_rate - nif->send_rate_used - netmsg->send_rate;
								netmsg->msg_state = NMS_SENDING;
							}
						}
						break;
					case QR_SCALE:
						gl_error("communication network does not yet support interface transmission scaling");
						return 0;
					default:
						gl_error("unrecognized communication resolution mode");
						return 0;
				}
				// limit send_rate and buffer_rate based on remaining bytes to send
				double rmn = netmsg->size - netmsg->bytes_sent;
				if(rmn < netmsg->send_rate + netmsg->buffer_rate){
					if(rmn < netmsg->send_rate){ // just in the bandwidth
						netmsg->send_rate = rmn;
						netmsg->buffer_rate = 0.0;
					} else {
						netmsg->buffer_rate = rmn - netmsg->send_rate;
					}
				}
				// netmsg->bytes_buffered = 
				nif->bandwidth_used += netmsg->send_rate;
				nif->bandwidth_used += netmsg->buffer_rate;
				bandwidth_used += netmsg->send_rate;
				buffer_used += netmsg->buffer_rate;
			}
		}
	}

	// if there's additional bandwidth, use it to unload the buffer back into the network
	if(bandwidth_used < bandwidth){
		double bandwidth_left = bandwidth - bandwidth_used;
		if(buffer_used > 0){
			for(nif = first_if; nif != 0 && bandwidth_left > 0.0; nif = nif->next){
				if(nif->has_outbound()){
					//	for each message,
					for(netmsg = nif->peek_outbox(); netmsg != 0; netmsg = netmsg->next){
						if(netmsg->bytes_buffered > 0.0){
							double dt = (double)(netmsg->last_update - gl_globalclock);
							if(netmsg->bytes_buffered > bandwidth_left){
								netmsg->buffer_rate = -bandwidth_left;
							} else {
								netmsg->buffer_rate = -netmsg->bytes_buffered;
							}
							netmsg->bytes_buffered += netmsg->buffer_rate * dt;
							netmsg->bytes_recv -= netmsg->buffer_rate * dt;
							bandwidth_left += netmsg->bytes_buffered;
							bandwidth_used -= netmsg->buffer_rate;		// netmsg->buffer_rate
							netmsg->send_rate -= netmsg->buffer_rate;	//	is negative
						}
					}
				}
			}
		}
	}

	/* don't forget to "receive" the messages!!! */
	network_message tempmsg;
	TIMESTAMP rv = TS_NEVER;
	double predicted_tx_sec = 0.0;
	TIMESTAMP next_tx_sec = 0;
	for(nif = first_if; nif != 0; nif = nif->next){
		if(nif->has_outbound()){
			//	for each message,
			for(netmsg = nif->peek_outbox(); netmsg != 0; netmsg = netmsg->next){
				/*** NOTE: THIS MUST TAKE DT INTO EFFECT ***/
				if(netmsg->bytes_recv >= netmsg->size){
					tempmsg.next = netmsg->next;
					netmsg->msg_state = NMS_DELIVERED;
					netmsg->rx_done_sec = gl_globalclock + (int64)ceil(latency);
					netmsg->end_time = gl_globalclock + (int64)ceil(latency);
					if(netmsg->end_time < rv)
						rv = netmsg->end_time;
//					memcpy(netmsg->pTo->data_buffer, netmsg->message, netmsg->buffer_size);
					// move netmsg from this nif's outbox to the other nif's inbox					
					// unlink netmsg from the list that it's in
					if(netmsg->prev != 0)
						netmsg->prev->next = netmsg->next;
					if(tempmsg.next != 0)
						tempmsg.next->prev = netmsg->prev;
					if(nif->outbox == netmsg){ // make sure we pull this off the outbox, if it's the first one there
						nif->outbox = tempmsg.next;
					}
					/* NOTE: puts these into the inbox in reverse order */
					// link netmsg to the list it's going to
					netmsg->next = netmsg->pTo->inbox;
					if(netmsg->next != 0)
						netmsg->next->prev = netmsg;
					netmsg->pTo->inbox = netmsg;
					// use a dummy msg to hold 'next'
					netmsg = &tempmsg;
				} else {
					// recalculate estimated transmission time
					if(netmsg->send_rate > 0.0){
						predicted_tx_sec = (netmsg->size - netmsg->bytes_recv) / netmsg->send_rate;
						if(predicted_tx_sec < 1.0){
							// run next step
							next_tx_sec = gl_globalclock + 1; // will finish next second
						} else {
							// send rate will change when the tail of the message is sent
							next_tx_sec = (int64)floor(predicted_tx_sec) + gl_globalclock;
						}
						if(next_tx_sec < rv){
							rv = next_tx_sec;
						} // if less than 1s to transmit, it'll wrap up in 1s
					}
					// no 'else', wait for other messages to free up bandwidth
				}
			}
		}
	}

	// moved to nif->presync()
/*
	TIMESTAMP tv = TS_NEVER;
	for(nif = first_if; nif != 0; nif = nif->next){
		if(nif->has_inbound()){
			tv = nif->handle_inbox();
			if(tv < rv){
				rv = tv;
			}
		}
	}
*/

	return rv;
}

//return my->notify(update_mode, prop);
int network::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_network(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(network::oclass);
	if (*obj!=NULL)
	{
		network *my = OBJECTDATA(*obj,network);
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

EXPORT int init_network(OBJECT *obj)
{
	network *my = OBJECTDATA(obj,network);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_network(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,network)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_network(OBJECT *obj, TIMESTAMP t1)
{
	network *my = OBJECTDATA(obj,network);
	try {
		TIMESTAMP t2 = my->sync(obj->clock, t1);
		//obj->clock = t1; // update in commit
		return t2;
	}
	catch (char *msg)
	{
		DATETIME dt;
		char ts[64];
		gl_localtime(t1,&dt);
		gl_strtime(&dt,ts,sizeof(ts));
		gl_error("%s::%s.init(OBJECT **obj={name='%s', id=%d},TIMESTAMP t1='%s'): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, ts, msg);
		return 0;
	}
}

EXPORT TIMESTAMP commit_network(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	network *my = OBJECTDATA(obj,network);
	TIMESTAMP rv = my->commit(t1, t2);
	obj->clock = gl_globalclock;
	return rv;
}

EXPORT int notify_network(OBJECT *obj, int update_mode, PROPERTY *prop){
	network *my = OBJECTDATA(obj,network);
	return my->notify(update_mode, prop);
}
/**@}**/
