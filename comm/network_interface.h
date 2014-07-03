/** $Id: network_interface.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_interface.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_INTERFACE_H
#define _NETWORK_INTERFACE_H

#include "comm.h"
#include "network.h"
#include "network_message.h"

typedef enum {
	DXM_FULL_DUPLEX,
	DXM_HALF_DUPLEX,
	DXM_XMIT_ONLY,
	DXM_RECV_ONLY
} DUPLEXMODE;

typedef enum{
	NIS_NONE,
	NIS_QUEUED,
	NIS_REJECTED,
	NIS_SENDING,
	NIS_COMPLETE,
	NIS_FAILED
} NISTATUS;

typedef enum{
	NSM_UPDATE,
	NSM_INTERVAL,
	NSM_UPDATE_PERIOD,
} SENDMESSAGE;

class network;
class network_message;

class network_interface
{
public:
	DUPLEXMODE duplex_mode;	// the mode the interface can send or receive in
	OBJECT *to;				// the network the interface sends across
	OBJECT *destination;	// the network interface to send the message to
	double size;			// virtual message size
	//double transfer_rate;	// virtual send/receive rate
	double send_rate;
	double recv_rate;
	double send_rate_used;
	double recv_rate_used;
	NISTATUS status;
	double bandwidth_used;	// combined up/down for the last second
	char1024 data_buffer;	// 
	int32 buffer_size;		// player-defined for byte i/o count
	char32 prop_str;		// where to r/w data to/from
	bool ignore_size;
	SENDMESSAGE send_message_mode;
	TIMESTAMP last_message_send_time;
	int64 update_rate;

	int32 curr_buffer_size;
	int32 prev_buffer_size;
	char1024 prev_data_buffer;

	PROPERTY *target;
	network_interface *next;
	network_message *inbox;
	network_message *outbox;
	network *pNetwork;
	TIMESTAMP next_msg_time;

private:
	bool write_msg;
public:
	static CLASS *oclass;
	network_interface(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	//int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

	int on_message();
	int check_buffer();
	TIMESTAMP handle_inbox(TIMESTAMP);
	network_message *handle_inbox(TIMESTAMP, network_message *);
	bool has_outbound(){return (outbox != 0);}		// returns true if there is a message being sent from this interface
	bool has_inbound(){return (inbox != 0);}		// returns true if there is a message completely received by this interface
	network_message *peek_outbox(){return outbox;}	// returns the first message in the outbox without modifying it
	network_message *peek_inbox(){return inbox;}	// returns the first message in the inbox without modifying it
};

#endif // _NETWORK_INTERFACE_H
