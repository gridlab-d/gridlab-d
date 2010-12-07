/** $Id $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network_message.cpp
	@addtogroup network_message Performance network_message object
	@ingroup comm

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include "network_message.h"

//////////////////////////////////////////////////////////////////////////
// CLASS FUNCTIONS
//////////////////////////////////////////////////////////////////////////
CLASS* network_message::oclass = NULL;

// the constructor registers the class and properties and sets the defaults
network_message::network_message(MODULE *mod) 
{
	// first time init
	if (oclass==NULL)
	{
		// register the class definition
		oclass = gl_register_class(mod,"network_message",sizeof(network_message),PC_BOTTOMUP);
		if (oclass==NULL)
			GL_THROW("unable to register object class implemented by %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_message class failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */

		// publish the class properties
		if (gl_publish_variable(oclass,
			PT_object, "from", PADDR(from),
			PT_object, "to", PADDR(to),
			PT_double, "size[B]", PADDR(size),
			PT_char1024, "message", PADDR(message),
			PT_int16, "buffer_size", PADDR(buffer_size),
			PT_int64, "start_time", PADDR(start_time),
			PT_int64, "end_time", PADDR(end_time),
			PT_double, "start_time_frac[s]", PADDR(start_time_frac),
			PT_double, "end_time_frac[s]", PADDR(end_time_frac),
			PT_double, "bytes_sent[B]", PADDR(bytes_sent),
			PT_double, "bytes_resent[B]", PADDR(bytes_resent),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
			/* TROUBLESHOOT
				The registration for the network_message properties failed.   This is usually caused
				by a coding error in the core implementation of classes or the module implementation.
				Please report this error to the developers.
			 */
	}
}

network_message::network_message(){
	from = to = 0;
	size = buffer_size = 0;
	bytes_sent = bytes_resent = 0;
	memset(message, 0, 1024);
	start_time = end_time = -1;
	start_time_frac = end_time_frac = 0.0;
}

int network_message::send_message(OBJECT *f, OBJECT *t, char *m, int sz, TIMESTAMP ts, double frac){
	from = f;
	to = t;
	size = sz;
	memcpy(message, m, size);
	start_time = ts;
	start_time_frac = frac;
	return 1; // success
}

// create is called every time a new object is loaded
int network_message::create() 
{
	gl_error("network_message is an internally used class and should not be instantiated by a model");
	return 0;
}

int network_message::init(OBJECT *parent)
{
	OBJECT *hdr = OBJECTHDR(this);


	// success
	return 1;
}

int network_message::isa(char *classname){
	return strcmp(classname,"network_message")==0;
}

TIMESTAMP network_message::sync(TIMESTAMP t0, TIMESTAMP t1) 
{
	OBJECT *obj = OBJECTHDR(this);
	return TS_NEVER; 
}

int network_message::commit(){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

int network_message::notify(int update_mode, PROPERTY *prop){
	OBJECT *obj = OBJECTHDR(this);
	return 1;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE
//////////////////////////////////////////////////////////////////////////

EXPORT int create_network_message(OBJECT **obj, OBJECT *parent)
{
	*obj = gl_create_object(network_message::oclass);
	if (*obj!=NULL)
	{
		network_message *my = OBJECTDATA(*obj,network_message);
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

EXPORT int init_network_message(OBJECT *obj)
{
	network_message *my = OBJECTDATA(obj,network_message);
	try {
		return my->init(obj->parent);
	}
	catch (char *msg)
	{
		gl_error("%s::%s.init(OBJECT *obj={name='%s', id=%d}): %s", obj->oclass->module->name, obj->oclass->name, obj->name, obj->id, msg);
		return 0;
	}
}

EXPORT int isa_network_message(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0){
		return OBJECTDATA(obj,network_message)->isa(classname);
	} else {
		return 0;
	}
}

EXPORT TIMESTAMP sync_network_message(OBJECT *obj, TIMESTAMP t1)
{
	network_message *my = OBJECTDATA(obj,network_message);
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

EXPORT int commit_network_message(OBJECT *obj){
	network_message *my = OBJECTDATA(obj,network_message);
	return my->commit();
}

EXPORT int notify_network_message(OBJECT *obj, int update_mode, PROPERTY *prop){
	network_message *my = OBJECTDATA(obj,network_message);
	return my->notify(update_mode, prop);
}
/**@}**/
