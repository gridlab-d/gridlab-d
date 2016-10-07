// switch_coordinator.cpp
// Copyright (C) 2016, Stanford University
// Author: David P. Chassin (dchassin@slac.stanford.edu)
//
// switch_coordinator - General purpose switch_coordinator objects
//
// == Synopsis ==
//
//   object switch_coordinator {
//     status {IDLE, ARMED, ACTIVE};
//     connect <name1>;
//     connect <name2>;
//     ...
//     armed [<name1>|<name2>|...];
//   }
//
// Description
// -----------
//
// Use 'connect' to add switches to the list of switches coordinated by the object.  Up to 64 switches can
// be coordinated. Each connected switch can be armed by setting the 'armed' object using the switch's name.
// Once all the armed switches have be set, use 'status' to activate the switching scheme.
//
// See also
// --------
// * powerflow
// ** switch
//

#include "gismo.h"

EXPORT_CREATE(switch_coordinator);
EXPORT_INIT(switch_coordinator);
EXPORT_PRECOMMIT(switch_coordinator);
EXPORT_SYNC(switch_coordinator);
EXPORT_COMMIT(switch_coordinator);
EXPORT_NOTIFY_PROP(switch_coordinator,armed);
EXPORT_LOADMETHOD(switch_coordinator,connect);
EXPORT_LOADMETHOD(switch_coordinator,arm);
EXPORT_LOADMETHOD(switch_coordinator,disarm);

CLASS *switch_coordinator::oclass = NULL;
switch_coordinator *switch_coordinator::defaults = NULL;

switch_coordinator::switch_coordinator(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"switch_coordinator",sizeof(switch_coordinator),PC_POSTTOPDOWN|PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class switch_coordinator";
		else
			oclass->trl = TRL_PROVEN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration,"status",get_status_offset(), PT_DESCRIPTION,"switch coordination status",
				PT_KEYWORD, "IDLE", (enumeration)SCS_IDLE,
				PT_KEYWORD, "ARMED", (enumeration)SCS_ARMED,
				PT_KEYWORD, "ACTIVE", (enumeration)SCS_ACTIVE,
			PT_set,"armed",get_armed_offset(), PT_DESCRIPTION,"set of armed switches",
				PT_KEYWORD, "NONE", (set)0,
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}
#define PUBLISH_METHOD(X) if ( !gl_publish_loadmethod(oclass,#X,loadmethod_switch_coordinator_##X) ) throw "gismo/switch_coordinator::switch_coordinator(MODULE*): unable to publish "#X" method";
		PUBLISH_METHOD(connect)
		PUBLISH_METHOD(arm)
		PUBLISH_METHOD(disarm)
		memset(this,0,sizeof(switch_coordinator));
	}
}

int switch_coordinator::create(void)
{
	memcpy(this,defaults,sizeof(*this));

	return 1; /* return 1 on success, 0 on failure */
}

int switch_coordinator::connect(char *name)
{
	gld_object *obj = gld_object::find_object(name);
	if ( !obj->is_valid() )
	{
		error("object '%s' is not found", name);
		return 0;
	}
	if ( !obj->isa("switch") )
	{
		error("object '%s' is not a switch", name);
		return 0;
	}
	if ( n_switches < 64 )
	{
		index[n_switches] = new gld_property(obj,"status");
		if ( index[n_switches]==NULL || ! index[n_switches]->is_valid() )
		{
			error("unable to get property 'status' of object '%s'", name);
			return 0;
		}
		gld_property prop(this,"armed");
		gld_keyword *last;
		for ( last = prop.get_first_keyword() ; last->get_next()!=NULL ; last = last->get_next() ) {}
		KEYWORD *next = new KEYWORD;
		strncpy(next->name,name,sizeof(next->name)-1);
		next->value = (unsigned int64)(pow(2,n_switches));
		next->next = NULL;
		((KEYWORD*)last)->next = next;
		n_switches++;
		verbose("added switch '%s' to arming list", name);
		return 1;
	}
	else
	{
		error("too many switches connected (max 64) to connect '%s'", name);
		return 0;
	}

	return 1;
}

int switch_coordinator::arm(char *name)
{
	gld_property prop(this,"armed");
	gld_keyword *key;
	for ( key = prop.get_first_keyword() ; key->get_next()!=NULL ; key = key->get_next() )
	{
		if ( *key==name )
		{
			armed |= key->get_set_value();
			notify_armed();
			verbose("arming switch '%s'",name);
			return 1;
		}
	}
	error("unable to arm '%s', no such device connected", name);
	return 0;
}

int switch_coordinator::disarm(char *name)
{
	gld_property prop(this,"armed");
	gld_keyword *key;
	for ( key = prop.get_first_keyword() ; key->get_next()!=NULL ; key = key->get_next() )
	{
		if ( *key==name )
		{
			armed &= ~key->get_set_value();
			verbose("disarming switch '%s'",name);
			notify_armed();
			return 1;
		}
	}
	error("unable to disarm '%s', no such device connected", name);
	return 0;
}

int switch_coordinator::init(OBJECT *parent)
{
	return 1;
}

int switch_coordinator::precommit(TIMESTAMP t1)
{
	if ( status==SCS_ACTIVE )
	{
		// record the initial state of the armed switches
		states = armed;
		unsigned int n;
		for ( n = 0 ; n < 64 ; n++ )
		{
			verbose("recording state of '%s'", (const char*)index[n]->get_name());
			states ^= ( index[n]->get_enumeration()==switch_object::OPEN ? 0 : 1<<n ); 
		}
	}
	return 1;
}

TIMESTAMP switch_coordinator::presync(TIMESTAMP t1)
{
	if ( status==SCS_ACTIVE )
	{
		// set state of armed devices
		unsigned int n;
		for ( n = 0 ; n < 64 ; n++ )
		{
			if ( armed&(1<<n) )
			{
				verbose("changing state of '%s'", (const char*)index[n]->get_name());
				index[n]->setp(states&(1<<n)?(enumeration)(switch_object::CLOSED):(enumeration)(switch_object::OPEN));
			}
		}
	}
	return TS_NEVER;
}

TIMESTAMP switch_coordinator::commit(TIMESTAMP t1, TIMESTAMP t2)
{
	if ( status==SCS_ACTIVE )
	{
		armed = 0;
		status = SCS_IDLE;
		verbose("switching done--changing status to IDLE");
	}
	return TS_NEVER;
}

int switch_coordinator::notify_armed(char *value)
{
	if ( armed == 0 ) // disarm regardless of status
	{
		verbose("nothing armed--changing status to IDLE");
		status = SCS_IDLE;
	}
	else if ( status == SCS_IDLE) // arm only if idle
	{
		verbose("first switch armed--changing status to ARMED");
		status = SCS_ARMED;
	}
	// active state isn't affected by arming
	return 1;
}
	
