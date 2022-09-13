/// $Id$
/// @file native.cpp
/// @ingroup connection
/// Native class implementation

#include <cstdlib>

#include "native.h"

EXPORT_CREATE(native);
EXPORT_INIT(native);
EXPORT_PRECOMMIT(native);
EXPORT_SYNC(native);
EXPORT_COMMIT(native);
EXPORT_FINALIZE(native);
EXPORT_NOTIFY(native);
EXPORT_PLC(native);
EXPORT_LOADMETHOD(native,link);
EXPORT_LOADMETHOD(native,option);

CLASS *native::oclass = NULL;
native *native::defaults = NULL;

native::VARMAPINDEX native::get_varmapindex(const char *name)
{
	static const char *varmapname[] = {"",
							  "allow",
							  "forbid",
							  "init",
							  "precommit",
							  "presync",
							  "sync",
							  "postsync",
							  "commit",
							  "prenotify",
							  "postnotify",
							  "finalize",
							  "plc",
							  "term"};
	VARMAPINDEX n;
	for ( n=ALLOW ; n<_NUMVMI ; n=(VARMAPINDEX)((int)n+1) )
	{
		if ( strcmp(varmapname[n],name)==0 )
			return n;
	}
	return NONE;
}

int native::parse_function(const char *specs)
{
	// parse specs
	char localclass[64];
	char localname[64];
	char direction[8];
	char remoteclass[64];
	char remotename[64];
	if (sscanf(specs,"%[^/]/%[^-<>\t ]%*[\t ]%[-<>]%*[\t ]%[^/]/%[^\n]",localclass,localname,direction,remoteclass,remotename)!=5 )
	{
		gl_error("native::add_function(const char *spec='%s'): specification is invalid",specs);
		return 0;
	}

	// setup outgoing call
	if ( strcmp(direction,"->")==0 )
	{
		// get local class structure
		CLASS *fclass = callback->class_getname(localclass);
		if ( fclass==NULL )
		{
			gl_error("native::add_function(const char *spec='%s'): local class '%s' does not exist",specs,localclass);
			return 0;
		}

		// check local class function map
		FUNCTIONADDR flocal = callback->function.get(localclass,localname);
		if ( flocal!=NULL )
			gl_warning("native::add_function(const char *spec='%s'): outgoing call definition of '%s' overwrites existing function definition in class '%s'",specs,localname,localclass);

		// get relay function
		flocal = add_relay_function(this,localclass,"",remoteclass,remotename,NULL, DXD_WRITE);
		if ( flocal==NULL )
			return 0;

		// define relay function
		return callback->function.define(fclass,localname,flocal)!=NULL;
	}

	// setup incoming call
	else if ( strcmp(direction,"<-")==0 )
	{
		gl_error("native::add_function(const char *spec='%s'): incoming calls not implemented",specs);
		return 0;
	}

	// TODO
	else
	{
		gl_error("native::add_function(const char *spec='%s'): bidirection calls not implemented",specs);
		return 0;
	}
    exception("connection/native::add_function(const char *spec='%s'): internal error (unexpected return path)",specs);
}

native::native(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"native",sizeof(native),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
		if (oclass==NULL)
			throw "connection/native::native(MODULE*): unable to register class connection:native";
		else
			oclass->trl = TRL_UNKNOWN;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_enumeration, "mode",get_mode_offset(),
				PT_DESCRIPTION,"connection mode",
				PT_KEYWORD,"SERVER",(enumeration)CM_SERVER,
				PT_KEYWORD,"CLIENT",(enumeration)CM_CLIENT,
				PT_KEYWORD,"NONE",(enumeration)CM_NONE,
			PT_enumeration, "transport",get_transport_offset(),
				PT_DESCRIPTION,"connection transport",
				PT_KEYWORD,"UDP",(enumeration)CT_UDP,
				PT_KEYWORD,"TCP",(enumeration)CT_TCP,
				PT_KEYWORD,"NONE",(enumeration)CT_NONE,
			PT_double, "timestep",get_timestep_offset(),
				PT_DESCRIPTION,"timestep between updates",
				PT_UNITS, "s",
			// TODO add published properties here
			NULL)<1)
				throw "connection/native::native(MODULE*): unable to publish properties of connection:native";

		if ( !gl_publish_loadmethod(oclass, "link", reinterpret_cast<int (*)(void *, char *)>(loadmethod_native_link)) )
			throw "connection/native::native(MODULE*): unable to publish link method of connection:native";
		if ( !gl_publish_loadmethod(oclass, "option",
									reinterpret_cast<int (*)(void *, char *)>(loadmethod_native_option)) )
			throw std::runtime_error("connection/native::native(MODULE*): unable to publish option method of connection:native");
		mode = 0;
		transport = 0;
		memset(map,0,sizeof(map));
	}
}

