/** $Id: network_message.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_message.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_MESSAGE_H
#define _NETWORK_MESSAGE_H

#include "comm.h"
#include "network_interface.h"

typedef enum {
	NMS_NONE = 0,
	NMS_PENDING = 1,
	NMS_SENDING = 2,
	NMS_DELIVERED = 3,
	NMS_REJECTED = 4,
	NMS_FAILED = 5,
	NMS_ERROR = 6,
	NMS_DELAYED = 7,
	NMS_TXRX = 8,
	NMS_LATENT = 9,
} MSGSTATE;

class network_interface;

class network_message {
public:
	OBJECT *from;
	OBJECT *to;
	double size;
	char1024 message;
	int16 buffer_size;
	int64 start_time;
	int64 end_time;
	double start_time_frac;
	double end_time_frac;
	double bytes_sent;
	double bytes_resent;
	double bytes_recv;
	double bytes_buffered; // bytes caught up in a buffered network's router storage spaces
	MSGSTATE msg_state;
	// second round of timing, for latency
	double latency;
	int64	tx_start_sec;
	double	tx_start_frac;
	int64	rx_start_sec;
	double	rx_start_frac;
	int64	tx_done_sec;
//	double	tx_done_frac;
	int64	rx_done_sec;
//	double	rx_done_frac;
	int64	tx_est;
	int64	rx_est;
	TIMESTAMP last_update;


	// could be private with Network & NI friended, package-level privacy
	network_message *next, *prev;
	network_interface *pTo, *pFrom;

	int send_message(network_interface *nif, TIMESTAMP ts, double latency);
	int send_message(OBJECT *f, OBJECT *t, char *m, int len, double sz, TIMESTAMP ts, double frac, double latency);

	double send_rate;
	double recv_rate;
	double buffer_rate;
public:
	static CLASS *oclass;
	network_message();
	network_message(MODULE *mod);
	int create();
	int isa(char *classname);
	int notify(int, PROPERTY *);

};

#endif // _NETWORK_MESSAGE_H