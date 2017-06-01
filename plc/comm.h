/** $Id: comm.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file comm.h
	@addtogroup plc_comm

	The PLC communications object implements message handling between PLC objects.
	Each communications networks is defined by a single object that handles all
	messages between PLC objects linked to it.  The network has the following
	properties
	- \p latency defines the message delivery latency (note this is a functional distribution).
	- \p reliability to define the probability with which a message is delivered at all, and
	- \p bitrate to define the data rate at which messages are delivered (longer messages take
	longer to be delivered if bitrate is non-zero),

 @{
 **/

#ifndef _COMM_H
#define _COMM_H

#include <stdarg.h>
#include "gridlabd.h"

class Message {
private:
	static unsigned int64 next_id;
	unsigned int64 id;
	class machine *src;
	class machine *dst;
	TIMESTAMP delivery_time; /**< scheduled delivery time of the message */
	size_t size; /**< size of the message */
	void *data; 
	Message *next, *prev; /**< next message (only used when in queue) */
public:
	Message(void *ptr, size_t len, class machine *from, class machine *to)
	{
		id = next_id++;
		if (len>0)
		{
			data = malloc((len/16+1)*16); // allocation in 16 byte increments
			memcpy(data,ptr,len+1);
		}
		else
			data = NULL;
		size=len;
		src=from;
		dst=to;
		delivery_time = TS_NEVER;
		next = prev = NULL;
	};
	~Message(void)
	{
		if (data!=NULL)
			free(data);
	}
	inline unsigned int64 get_id(void) const { return id;};
	inline void *get_data(void) const { return data;};
	inline size_t get_size(void) const { return size;};
	inline TIMESTAMP get_deliverytime(void) const { return delivery_time;};
	inline void set_deliverytime(TIMESTAMP dt) { delivery_time=dt;};
	inline class machine *get_src(void) const { return src;};
	inline class machine *get_dst(void) const { return dst;};
	inline Message *get_next(void) const { return next;};
	inline Message *get_prev(void) const { return prev;};
	inline void set_next(Message *msg) { next=msg;};
	inline void set_prev(Message *msg) { prev=msg;};
};

class Queue {
private:
	Message *first;
	Message *last;
public:
	Queue();
	~Queue();
	void add(Message *msg);
	Message *peek(void);
	Message *take(Message *msg=NULL);
	TIMESTAMP next_delivery_time(void);
	inline bool is_empty(void) { return first==NULL;};
	Message *next(Message *msg);
	Message *drop(Message *msg, bool reverse=false);
};

#include "machine.h"

class comm : public gld_object {
public:
	double bitrate;				///< bit rate of message delivery [b/s]
	char256 latency;			///< latency distribution (random functional)
	double reliability;			///< reliability of delivery [pu]
	double timeout;				///< message routing timeout [s]
private:
	Queue queue;
	RANDOMTYPE rtype;
	double a, b;
	TIMESTAMP next_delivery;
public:
	static CLASS *oclass;
	static comm* defaults;
public:
	comm(MODULE *mod);
	~comm(void);
	int create();
	int init(OBJECT *parent);
	TIMESTAMP sync(TIMESTAMP t0);
public:
	static void route(comm *net, Message *msg);
protected:
	int send(void *data, size_t size, machine *from, machine *to);
private:
	void route(Message *msg);
	friend class machine;
};

/**@}*/
#endif
