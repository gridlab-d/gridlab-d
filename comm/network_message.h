/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_message.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_MESSAGE_H
#define _NETWORK_MESSAGE_H

#include "comm.h"
#include "network.h"

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

	int send_message(OBJECT *f, OBJECT *t, char *m, int sz, TIMESTAMP ts, double frac);
public:
	static CLASS *oclass;
	network_message();
	network_message(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int commit();
};

#endif // _NETWORK_MESSAGE_H