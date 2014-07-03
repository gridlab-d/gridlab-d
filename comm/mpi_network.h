/** $Id: mpi_network.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file mpi_network.h
	@addtogroup network

 @{
 **/

//#ifndef USE_MPI
//#define USE_MPI
//#endif

#ifdef USE_MPI
#ifndef _MPI_NETWORK_CLASS_H
#define _MPI_NETWORK_CLASS_H

#include "mpi.h"

#include "comm.h"

class mpi_network
{
public:
	int64	interval;
	int64	reply_time;
	int32	mpi_target;
private:
	TIMESTAMP last_time, next_time, their_time;
public:
	static CLASS *oclass;
	mpi_network(MODULE *mod);
	int create();
	int init(OBJECT *parent);
	int isa(char *classname);
	int notify(int, PROPERTY *);
	TIMESTAMP sync(TIMESTAMP t0, TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
};

#endif // _NETWORK_CLASS_H
#endif // USE_MPI
/**@}**/
