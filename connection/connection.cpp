// $Id$

#include "connection.h"
#include "server.h"
#include "client.h"

////////////////////////////////////////////////////////////////////////////////////
connection_mode::connection_mode(void)
{
	seqnum = 0;
	transport = NULL;
	ignore_error = 1;
}

CONNECTIONMODE connection_mode::get_mode(const char *s)
{
	if ( strcmp(s,"server")==0 ) return CM_SERVER;
	else if ( strcmp(s,"client")==0 ) return CM_CLIENT;
	else return (CONNECTIONMODE)CM_NONE;
}


void connection_mode::set_translators(EXCHANGETRANSLATOR *out,
									  EXCHANGETRANSLATOR *in,
									  TRANSLATOR *data)
{
	xlate_out = out;
	xlate_in = in;
}

connection_mode* connection_mode::new_instance(const char *options)
{
	char *next_tag=NULL;
	char *tag = NULL;
	connection_mode *connection = NULL;
	char temp[4096];
	strcpy(temp,options);
	while ( (tag=strtok_s(tag==NULL?temp:NULL,",",&next_tag))!=NULL )
	{
		if ( connection==NULL )
		{
			CONNECTIONMODE e = get_mode(tag);
			switch ( e ) {
			case CM_SERVER: 
				connection = (connection_mode*)new server();
				break;
			case CM_CLIENT: 
				connection = (connection_mode*)new client();
				break;
			default: 
				gl_error("connection_mode::new_instance(char *options='%s'): unknown connection mode '%s'", options, tag);
				return NULL;
				break;
			}
		}
		else if ( connection->get_transport()==NULL )
		{
			connection->set_transport(connection_transport::get_transport(tag));
		}
		else
		{
			char cmd[256], arg[256];
			int n = sscanf(tag,"%[^ =]%*[ =]%[^,;]",cmd,arg);
			if ( n==0 )
			{
				gl_error("connection_mode::new_instance(char *options='%s'): unable to parse tag '%s'", options, tag);
				return NULL;
			}
			else if ( n==1 )
			{
				// TODO add flag options here
				gl_error("connection_mode::new_instance(char *options='%s'): unrecognized tag '%s'", options, tag);
				return NULL;
			}
			else if ( n==2 )
			{
				if ( strcmp(cmd,"on_error")==0 )
				{
					if ( strcmp(arg,"halt")==0 )
						connection->ignore_error = 0;
					else if ( strcmp(arg,"ignore")==0 )
						connection->ignore_error = 1;
					// TODO add other error modes
					else
					{
						gl_error("connection_mode::new_instance(char *options='%s'): unrecognized %s value '%s'", options, cmd, arg);
						return NULL;
					}
				}
				// TODO add multi-argument options here
				else
				{
					gl_error("connection_mode::new_instance(char *options='%s'): unrecognized tag '%s'", options, tag);
					return NULL;
				}
				goto OK;
			}
			// hand off to client/server subclass
			if ( !connection->option(tag)  )
			{
				gl_error("connection_mode::new_instance(char *options='%s'): unrecognized connection option '%s'", options, tag);
				return NULL;
			}
		}
	}
OK:
	if ( connection==NULL )
	{
		gl_warning("connection_mode::new_instance(char *options='%s'): connection mode not specified", options);
	}
	if ( connection->get_transport()==NULL )
	{
		gl_warning("connection_mode::new_instance(char *options='%s'): transport not specified, using default UDP", options);
		connection->set_transport(CT_UDP);
	}

	return connection;
}

void connection_mode::set_transport(CONNECTIONTRANSPORT t)
{
	// ignore same transport
	if ( transport!=NULL && transport->get_transport()==t )
		return;

	// delete old transport
	if ( transport!=NULL )
		delete transport; 

	// create new transport
	transport = connection_transport::new_instance(t);

	// check result
	if ( transport==NULL )
		gl_warning("connection_mode::set_transport(): unable to create new instance");

	// reset sequence number
	seqnum = 0;
}

