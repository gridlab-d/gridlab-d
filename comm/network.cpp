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
		} else {
			; // latency does not use a distribution and does not require updating
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
	// success
	return 1;
}

int network::isa(char *classname){
	return strcmp(classname,"network")==0;
}

TIMESTAMP network::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

//EXPORT int commit_network(OBJECT *obj){
int network::commit(){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
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
		obj->clock = t1;
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

EXPORT int commit_network(OBJECT *obj){
	network *my = OBJECTDATA(obj,network);
	return my->commit();
}

EXPORT int notify_network(OBJECT *obj, int update_mode, PROPERTY *prop){
	network *my = OBJECTDATA(obj,network);
	return my->notify(update_mode, prop);
}
/**@}**/
