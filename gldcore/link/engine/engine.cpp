// $Id: engine.cpp
// Copyright (C) 2012 Battelle Memorial Institute

#include <cstdlib>

#include "engine.h"

CALLBACKS *callback = NULL;

#include "link.h"
#include "build.h"

gld_property **sync_index = NULL;
unsigned int sync_index_size = 0;

typedef enum {ELS_INIT=0, ELS_OK=1, ELS_ERROR=2, ELS_TERM=3} ENGINELINKSTATUS;
const char *enginelinkstatus[] = {"INIT","OK","ERROR","TERM"};


unsigned long get_addr(struct sockaddr_in *data, unsigned int part)
{
	union {
		unsigned char p[4];
		unsigned long dw;
	} addr;
	addr.dw = data->sin_addr.s_addr;
	if ( part==0 )
		return ntohl(addr.dw);
	else if ( part>=1 && part<=4 )
		return addr.p[part-1];
	else
		return -1;
}

unsigned short get_port(struct sockaddr_in *data)
{
	return ntohs(data->sin_port);
}

EXPORT bool glx_create(glxlink *mod, CALLBACKS *fntable)
{
	callback = fntable;



	// create engine link
	ENGINELINK *engine = new ENGINELINK;
	memset(engine,0,sizeof(ENGINELINK));

#ifdef USE_TCP
	//do nothing
#else
	setupUDPSocket(engine);
#endif

	// set defaults
	strcpy(engine->protocol,"UDP");

	engine->recv_timeout = 100; // 10 seconds before recv gives up
	engine->cachesize = 0; // automatic
	engine->send = NULL;
	engine->recv = NULL;

	mod->set_data(engine);
	return true;
}