void connection_mode::error(char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_error("connection/%s: %s",get_mode_name(), msg);
}
void connection_mode::warning(char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_warning("connection/%s: %s",get_mode_name(), msg);
}
void connection_mode::info(char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_output("connection/%s: %s",get_mode_name(), msg);
}
void connection_mode::debug(int level, const char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_debug("connection/%s: %s",get_mode_name(), msg);
}
void connection_mode::exception(const char *fmt, ...)
{
	static char msg[1024];
	size_t len = sprintf("connection/%s: ", get_mode_name());
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg+len,fmt,ptr);
	va_end(ptr);
	throw msg;
}

int connection_mode::init(void)
{
	return get_transport()->init();
}
int connection_mode::option(char *target, char *command)
{
	// specifically targetted to server or client
	if ( strcmp(target,get_mode_name())==0 ) 
	{
		switch ( get_mode() ) {
		case CM_CLIENT: return ((client*)this)->option(command);
		case CM_SERVER: return ((server*)this)->option(command);
		default: return 0;
		}
	}

	// specifically target to udp or tcp
	if ( strcmp(target,"transport")==0 || strcmp(target,transport->get_transport_name())==0 )
		return transport->option(command);

	// cache control options
	if ( strcmp(target,"readcache")==0 )
		return read_cache.option(command);
	if ( strcmp(target,"writecache")==0 )
		return write_cache.option(command);

	// local target
	if ( strcmp(target,"connection")==0 )
	{
		if ( strcmp(command,"ignore")==0 )
		{
		}
	}
	else 
		gl_error("connection/connection_mode::option(char *target='%s', char *command='%s'): target not recognized", target, command);
	return 0;
}

int connection_mode::update(VARMAP *var, DATAEXCHANGEDIRECTION dir, TRANSLATOR *xlate)
{
	gld_property *target = var->obj;
	if ( !target->is_valid() )
	{
		gl_error("connection_mode::update(VARMAP *var=%p, TRANSLATOR *xlate=%p): target '%s' is not valid", var, xlate, var->local_name);
		return 0;
	}

	if ( (dir == DXD_READ && var->dir == DXD_READ) )
	{
		if ( !read_cache.read(var, xlate) ) 
			gl_warning("connection_mode::update(VARMAP *var=%p, TRANSLATOR *xlate=%p): unable to read/translate data from %s", var, xlate, var->remote_name);
		else
			gl_debug("reading %s from %s", var->local_name, var->remote_name);
	}
	else if ( (dir == DXD_WRITE && var->dir == DXD_WRITE) )
	{	
		if ( !write_cache.write(var, xlate) )
			gl_warning("connection_mode::update(VARMAP *var=%p, TRANSLATOR *xlate=%p): unable to write/translate data for %s", var, xlate, var->remote_name);
		else
			gl_debug("writing %s to %s", var->local_name, var->remote_name);
	}

	return 1;
}

int connection_mode::update(varmap *varlist, char *tag, TRANSLATOR *xlate)
{
	int count=0;
	VARMAP *v;
	//update outgoing cache variables with the local values
	for ( v = varlist->getfirst() ; v!=NULL ; v = v->next )
		if ( !update(v,DXD_WRITE,xlate) )
			return -1;
		else
			count++;
	if ( count>0 )
	{
		gl_debug("write cache dump...");
		write_cache.dump();
		int64 id;
		//send outgoing data
		if ( client_initiated(
				MSG_INITIATE, 
				MSG_TAG,"method",tag,
				MSG_OPEN,"data",
					MSG_DATA,&write_cache,
				MSG_CLOSE,
				MSG_COMPLETE, &id,
				NULL)<0 )
			return -1;
		//recieve incoming data
		if ( server_response(
				MSG_INITIATE,
				MSG_TAG,"result",tag,
				MSG_OPEN,"data",
					MSG_DATA,&read_cache,
				MSG_CLOSE,
				MSG_COMPLETE, &id,
				NULL)<0 )
			return ignore_error>0;
		//update local variables with incoming cache values.
		count = 0;
		for ( v = varlist->getfirst() ; v!=NULL ; v = v->next )
			if ( !update(v,DXD_READ,xlate) )
				return -1;
			else
				count++;
		gl_debug("read cache dump...");
		read_cache.dump();
	}
	return count;
}