int native::create(void) 
{
	mode = CM_CLIENT;
	transport = CT_TCP;
	timestep = 1.0;

	// setup all the variable maps
	for ( int n=ALLOW ; n<_NUMVMI ; n++ )
		map[n] = new varmap;

	return 1; /* return 1 on success, 0 on failure */
}

int native::init(OBJECT *parent, TRANSLATOR *xlate)
{
	// process all the locals in the variable map
	for ( int n=ALLOW ; n<_NUMVMI ; n++ )
	{
		// resolve objects
		map[n]->resolve();

		// create cache for data
		if ( n>=_FIRST && n<=_LAST )
			map[n]->linkcache(get_connection(),(void *)xlate);
	}
		
	return 1;
}

/// link pseudo-property handler for native connections
int native::link(char *value)
{
	char command[256];
	char argument[1024];
	// parse the pseudo-property
	if ( sscanf(value,"%[^:]:%[^\n]", command, argument)==2 )
	{
		gl_verbose("connection/native::link(char *value='%s') parsed ok", value);

		// function command is a special case of link
		if ( strcmp(command,"function")==0 )
		{
			return parse_function(argument);
		}
		else
		{
			int n = get_varmapindex(command);
			if ( n!=NONE )
				return map[n]->add(argument, CT_UNKNOWN);
			else
			{
				gl_error("connection/native::link(char *value='%s') map '%s' not recognized", value, command);
				return 0;
			}
		}
	}
	else
	{
		gl_error("connection/native::link(char *value='%s'): unable to parse link argument", value);
		return 0;
	}
	return 1;
}

/// option pseudo-property handler for native connections
int native::option(char *value)
{
	char target[256];
	char command[1024];

	// parse the pseudo-property
	if ( sscanf(value,"%[^:]:%[^\n]", target, command)==2 )
	{
		gl_verbose("connection/native::option(char *value='%s') parsed ok", value);
		return option(target,command);
	}
	else
	{
		gl_error("connection/native::option(char *value='%s'): unable to parse option argument", value);
		return 0;
	}
}

/// option write accessor for native connections
int native::option(char *target, char *command)
{
	if ( strcmp(target,"connection")==0 )
	{
		if ( get_connection()==NULL )
		{
			new_connection(command);
			if ( m==NULL )
			{
				gl_error("connection/native::option(char *target='%s', char *command='%s'): connection with options '%s' failed", target, command, command);
				return 0;
			}
			else
				return 1;
		}
		else if ( !get_connection()->option(command) )
			gl_error("connection/native::option(char *target='%s', char *command='%s'): connection mode already set", target, command);
		return 0;
	}
	else if ( get_connection()==NULL )
	{
		gl_error("option(char *target='%s', char *command='%s'): connection mode hasn't be established", target, command);
		return 0;
	}
	else
		return get_connection()->option(target,command);
}

int native::precommit(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[PRECOMMIT],"precommit",xlate)<0 )
	{
		gl_error("connection/native::precommit(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
		return 0;
	}
	else
		return 1;
}

TIMESTAMP native::presync(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[PRESYNC],"presync",xlate)<0 )
	{
		gl_error("connection/native::presync(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
		return TS_ZERO;
	}
	else
		return t + (TIMESTAMP)timestep;
}