EXPORT bool glx_settag(glxlink *mod, char *tag, char *data)
{
	ENGINELINK *engine = (ENGINELINK*)mod->get_data();
	if ( strcmp(tag,"timeout")==0 )
	{
		if ( sscanf(data,"%u",&engine->recv_timeout)==0 )
		{
			gl_error("invalid timeout value '%s'", data);
			return false;
		}
		else
		{
			gl_debug("timeout set to %d s", engine->recv_timeout);
			return true;
		}
	}
	else if ( strcmp(tag,"cache")==0 )
	{
		if ( strcmp(data,"auto")==0 )
		{
			gl_debug("cachesize set to 'auto'");
			engine->cachesize = 0;
			return true;
		}
		else if ( sscanf(data,"%u",&engine->cachesize)==0 )
		{
			gl_error("invalid cachesize value '%s'", data);
			return false;
		}
		else
		{
			gl_debug("cachesize set to %d MB", engine->cachesize);
			return true;
		}
	}
	gl_error("tag '%s' not valid for engine target", tag);
	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// RAW MESSAGE HANDLING

void engine_sleep(unsigned int msec)
{
#ifdef _WIN32
	Sleep(msec);
#else
	usleep(msec*1000);
#endif
}


int engine_recv(ENGINELINK *engine, char *buffer, int maxlen)
{
	if(engine->sd->type==UDP)
		return recvUDPSocket(engine,buffer,maxlen);
	return -1; //TCP not yet here!
}

int engine_send(ENGINELINK *engine, char *buffer,int len){

      if(engine->sd->type==UDP)
		  return sendUDPSocket(engine,buffer,len);
	  return -1;
}


////////////////////////////////////////////////////////////////////////////////////////////////
// PROTOCOL MESSAGE HANDLING
bool recv_status(ENGINELINK *engine, ENGINELINKSTATUS *status, char *msg=NULL, int maxlen=0)
{
	char buffer[1500];
	int len = engine_recv(engine,buffer,sizeof(buffer));
	if ( len<=0 )
	{
		gl_error("recv_status failed");
		return false;
	}
	char *tag = strchr(buffer,' ');
	if ( strcmp(buffer,"INIT")==0 )
	{
		if ( msg!=NULL && tag!=NULL && maxlen>1) strncpy(msg,tag+1,maxlen-1);
		*status = ELS_INIT;
		return true;
	}
	else if ( strcmp(buffer,"OK")==0 )
	{
		if ( msg!=NULL && tag!=NULL && maxlen>1) strncpy(msg,tag+1,maxlen-1);
		*status = ELS_OK;
		return true;
	}
	else if ( strcmp(buffer,"ERROR")==0 )
	{
		if ( msg!=NULL && tag!=NULL && maxlen>1) strncpy(msg,tag+1,maxlen-1);
		*status = ELS_ERROR;
		return true;
	}
	else if ( strcmp(buffer,"TERM")==0 )
	{
		if ( msg!=NULL && tag!=NULL && maxlen>1) strncpy(msg,tag+1,maxlen-1);
		*status = ELS_TERM;
		return true;
	}
	else
	{
		gl_error("invalid status message received, message was '%s'", buffer);
		return false;
	}
}

bool recv_init(ENGINELINK *engine)
{
	char buffer[1500];
	int len = engine_recv(engine,buffer,sizeof(buffer));
	printf("recv init %s\n",buffer);
	if ( len>0 )
	{
		if ( strcmp(buffer,"INIT")==0 )
			return true;
		else
		{
			gl_error("recv_init() did not receive an init message, message received was '%s'", buffer);
			return false;
		}
	}
	return false;
}

bool send_init(ENGINELINK *engine)
{
	char buffer[1500];
	int len=sprintf(buffer,"GRIDLABD %d.%d.%d (%s)",REV_MAJOR,REV_MINOR,REV_PATCH,BRANCH);

	return engine_send(engine,buffer,len+1) > 0;
}

bool send_protocol(ENGINELINK *engine)
{
        char buffer[1500];
	int len=sprintf(buffer,"PROTOCOL %s", engine->protocol);
	return engine_send(engine,buffer,len+1) > 0;
}

size_t recalc_cachesize(ENGINELINK *engine){
	engine->cachesize=1;
	return engine->cachesize;
}

bool send_cachesize(ENGINELINK *engine)
{
	if ( engine->cachesize==0 ) recalc_cachesize(engine);
	char buffer[1500];
	int len=sprintf(buffer,"CACHESIZE %d", engine->cachesize);

	return engine_send(engine,buffer,len+1) > 0;
}

bool send_timeout(ENGINELINK *engine)
{
	char buffer[1500];
	int len=sprintf(buffer,"TIMEOUT %d", engine->recv_timeout);
	return engine_send(engine,buffer,len) > 0;
}

bool send_status(ENGINELINK *engine, ENGINELINKSTATUS status,char *msg=NULL)
{
	char buffer[1500];
	int len;

	if ( msg==NULL ){
		len = sprintf(buffer,"%s", enginelinkstatus[status]);
		return engine_send(engine,buffer,len+1) > 0;
	}
	else{
		len = sprintf(buffer,"%s\n%s", enginelinkstatus[status],msg);
		return engine_send(engine,buffer,len+1) > 0;
	}
}

bool recv_sync(ENGINELINK *engine,TIMESTAMP *t)
{
	char buffer[1500];
	int len = engine_recv(engine,buffer,sizeof(buffer));
	if ( len>0 )
	{
		gld_clock ts(buffer);
		char timestr[64];
		if ( sscanf(buffer,"SYNC %63[-0-9. :A-Z]",timestr)==1 && ts.from_string(timestr) )
		{
			*t = ts.get_timestamp();
			return true;
		}
		else if ( sscanf(buffer,"SYNC %lld",(int64*)t)==1 )
			return true;
		else
		{
			gl_error("recv_sync() did not receive a valid timestamp, message received was '%s'",buffer);
			return false;
		}
	}
	return false;
}

bool send_time(ENGINELINK *engine,TIMESTAMP t0){

  char buffer[1500];
  int len=sprintf(buffer,"SYNC %lld", t0);
  if(engine_send(engine,buffer,len+1) <=0)
    return false;
  return true;
}

bool send_exports(ENGINELINK *engine)
{
	char buffer[1500];
	int len;
	SYNCDATA *item;
	for ( item=engine->send ; item!=NULL ; item=item->next )
	{
		len=sprintf(buffer,"%d %s", item->index, item->prop->get_string().get_buffer());
		if ( engine_send(engine,buffer,len+1)<=0 )
			return false;
	}
	return true;
}

bool recv_imports(ENGINELINK *engine)
{
	SYNCDATA *item;
	for ( item=engine->recv ; item!=NULL ; item=item->next )
	{
		char buffer[1500];
		unsigned int index;
		char value[1025];
		if ( !engine_recv(engine,buffer,1500)){

		  return false;
		}
		if ( sscanf(buffer,"%d %1024[^\n]",&index,value) < 2)
		  return false;
		if ( index>=0 && index<sync_index_size )
		{
			gld_property *prop = sync_index[index];
			if ( prop->from_string(value)<=0 )
			{
				gl_warning("unable to read import value '%s' for %s.%s", prop->get_object()->name, prop->get_property()->name);
				true;
			}
		}
		else
		{
			gl_warning("recv_imports index value %d is invalid", index);
			return true;
		}
	}
	return true;
}

SYNCDATA *add_property(SYNCDATA *list, unsigned int index, gld_property *prop)
{
	SYNCDATA *data = new SYNCDATA;
	data->index = index;
	data->prop = prop;
	data->next = list;
	return data;
}
bool add_global(ENGINELINK *engine, unsigned int index, GLOBALVAR *var)
{
	gld_property *prop = new gld_property(var);
	engine->send = add_property(engine->send, index, prop);
	engine->recv = add_property(engine->recv, index, prop);
	char buffer[1500];
	char buffname[255];
	if(gl_name(prop->get_object(),buffname,255)==NULL){
		strcpy(buffname,"NULL");
	}
	int len = sprintf(buffer,"GLOBAL %d %d %lu %s %s %s", index,
		(PROPERTYTYPE)prop->get_type(), // TODO convert this to text
		prop->get_size(),buffname, prop->get_name(), prop->get_string().get_buffer());
	return engine_send(engine,buffer,len+1) > 0;
}
bool add_import(ENGINELINK *engine, unsigned int index, OBJECTPROPERTY *objprop)
{
	gld_property *prop = new gld_property(objprop->obj,objprop->prop);
	engine->recv = add_property(engine->recv, index, prop);
	char buffer[1500];
	char buffname[255];
	if(gl_name(prop->get_object(),buffname,255)==NULL){
		strcpy(buffname,"NULL");
	}
	int len = sprintf(buffer,"IMPORT %d %d %lu %s %s %s", index,
		(PROPERTYTYPE)prop->get_type(), // TODO convert this to text
		prop->get_size(),buffname, prop->get_name(), prop->get_string().get_buffer());
	return engine_send(engine,buffer,len+1) > 0;
}
bool add_export(ENGINELINK *engine, unsigned int index, OBJECTPROPERTY *objprop)
{
	gld_property *prop = new gld_property(objprop->obj,objprop->prop);
	engine->send = add_property(engine->send, index, prop);
	char buffer[1500];
	char buffname[255];
	if(gl_name(prop->get_object(),buffname,255)==NULL){
		strcpy(buffname,"NULL");
	}
	int len = sprintf(buffer,"EXPORT %d %d %lu %s %s %s", index,
		(PROPERTYTYPE)prop->get_type(), // TODO convert this to text
		prop->get_size(),buffname, prop->get_name(), prop->get_string().get_buffer());
	return engine_send(engine,buffer,len+1) > 0;

}

////////////////////////////////////////////////////////////////////////////////////////////////
// INITIALIZATION SEQUENCE
EXPORT bool glx_init(glxlink *mod)
{
	ENGINELINK *engine = (ENGINELINK*)mod->get_data();
	gl_verbose("initializing engine link");

	if(engine->sd->type==UDP){
		if(bindUDPSocket(engine) && acceptUDPSocket(engine)){
		}
		else{
			return false;
		}

	}
	if(engine->sd->type==TCP){
		return false;
	}

	// wait for first incoming message (should be INIT)
	if ( !recv_init(engine) )
	{
		gl_error("unable to receive engine link initialization");
		return false;
	}
	else if ( !send_init(engine) )
	{
		gl_error("unable to send engine link version info");
		return false;
	}
	else if ( !send_protocol(engine) ) // TODO support SHMEM/MMAP
	{
		gl_error("unable to send engine link protocol info");
		return false;
	}
	else if ( !send_cachesize(engine) )
	{
		gl_error("unable to send engine link cachesize info");
		return false;
	}
	else if ( !send_timeout(engine) )
	{
		gl_error("unable to send engine timeout info");
		return false;
	}

	// construct and send globals lists
	unsigned int index = 0;
	LINKLIST *item;
	for ( item=mod->get_globals() ; item!=NULL ; item=mod->get_next(item) )
	{
		GLOBALVAR *var = mod->get_globalvar(item);
		if ( var!=NULL && !add_global(engine,index++,var) )
		{
			gl_error("unable to send global specs for %s", var->prop->name);
			return false;
		}
	}

	// construct and send imports lists
	for ( LINKLIST *item=mod->get_imports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_import(item);
		if ( objprop!=NULL && !add_import(engine,index++,objprop) )
		{
			gl_error("unable to send import specs for %s.%s", objprop->obj->name, objprop->prop->name);
			return false;
		}
	}

	// construct and send exports lists
	for ( LINKLIST *item=mod->get_exports() ; item!=NULL ; item=mod->get_next(item) )
	{
		OBJECTPROPERTY *objprop = mod->get_export(item);
		if ( objprop!=NULL && !add_export(engine,index++,objprop) )
		{
			gl_error("unable to send export specs for %s.%s", objprop->obj->name, objprop->prop->name);
			return false;
		}
	}

	// send done message
	if ( !engine_send(engine, const_cast<char*>("DONE"),5) )
	{
		gl_error("unable to send exports DONE message");
		return false;
	}

	// recent status message
	char msg[1500];
	ENGINELINKSTATUS status;
	if ( !recv_status(engine,&status,msg,sizeof(msg)) || status!=ELS_OK )
	{
		gl_error("engine link status not OK [%s]", msg);
		return false;
	}

	// index send/recv lists
	SYNCDATA *data;
	sync_index_size = index;
	sync_index = new gld_property*[sync_index_size];
	memset(sync_index,0,sizeof(gld_property*)*sync_index_size);
	for ( data=engine->send ; data!=NULL ; data=data->next )
		sync_index[data->index] = data->prop;
	for ( data=engine->recv ; data!=NULL ; data=data->next )
		sync_index[data->index] = data->prop;

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// SYNCHRONIZATION SEQUENCE
EXPORT TIMESTAMP glx_sync(glxlink* mod,TIMESTAMP t0)
{
	ENGINELINK *engine = (ENGINELINK*)mod->get_data();
	TIMESTAMP t1 = TS_NEVER;

	if ( !send_time(engine,t0) ){
		printf("sending times!!!\n");
		return TS_INVALID;
	}
	if ( !send_exports(engine) ){
	        printf("sending expo\n");
		return TS_INVALID; // error
	}

	if ( !send_status(engine,ELS_OK) ){
		printf("sending okk\n");
		return TS_INVALID; // error
	}
	t1=0;
	if ( !recv_sync(engine,&t1) ){
		printf("recv time!!!\n");
		return TS_INVALID; // error
	}

	if ( !recv_imports(engine) )
		return TS_INVALID; // error

	ENGINELINKSTATUS status;
	if ( !recv_status(engine,&status) )
		return TS_INVALID; // error

	return status==ELS_OK ? t1 : TS_INVALID;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// TERMINATION SEQUENCE
EXPORT bool glx_term(glxlink* mod)
{
	ENGINELINK *engine = (ENGINELINK*)mod->get_data();

	if ( !send_time(engine,TS_NEVER) )
		return TS_NEVER;

	if ( !send_exports(engine) )
		return TS_INVALID; // error

	if ( !send_status(engine,ELS_TERM) )
		return TS_INVALID; // error

	return true;
}