cacheitem *connection_mode::create_cache(VARMAP *map)
{
	if ( map->dir==DXD_READ ){
		cacheitem * item = read_cache.find_item(map);
		if(item != NULL){
			return item;
		} else{
			return read_cache.add_item(map);
		}
	} else if ( map->dir==DXD_WRITE ){
		cacheitem * item = write_cache.find_item(map);
		if(item != NULL){
			return item;
		} else{
			return write_cache.add_item(map);
		}
	} else return NULL;
}

int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, bool critical)
{
	if ( !transport->message_open() )
	{
		error("new message queue opened with unsent messages pending");
		return -1;
	}
	else
		debug(9,"message queue control: MSG_INITIATE (critical=%s)", critical?"yes":"no");
	transport->reset_fieldcount();
	xlate(transport,NULL,NULL,ET_GROUPOPEN,ETO_NONE);
	transport->reset_fieldcount();
	return 0;
}
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate)
{
	if ( !transport->message_continue() )
	{
		error("message queue continued with none open");
		return -1;
	}
	else
		debug(9,"message queue control: MSG_CONTINUE");
	return 0;
}
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, int64 *id)
{
	exchange(xlate,"id",(int64&)seqnum);
	if ( id!=NULL ) *id = seqnum;
	transport->reset_fieldcount();
	xlate(transport,NULL,NULL,ET_GROUPCLOSE,ETO_NONE);
	if ( !transport->message_close() )
	{
		error("message queue closed with none open");
		return -1;
	}
	else
		debug(9,"message queue control: MSG_COMPLETE (id=%lld)", id);
	return 0; // ok
}

// exchange a tag/value pair
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, const char *tag, char *value)
{
	return xlate(transport,tag,value,0,ETO_QUOTES);
}
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, const char *tag, size_t len, char *value)
{
	return xlate(transport,tag,value,len,ETO_QUOTES);
}
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, const char *tag, double &real)
{
	char temp[1024];
	int len = sprintf(temp,"%lg",real);
	int status = xlate(transport,tag,temp,len,ETO_NONE);
	if ( status>0 )
		status = sscanf(temp,"%lg",&real);
	return status;
}
int connection_mode::exchange(EXCHANGETRANSLATOR *xlate, const char *tag, int64 &integer)
{
	char temp[1024];
	int len = sprintf(temp,"%lld",integer);
	int status = xlate(transport,tag,temp,sizeof(temp),ETO_NONE);
	if ( status>0 )
		status = sscanf(temp,"%lld",&integer);
	return status;
}
int connection_mode::exchange_schema(EXCHANGETRANSLATOR *xlate, cache *list)
{
	int status;
	for (status=0 ; status<list->get_count() ; status++)
	{
		cacheitem * item = list->get_item(status);
		VARMAP *map = item->get_var();
		gld_property prop(map->obj->get_object(),strchr(map->local_name,'.')+1);
		gld_type type = prop.get_type();
		PROPERTYSPEC *spec = type.get_spec();
		char info[256];
		sprintf(info,"%s %s:%s", spec->name, prop.get_object()->name, prop.get_name());
		xlate(transport,map->remote_name,info,256,ETO_QUOTES);
	}
	return status;
}
int connection_mode::exchange_data(EXCHANGETRANSLATOR *xlate, cache *list)
{
	int n;
	for ( n=0 ; n<list->get_count() ; n++)
	{
		cacheitem *item = list->get_item(n);
		char * remote_name = item->get_remote();
		char * item_buffer = item->get_buffer();
		if ( !xlate(transport,item->get_remote(),item->get_buffer(),item->get_size(),ETO_QUOTES) )
			return 0;
	}
	return 1;
}
int connection_mode::exchange_open(EXCHANGETRANSLATOR *xlate, char *tag)
{
	int status = xlate(transport,tag,NULL,ET_GROUPOPEN,ETO_QUOTES);
	transport->reset_fieldcount();
	return status;
}
int connection_mode::exchange_close(EXCHANGETRANSLATOR *xlate)
{
	transport->reset_fieldcount();
	return xlate(transport,NULL,NULL,ET_GROUPCLOSE,ETO_NONE);
}