TIMESTAMP native::sync(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[SYNC],"sync",xlate)<0 )
	{
		gl_error("connection/native::sync(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
		return TS_ZERO;
	}
	else
		return t + (TIMESTAMP)timestep;
}

TIMESTAMP native::postsync(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[POSTSYNC],"postsync",xlate)<0 )
	{
		gl_error("connection/native::postsync(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
		return TS_ZERO;
	}
	else
		return t + (TIMESTAMP)timestep;
}

TIMESTAMP native::commit(TIMESTAMP t0, TIMESTAMP t1, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[COMMIT],"commit",xlate)<0 )
	{
		gl_error("connection/native::commit(TIMESTAMP t0=%lld, TIMESTAMP t1=%lld, TRANSLATOR *xltr=%p): update failed", t0,t1,xlate);
		return TS_ZERO;
	}
	else
		return t0 + (TIMESTAMP)timestep;
}

int native::prenotify(PROPERTY *p,char *v, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[PRENOTIFY],"prenotify",xlate)<0 )
	{
		gl_error("connection/native::prenotify(PROPERTY *p={name='%s'}, char *v='%s', TRANSLATOR *xltr=%p): update failed", p->name,v,xlate);
		return 0;
	}
	else 
		return 1;
}

int native::postnotify(PROPERTY *p,char *v, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[POSTNOTIFY],"postnotify",xlate)<0 )
	{
		gl_error("connection/native::postnotify(PROPERTY *p={name='%s'}, char *v='%s', TRANSLATOR *xltr=%p): update failed", p->name,v,xlate);
		return 0;
	}
	else 
		return 1;
}

int native::finalize(TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[FINALIZE],"finalize",xlate)<0 )
	{
		gl_error("connection/native::finalize(TRANSLATOR *xltr=%p): update failed", xlate);
		return 0;
	}
	else 
		return 1;
}

TIMESTAMP native::plc(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[PLC],"plc",xlate)<0 )
	{
		gl_error("connection/native::plc(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
		return 0;
	}
	else 
		return t + (TIMESTAMP)timestep;
}

void native::term(TIMESTAMP t, TRANSLATOR *xlate)
{
	if ( get_connection()->update(map[TERM],"term",xlate)<0 )
		gl_error("connection/native::sync(TIMESTAMP t=%lld, TRANSLATOR *xltr=%p): update failed", t,xlate);
}
//TODO declare this function.
/*int64 native::function(char*remotename,char*functionname,void*data,size_t datalen)
{
	int64 result = 0;
	if ( get_connection()->route_function(remotename,functionname,data,datalen,result)<0 )
		gl_error("connection/native::function(%s,%s,...): update failed", remotename,functionname);
	return result;
}*/

///////////////////////////////////////////////////////////////////////////////////////
// FUNCTION RELAY
///////////////////////////////////////////////////////////////////////////////////////

static FUNCTIONRELAY *first_relayfunction = NULL;

static char hex(char c)
{
	if ( c<10 ) return c+'0';
	else if ( c<16 ) return c-10+'A';
	else return '?';
}
static char unhex(char h)
{
	if ( h>='0' && h<='9' )
		return h-'0';
	else if ( h>='A' && h<='F' )
		return h-'A'+10;
	else if ( h>='a' && h<='f' )
		return h-'a'+10;
	return h-' ';
}
static size_t convert_to_hex(char *out, size_t max, const char *in, size_t len)
{
	size_t hlen = 0;
	for ( size_t n=0; n<len ; n++,hlen+=2 )
	{
		char byte = in[n];
		char lo = in[n]&0xf;
		char hi = (in[n]>>4)&0xf;
		*out++ = hex(lo);
		*out++ = hex(hi);
		if ( hlen>=max ) return -1; // buffer overrun
	}
	*out = '\0';
	return hlen;
}
static size_t convert_from_hex(void *buf, size_t len, const char *hex, size_t hexlen)
{
	char *p = (char*)buf;
	char lo = 0;
	char hi = 0;
	char c = 0;
	size_t n = 0;
	for(n = 0; n < hexlen && *hex != '\0'; n += 2)
	{
		c = unhex(*hex);
		if ( c==-1 ) return -1; // bad hex data
		lo = c&0x0f;
		c = unhex(*(hex+1));
		hi = (c<<4)&0xf0;
		if ( c==-1 ) return -1; // bad hex data
		*p = hi|lo;
		p++;
		hex = hex + 2;
		if ( n>=len ) return -1; // buffer overrun
	}
	return n;
}

