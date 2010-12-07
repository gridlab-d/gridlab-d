/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_CLASS_H
#define _NETWORK_CLASS_H

#include "comm.h"

typedef enum {
	QR_REJECT,
	QR_QUEUE
} QUEUERESOLUTION;

class network
{
public:
	double latency;
	char32 latency_mode;
	double latency_period;
	double latency_arg1;
	double latency_arg2;
	double bandwidth;
	QUEUERESOLUTION queue_resolution;
	double buffer_size;
	double timeout;
	double bandwidth_used;
private:
	RANDOMTYPE random_type;
	TIMESTAMP latency_last_update;

	void update_latency();
public:
	static CLASS *oclass;
	network(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int commit();
};

#endif // _NETWORK_CLASS_H

/**@}**/
