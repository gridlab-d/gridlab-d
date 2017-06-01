/** $Id: network.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_CLASS_H
#define _NETWORK_CLASS_H

#include "comm.h"
#include "network_message.h"
#include "network_interface.h"

typedef enum {
	QR_REJECT,		// once bandwidth capacity is met, additional messages are rejected
	QR_QUEUE,		// once bandwidth capacity is met, edge routers buffer messages before transmitting across the network
	QR_SCALE, /*x*/	// once bandwidth capacity is met, transmitters are scaled back so as to not exceed network capacity
} QUEUERESOLUTION;

// future idea
typedef enum {
	TXM_SIMPLE,		// data is sent and received as fast as it can be
	TXM_BALANCE		// the sender will not outpace the receiver
} TXMODE;

// future idea
typedef enum{
	MXM_FIFO,		// the messages are sent first-in first out and will block additional messages
	MXM_BALANCE,	// messages are sent at a rate proportional to the receiver's bandwidth
	MXM_CASCADE		// messages are sent FIFO, but if Rx limits Tx, additional messages may try to be sent
} MXMODE;

class network_message;
class network_interface;

class network
{
public:
	double latency;
	char32 latency_mode;
	double latency_period;
	double latency_arg1;
	double latency_arg2;
	double bandwidth;		// maximum kbytes/sec that can be fed through the network
	QUEUERESOLUTION queue_resolution; // determines what the network does when the sum of the interface traffic wants to exceed the bandwidth
	double buffer_size;		// how many bytes can be stored 'in the network' when traffic exceeds network bandwidth
	double buffer_used;		// how many bytes are stored by edge-routers before streaming across the network
	double timeout;
	double bandwidth_used;

	double bytes_rejected;	// how much bandwidth was rejected by the network
	double bandwidth_scale;	
private:
	RANDOMTYPE random_type;
	TIMESTAMP latency_last_update;
	TIMESTAMP latency_next_update;
	TIMESTAMP next_event;

	network_message *first_msg, *last_msg;
	network_interface *first_if, *last_if;
	double bytes_to_reject;

	void update_latency();
public:
	static CLASS *oclass;
	network(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int attach(network_interface *nif);
};

#endif // _NETWORK_CLASS_H

/**@}**/