/// relay function to handle outgoing function calls
extern "C" void outgoing_route_function(char *from, char *to, char *funcName, char *funcClass, void *data, size_t len)
{
	int64 result = -1;
	char *rclass = funcClass;
	char *lclass = from;
	size_t hexlen = 0;
	FUNCTIONRELAY *relay = find_relay_function(funcName,rclass);
	if(relay == NULL){
		throw("native::outgoing_route_function: the relay function for function name %s could not be found.", funcName);
	}
	char message[3000] = "";

	// get transport from relay 
	connection_mode *connection = relay->route->get_connection();
	connection_transport *transport = connection->get_transport();
	size_t msglen = 0;

	// check from and to names
	if ( to==NULL || from==NULL )
	{
		throw("from objects and to objects must be named.");
	}

	// write from and to names to transport
	connection->xlate_out(transport,"to",to,0,ETO_QUOTES);
	connection->xlate_out(transport,"class",relay->remoteclass,0,ETO_QUOTES);
	connection->xlate_out(transport,"function",relay->remotename,0,ETO_QUOTES);
	connection->xlate_out(transport,"from",from,0,ETO_QUOTES);

	// convert data to hex
	hexlen = convert_to_hex(message,sizeof(message),(const char*)data,len);
	if ( hexlen>0 )
		connection->xlate_out(transport,"data",message,0,ETO_QUOTES);
	
	// deliver message to transport
	if ( transport->send(message,msglen)<0 )
	{
		throw("Message failed to be sent.");
	}
}

extern "C" FUNCTIONADDR add_relay_function(native *route, const char *fclass,const char *flocal, const char *rclass, const char *rname, TRANSLATOR *xlate, DATAEXCHANGEDIRECTION direction)
{
	// check for existing of relay (note only one relay is allowed per class pair)
	FUNCTIONRELAY *relay = find_relay_function(rname, rclass);
	if ( relay!=NULL )
	{
		gl_error("connection_transport::create_relay_function(rclass='%s', rname='%s') a relay function is already defined for '%s/%s'", rclass,rname,rclass,rname);
		return 0;
	}

	// allocate space for relay info
	relay = (FUNCTIONRELAY*)malloc(sizeof(FUNCTIONRELAY));
	if ( relay==NULL )
	{
		gl_error("connection_transport::create_relay_function(rclass='%s', rname='%s') memory allocation failed", rclass,rname);
		return 0;
	}

	// setup relay info
	relay->localclass = fclass;
	relay->localcall = flocal;
	strncpy(relay->remoteclass,rclass,sizeof(relay->remoteclass)-1);
	strncpy(relay->remotename,rname,sizeof(relay->remotename)-1);
	relay->drtn = direction;
	relay->next = first_relayfunction;
	relay->xlate = xlate;

	// link to existing relay list (if any)
	relay->route = route;
	first_relayfunction = relay;

	// return entry point for relay function
	return (FUNCTIONADDR)outgoing_route_function;
}

extern "C" FUNCTIONRELAY *find_relay_function(const char*rname, const char *rclass)
{
	// TODO: this is *very* inefficient -- a hash should be used instead
	FUNCTIONRELAY *relay;
	for ( relay=first_relayfunction ; relay!=NULL ; relay=relay->next )
	{
		if ( strcmp(relay->remoteclass,rclass)==0 && strcmp(relay->remotename, rname)==0 )
			return relay;
	}
	return NULL;
}

