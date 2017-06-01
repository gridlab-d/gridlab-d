/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.cpp
	@addtogroup mpi_network MPI network object controller
	@ingroup comm

 @{
 **/

//#ifndef USE_MPI
//#define USE_MPI
//#endif

#ifdef USE_MPI

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "mpi_network.h"


//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* mpi_network::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
mpi_network::mpi_network(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"mpi_network",sizeof(mpi_network),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the mpi_network class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_int64, "interval", PADDR(interval),
			PT_int32, "mpi_target", PADDR(mpi_target),
			PT_int64, "reply_time", PADDR(reply_time), PT_ACCESS, PA_REFERENCE,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int mpi_network::create() 
{
	mpi_target = 1;
	interval = 60;
	reply_time = last_time = next_time = their_time = 0;
	return 1;
}

int mpi_network::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);

	// input validation checks
	if(mpi_target < 0){
		gl_error("mpi_network init(): target is negative");
		return 0;
	}
	if(mpi_target == 0){
		gl_error("mpi_network init(): target is 0, which is assumed to be GridLAB-D");
		return 0;
	}
	
	if(interval < 1){
		gl_error("mpi_network init(): interval is not greater than zero");
		return 0;
	}

	// success
	return 1;
}

int mpi_network::isa(char *classname){
	return strcmp(classname,"mpi_network")==0;
}

TIMESTAMP mpi_network::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	MPI_Status status;
	int rv = 0;

	// first iteration for this object?
	if(t0 == 0){
		last_time = next_time = t1;
	}
	
	if(next_time >= t1){
		last_time = t1;
		rv = MPI_Send(&last_time, sizeof(last_time), MPI_LONG_LONG, mpi_target, 0, MPI_COMM_WORLD);
		next_time += interval;
	}
	if(rv == 0){ // not sure about error-checking this
		;
	}

	rv = MPI_Recv(&their_time, sizeof(their_time), MPI_LONG_LONG, mpi_target, 0, MPI_COMM_WORLD, &status);
	if(rv == 0){ // not sure about error-checking this
		;
	}
	if(their_time != last_time){
		gl_error("mpi_network::sync(): epic fail due to mismatched timestamp");
	}
	reply_time = their_time;
	return next_time;
}

//EXPORT int commit_network(OBJECT *obj){
TIMESTAMP mpi_network::commit(TIMESTAMP t1, TIMESTAMP t2){
	OBJECT *obj = OBJECTHDR(this);
	TIMESTAMP t0 = obj->clock;

	return TS_NEVER;
}

//return my->notify(update_mode, prop);
int mpi_network::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_mpi_network(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(mpi_network::oclass);
	if (*obj!=NULL)
	{
		mpi_network *my = OBJECTDATA(*obj,mpi_network);
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

EXPORT int init_mpi_network(OBJECT *obj)
{
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_mpi_network(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,mpi_network)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_mpi_network(OBJECT *obj, TIMESTAMP t1)
{
	mpi_network *my = OBJECTDATA(obj,mpi_network);
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

EXPORT TIMESTAMP commit_mpi_network(OBJECT *obj, TIMESTAMP t1, TIMESTAMP t2){
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	TIMESTAMP rv = my->commit(t1, t2);
	obj->clock = gl_globalclock;
	return rv;
}

EXPORT int notify_mpi_network(OBJECT *obj, int update_mode, PROPERTY *prop){
	mpi_network *my = OBJECTDATA(obj,mpi_network);
	return my->notify(update_mode, prop);
}

#endif
// USE_MPI
/**@}**/