int connection_mode::send(char *buf, size_t len)
{
	return (int)transport->send(buf,len);
}
int connection_mode::recv(char *buf, size_t len)
{
	return (int)transport->recv(buf,len);
}

int connection_mode::server_response(MESSAGEFLAG flag,...)
{
	bool critical = flag==MSG_CRITICAL;
	int count;
	int msgcount = 0;
	int64 *id=NULL;
	bool stop = false;
	va_list flg;
	va_start(flg, flag);
	switch ( get_mode() ) {
	case CM_CLIENT:
		if ( recv()>0 )
		{
			debug(0,"receiving message: length=[%d] buffer=[%s]", transport->get_position(), transport->get_input());
			while ( stop == false )
			{
				msgcount++;
				switch ( flag ) {
				case MSG_CRITICAL:
					critical = true;
					debug(9,"connection_mode::exchange() message is critical");
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_INITIATE:
					if ( exchange(xlate_in,critical)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_CONTINUE:
					if ( exchange(xlate_in)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_COMPLETE:
					id = va_arg(flg,int64*);
					if ( exchange(xlate_in,id)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;

					}
					msgcount = 0; // signal message is complete and should be handled
					count = msgcount;
					stop = true;
					break;
				case MSG_SCHEMA:
				{
					DATAEXCHANGEDIRECTION dir = (DATAEXCHANGEDIRECTION)va_arg(flg,int);
					debug(9,"connection_mode::exchange(...) MSG_SCHEMA('%s',cache=0x%p)",dir==DXD_READ?"input":(dir==DXD_WRITE?"output":"invalid"),dir==DXD_READ?&read_cache:(dir==DXD_WRITE?&write_cache:NULL));
					if( dir==DXD_READ){
						if ( exchange_schema(xlate_out,&read_cache)<0 )
						{
							error("exchange(): schema exchange failed");
							count = -msgcount;
							stop = true;
						}
					} else if( dir == DXD_WRITE){
						if ( exchange_schema(xlate_out,&write_cache)<0 )
						{
							error("exchange(): schema exchange failed");
							count = -msgcount;
							stop = true;
						}
					} else {
						error("exchange(): schema exchange failed, invalid data direction supplied");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_TAG:
				{
					char *tag = va_arg(flg,char*);
					char *value = va_arg(flg,char*);
					debug(9,"connection_mode::exchange(...) MSG_TAG('%s','%s')", tag,value);
					if ( exchange(xlate_in,tag,value)<0 )
					{
						error("connection_mode::exchange(): tagged value exchange failed for ('%s','%s')",tag,value);
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_STRING:
				{
					char *tag = va_arg(flg,char*);
					size_t len = va_arg(flg,size_t);
					char *buf = va_arg(flg,char*);
					debug(9,"client_initiated(...) MSG_STRING('%s',%d,'%s')", tag,len,buf);
					if ( exchange(xlate_in,tag,len,buf)<0 )
					{
						error("connection_mode::exchange(): string value exchange failed for ('%s',%d,'%s')",tag,len,buf);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_REAL:
				{
					char *tag = va_arg(flg,char*);
					double *data = va_arg(flg,double*);
					debug(9,"exchange(...) MSG_REAL('%s',0x%p=%lf)",tag,data,*data);
					if ( exchange(xlate_in,tag,*data)<0 )
					{
						error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_INTEGER:
				{
					char *tag = va_arg(flg,char*);
					int64 *data = va_arg(flg,int64*);
					debug(9,"connection_mode::exchange(...) MSG_INTEGER('%s',0x%p=%ll)",tag,data,*data);
					if ( exchange(xlate_in,tag,*data)<0 )
					{
						error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_DATA:
				{
					cache *list = va_arg(flg,cache*);
					debug(9,"connection_mode::exchange(...) MSG_DATA(cache=0x%p)",list);
					if ( exchange_data(xlate_in,list)<0 )
					{
						error("connection_mode::exchange(): data exchange failed");
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_OPEN:
				{
					char *tag = va_arg(flg,char*);
					debug(9,"connection_mode::exchange(...) MSG_OPEN('%s')",tag);
					if ( exchange_open(xlate_in,tag)<0 )
					{
						error("connection_mode::exchange(): start group '%s' exchange failed",tag);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_CLOSE:
					debug(9,"connection_mode::exchange(...) MSG_CLOSE");
					if ( exchange_close(xlate_in)<0 )
					{
						error("connection_mode::exchange(): end group exchange failed");
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				default:
					warning("connection_mode::exchange(...) unrecognized message control flag %d was ignored", flag);
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
			}
			if(msgcount != 0){
				 count = msgcount>0?msgcount:-1; // empty stack is an error
			}
			va_end(flg);
			return count;
		}
		return critical?-1:0;
	case CM_SERVER:
		while ( stop == false )
		{
			msgcount++;
			switch ( flag ) {
			case MSG_CRITICAL:
				critical = true;
				debug(9,"connection_mode::exchange() message is critical");
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_INITIATE:
				seqnum++;
				if ( exchange(xlate_out,critical)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_CONTINUE:
				if ( exchange(xlate_out)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_COMPLETE:
				id = va_arg(flg,int64*);
				if ( exchange(xlate_out,id)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;

				}
				msgcount = 0; // signal message is complete and should be handled
				count = msgcount;
				stop = true;
				break;
			case MSG_SCHEMA:
			{
				DATAEXCHANGEDIRECTION dir = (DATAEXCHANGEDIRECTION)va_arg(flg,int);
				debug(9,"connection_mode::exchange(...) MSG_SCHEMA('%s',cache=0x%p)",dir==DXD_READ?"input":(dir==DXD_WRITE?"output":"invalid"),dir==DXD_READ?&read_cache:(dir==DXD_WRITE?&write_cache:NULL));
				if( dir==DXD_READ){
					if ( exchange_schema(xlate_out,&read_cache)<0 )
					{
						error("exchange(): schema exchange failed");
						count = -msgcount;
						stop = true;
					}
				} else if( dir == DXD_WRITE){
					if ( exchange_schema(xlate_out,&write_cache)<0 )
					{
						error("exchange(): schema exchange failed");
						count = -msgcount;
						stop = true;
					}
				} else {
					error("exchange(): schema exchange failed, invalid data direction supplied");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_TAG:
			{
				char *tag = va_arg(flg,char*);
				char *value = va_arg(flg,char*);
				debug(9,"connection_mode::exchange(...) MSG_TAG('%s','%s')", tag,value);
				if ( exchange(xlate_out,tag,value)<0 )
				{
					error("connection_mode::exchange(): tagged value exchange failed for ('%s','%s')",tag,value);
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_STRING:
			{
				char *tag = va_arg(flg,char*);
				size_t len = va_arg(flg,size_t);
				char *buf = va_arg(flg,char*);
				debug(9,"client_initiated(...) MSG_STRING('%s',%d,'%s')", tag,len,buf);
				if ( exchange(xlate_out,tag,len,buf)<0 )
				{
					error("connection_mode::exchange(): string value exchange failed for ('%s',%d,'%s')",tag,len,buf);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_REAL:
			{
				char *tag = va_arg(flg,char*);
				double *data = va_arg(flg,double*);
				debug(9,"exchange(...) MSG_REAL('%s',0x%p=%lf)",tag,data,*data);
				if ( exchange(xlate_out,tag,*data)<0 )
				{
					error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_INTEGER:
			{
				char *tag = va_arg(flg,char*);
				int64 *data = va_arg(flg,int64*);
				debug(9,"connection_mode::exchange(...) MSG_INTEGER('%s',0x%p=%ll)",tag,data,*data);
				if ( exchange(xlate_out,tag,*data)<0 )
				{
					error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_DATA:
			{
				cache *list = va_arg(flg,cache*);
				debug(9,"connection_mode::exchange(...) MSG_DATA(cache=0x%p)",list);
				if ( exchange_data(xlate_out,list)<0 )
				{
					error("connection_mode::exchange(): data exchange failed");
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_OPEN:
			{
				char *tag = va_arg(flg,char*);
				debug(9,"connection_mode::exchange(...) MSG_OPEN('%s')",tag);
				if ( exchange_open(xlate_out,tag)<0 )
				{
					error("connection_mode::exchange(): start group '%s' exchange failed",tag);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_CLOSE:
				debug(9,"connection_mode::exchange(...) MSG_CLOSE");
				if ( exchange_close(xlate_out)<0 )
				{
					error("connection_mode::exchange(): end group exchange failed");
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			default:
				warning("connection_mode::exchange(...) unrecognized message control flag %d was ignored", flag);
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
		}
		va_end(flg);
		if ( count==0 )
		{
			debug(0,"sending message: length=[%d] buffer=[%s]", transport->get_position(), transport->get_input());
			return send()==transport->get_position();
		}
		return count<0?(critical?-1:0):0;
	default:
		error("server_response(...): connection mode not valid");
		return critical?-1:0;
	}
}
int connection_mode::client_initiated(MESSAGEFLAG flag,...)
{
	bool critical = flag==MSG_CRITICAL;
	int count;
	int msgcount = 0;
	int64 *id=NULL;
	bool stop = false;
	va_list flg;
	va_start(flg, flag);
	switch ( get_mode() ) {
	case CM_CLIENT:
		while ( stop == false )
		{
			msgcount++;
			switch ( flag ) {
			case MSG_CRITICAL:
				critical = true;
				debug(9,"connection_mode::exchange() message is critical");
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_INITIATE:
				seqnum++;
				if ( exchange(xlate_out,critical)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_CONTINUE:
				if ( exchange(xlate_out)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			case MSG_COMPLETE:
				id = va_arg(flg,int64*);
				if ( exchange(xlate_out,id)<0 )
				{
					error("connection_mode::exchange(): transport status control error");
					count = -msgcount;
					stop = true;

				}
				msgcount = 0; // signal message is complete and should be handled
				count = msgcount;
				stop = true;
				break;
			case MSG_SCHEMA:
			{
				DATAEXCHANGEDIRECTION dir = (DATAEXCHANGEDIRECTION)va_arg(flg,int);
				debug(9,"connection_mode::exchange(...) MSG_SCHEMA('%s',cache=0x%p)",dir==DXD_READ?"input":(dir==DXD_WRITE?"output":"invalid"),dir==DXD_READ?&read_cache:(dir==DXD_WRITE?&write_cache:NULL));
				if( dir==DXD_READ){
					if ( exchange_schema(xlate_out,&read_cache)<0 )
					{
						error("exchange(): schema exchange failed");
						count = -msgcount;
						stop = true;
					}
				} else if( dir == DXD_WRITE){
					if ( exchange_schema(xlate_out,&write_cache)<0 )
					{
						error("exchange(): schema exchange failed");
						count = -msgcount;
						stop = true;
					}
				} else {
					error("exchange(): schema exchange failed, invalid data direction supplied");
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_TAG:
			{
				char *tag = va_arg(flg,char*);
				char *value = va_arg(flg,char*);
				debug(9,"connection_mode::exchange(...) MSG_TAG('%s','%s')", tag,value);
				if ( exchange(xlate_out,tag,value)<0 )
				{
					error("connection_mode::exchange(): tagged value exchange failed for ('%s','%s')",tag,value);
					count = -msgcount;
					stop = true;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_STRING:
			{
				char *tag = va_arg(flg,char*);
				size_t len = va_arg(flg,size_t);
				char *buf = va_arg(flg,char*);
				debug(9,"client_initiated(...) MSG_STRING('%s',%d,'%s')", tag,len,buf);
				if ( exchange(xlate_out,tag,len,buf)<0 )
				{
					error("connection_mode::exchange(): string value exchange failed for ('%s',%d,'%s')",tag,len,buf);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_REAL:
			{
				char *tag = va_arg(flg,char*);
				double *data = va_arg(flg,double*);
				debug(9,"exchange(...) MSG_REAL('%s',0x%p=%lf)",tag,data,*data);
				if ( exchange(xlate_out,tag,*data)<0 )
				{
					error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_INTEGER:
			{
				char *tag = va_arg(flg,char*);
				int64 *data = va_arg(flg,int64*);
				debug(9,"connection_mode::exchange(...) MSG_INTEGER('%s',0x%p=%ll)",tag,data,*data);
				if ( exchange(xlate_out,tag,*data)<0 )
				{
					error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_DATA:
			{
				cache *list = va_arg(flg,cache*);
				debug(9,"connection_mode::exchange(...) MSG_DATA(cache=0x%p)",list);
				if ( exchange_data(xlate_out,list)<0 )
				{
					error("connection_mode::exchange(): data exchange failed");
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_OPEN:
			{
				char *tag = va_arg(flg,char*);
				debug(9,"connection_mode::exchange(...) MSG_OPEN('%s')",tag);
				if ( exchange_open(xlate_out,tag)<0 )
				{
					error("connection_mode::exchange(): start group '%s' exchange failed",tag);
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
			case MSG_CLOSE:
				debug(9,"connection_mode::exchange(...) MSG_CLOSE");
				if ( exchange_close(xlate_out)<0 )
				{
					error("connection_mode::exchange(): end group exchange failed");
					count = -msgcount;
					stop = true;
					break;
				}
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			default:
				warning("connection_mode::exchange(...) unrecognized message control flag %d was ignored", flag);
				if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
					stop = true;
				}
				break;
			}
		}
		if(msgcount != 0){
		     count = msgcount>0?msgcount:-1; // empty stack is an error
		}
		va_end(flg);
		if ( count==0 )
		{
			debug(0,"sending message: length=[%d] buffer=[%s]", transport->get_position(), transport->get_input());
			return send()==transport->get_position();
		}
		return count<0?(critical?-1:0):0;
	case CM_SERVER:
		if ( recv()>0 )
		{
			debug(0,"receiving message: length=[%d] buffer=[%s]", transport->get_position(), transport->get_input());
			while ( stop == false )
			{
				msgcount++;
				switch ( flag ) {
				case MSG_CRITICAL:
					critical = true;
					debug(9,"connection_mode::exchange() message is critical");
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_INITIATE:
					if ( exchange(xlate_in,critical)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_CONTINUE:
					if ( exchange(xlate_in)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				case MSG_COMPLETE:
					id = va_arg(flg,int64*);
					if ( exchange(xlate_in,id)<0 )
					{
						error("connection_mode::exchange(): transport status control error");
						count = -msgcount;
						stop = true;

					}
					msgcount = 0;
					count = msgcount; // signal message is complete and should be handled
					stop = true;
					break;
				case MSG_SCHEMA:
				{
					DATAEXCHANGEDIRECTION dir = (DATAEXCHANGEDIRECTION)va_arg(flg,int);
					debug(9,"connection_mode::exchange(...) MSG_SCHEMA('%s',cache=0x%p)",dir==DXD_READ?"input":(dir==DXD_WRITE?"output":"invalid"),dir==DXD_READ?&read_cache:(dir==DXD_WRITE?&write_cache:NULL));
					if( dir==DXD_READ){
						if ( exchange_schema(xlate_out,&read_cache)<0 )
						{
							error("exchange(): schema exchange failed");
							count = -msgcount;
							stop = true;
						}
					} else if( dir == DXD_WRITE){
						if ( exchange_schema(xlate_out,&write_cache)<0 )
						{
							error("exchange(): schema exchange failed");
							count = -msgcount;
							stop = true;
						}
					} else {
						error("exchange(): schema exchange failed, invalid data direction supplied");
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_TAG:
				{
					char *tag = va_arg(flg,char*);
					char *value = va_arg(flg,char*);
					debug(9,"connection_mode::exchange(...) MSG_TAG('%s','%s')", tag,value);
					if ( exchange(xlate_in,tag,value)<0 )
					{
						error("connection_mode::exchange(): tagged value exchange failed for ('%s','%s')",tag,value);
						count = -msgcount;
						stop = true;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_STRING:
				{
					char *tag = va_arg(flg,char*);
					size_t len = va_arg(flg,size_t);
					char *buf = va_arg(flg,char*);
					debug(9,"client_initiated(...) MSG_STRING('%s',%d,'%s')", tag,len,buf);
					if ( exchange(xlate_in,tag,len,buf)<0 )
					{
						error("connection_mode::exchange(): string value exchange failed for ('%s',%d,'%s')",tag,len,buf);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_REAL:
				{
					char *tag = va_arg(flg,char*);
					double *data = va_arg(flg,double*);
					debug(9,"exchange(...) MSG_REAL('%s',0x%p=%lf)",tag,data,*data);
					if ( exchange(xlate_in,tag,*data)<0 )
					{
						error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_INTEGER:
				{
					char *tag = va_arg(flg,char*);
					int64 *data = va_arg(flg,int64*);
					debug(9,"connection_mode::exchange(...) MSG_INTEGER('%s',0x%p=%ll)",tag,data,*data);
					if ( exchange(xlate_in,tag,*data)<0 )
					{
						error("connection_mode::exchange(): double value exchange failed for ('%s',0x%p=%lf)",tag,data,*data);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_DATA:
				{
					cache *list = va_arg(flg,cache*);
					debug(9,"connection_mode::exchange(...) MSG_DATA(cache=0x%p)",list);
					if ( exchange_data(xlate_in,list)<0 )
					{
						error("connection_mode::exchange(): data exchange failed");
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_OPEN:
				{
					char *tag = va_arg(flg,char*);
					debug(9,"connection_mode::exchange(...) MSG_OPEN('%s')",tag);
					if ( exchange_open(xlate_in,tag)<0 )
					{
						error("connection_mode::exchange(): start group '%s' exchange failed",tag);
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
				case MSG_CLOSE:
					debug(9,"connection_mode::exchange(...) MSG_CLOSE");
					if ( exchange_close(xlate_in)<0 )
					{
						error("connection_mode::exchange(): end group exchange failed");
						count = -msgcount;
						stop = true;
						break;
					}
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				default:
					warning("connection_mode::exchange(...) unrecognized message control flag %d was ignored", flag);
					if((flag = (MESSAGEFLAG)va_arg(flg, int))==NULL){
						stop = true;
					}
					break;
				}
			}
			if(msgcount != 0){
			     count = msgcount>0?msgcount:-1; // empty stack is an error
			}
			va_end(flg);
			return count;
		}
		return critical?-1:0;
	default:
		error("client_initiated(...): connection mode not valid");
		return critical?-1:0;
	}
}
