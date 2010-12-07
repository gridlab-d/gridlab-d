/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_interface.cpp
	@addtogroup network_interface Performance network_interface object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include "network_interface.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* network_interface::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
network_interface::network_interface(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"network_interface",sizeof(network_interface),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_interface class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_enumeration, "duplex_mode", PADDR(duplex_mode),
				PT_KEYWORD, "FULL_DUPLEX", DXM_FULL_DUPLEX,
				PT_KEYWORD, "HALF_DUPLEX", DXM_HALF_DUPLEX,
				PT_KEYWORD, "TRANSMIT_ONLY", DXM_XMIT_ONLY,
				PT_KEYWORD, "RECEIVE_ONLY", DXM_RECV_ONLY,
			PT_object, "to", PADDR(to),
			PT_double, "size[MB]", PADDR(size), PT_ACCESS, PA_REFERENCE,
			PT_double, "transfer_rate[MB/s]", PADDR(transfer_rate),
			PT_enumeration, "status", PADDR(status), PT_ACCESS, PA_REFERENCE,
				PT_KEYWORD, "NONE", NIS_NONE,
				PT_KEYWORD, "QUEUED", NIS_QUEUED,
				PT_KEYWORD, "REJECTED", NIS_REJECTED,
				PT_KEYWORD, "SENDING", NIS_SENDING,
				PT_KEYWORD, "COMPLETE", NIS_COMPLETE,
				PT_KEYWORD, "FAILED", NIS_FAILED,
			PT_double, "bandwidth_used[MB/s]", PADDR(bandwidth_used), PT_ACCESS, PA_REFERENCE,
			PT_int32, "buffer_size", PADDR(buffer_size),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_interface properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

// create is called every time a new object is loaded
int network_interface::create() 
{
	return 1;
}

int network_interface::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);
	// input validation checks
	if(to == 0){
		GL_THROW("network interface is not connected to a network");
	}
	if(!gl_object_isa(to, "network", "comm")){
		GL_THROW("network interface is connected to a non-network object");
	} else {
		pNetwork = OBJECTDATA(to, network);
	}
	if(transfer_rate <= 0.0){
		GL_THROW("network interface has a non-positive transfer rate");
	}

	// operational variable defaults
	bandwidth_used = 0.0;
	size = 0;
	// success
	return 1;
}

int network_interface::isa(char *classname){
	return strcmp(classname,"network_interface")==0;
}

TIMESTAMP network_interface::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

//EXPORT int commit_network_interface(OBJECT *obj){
int network_interface::commit(){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//return my->notify(update_mode, prop);
int network_interface::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_network_interface(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(network_interface::oclass);
	if (*obj!=NULL)
	{
		network_interface *my = OBJECTDATA(*obj,network_interface);
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

EXPORT int init_network_interface(OBJECT *obj)
{
	network_interface *my = OBJECTDATA(obj,network_interface);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_network_interface(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,network_interface)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_network_interface(OBJECT *obj, TIMESTAMP t1)
{
	network_interface *my = OBJECTDATA(obj,network_interface);
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

EXPORT int commit_network_interface(OBJECT *obj){
	network_interface *my = OBJECTDATA(obj,network_interface);
	return my->commit();
}

EXPORT int notify_network_interface(OBJECT *obj, int update_mode, PROPERTY *prop){
	network_interface *my = OBJECTDATA(obj,network_interface);
	return my->notify(update_mode, prop);
}
/**@}**/
