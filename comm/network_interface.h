/** $Id$
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_interface.h
	@addtogroup network

 @{
 **/

#ifndef _NETWORK_INTERFACE_H
#define _NETWORK_INTERFACE_H

#include "comm.h"
#include "network.h"

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

class network_interface
{
public:
	DUPLEXMODE duplex_mode;
	OBJECT *to;
	double size;
	double transfer_rate;
	NISTATUS status;
	double bandwidth_used;
	char1024 data_buffer;
	int32 buffer_size;
private:
	network *pNetwork;
public:
	static CLASS *oclass;
	network_interface(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	int commit();
};

#endif // _NETWORK_INTERFACE_H
