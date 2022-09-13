/// $Id$
///
/// Handles the interface between a server or client and underlying transport
///
#include "connection.h"
#include "udp.h"
#include "tcp.h"

////////////////////////////////////////////////////////////////////////////////////
connection_transport::connection_transport(void)
{
	maxmsg = 1500;
	memset(output,0,sizeof(output));
	position = -1;
	field_count = 0;
	delimiter = NULL;

	memset(input,0,sizeof(input));
	translation = NULL;
	translator = NULL;

	on_error = TE_RETRY;
	maxretry = 5;
}

connection_transport::~connection_transport(void)
{
	if ( position>0 )
		warning("transport closed before pending message could be sent");
}

CONNECTIONTRANSPORT connection_transport::get_transport(const char *s)
{
	if ( strcmp(s,"udp")==0 ) return CT_UDP;
	else if ( strcmp(s,"tcp")==0 ) return CT_TCP;
	else return CT_NONE;
}

connection_transport* connection_transport::new_instance(CONNECTIONTRANSPORT e)
{
	connection_transport *t;
	switch ( e ) {
	case CT_UDP: 
		t = (connection_transport*)new udp();
		break;
	case CT_TCP: 
		t = (connection_transport*)new tcp();
		break;
	default: 
		gl_error("invalid transport type");
		return NULL;
	}
	if ( !t->create() )
	{
		delete t;
		return NULL;
	}
	else
		return t;
}

void connection_transport::error(const char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_error("connection/%s: %s",get_transport_name(), msg);
}
void connection_transport::warning(const char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_warning("connection/%s: %s",get_transport_name(), msg);
}
void connection_transport::info(const char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_output("connection/%s: %s",get_transport_name(), msg);
}
void connection_transport::debug(int level, const char *fmt, ...)
{
	char msg[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg,fmt,ptr);
	va_end(ptr);
	gl_debug("connection/%s: %s",get_transport_name(), msg);
}
void connection_transport::exception(const char *fmt, ...)
{
	static char msg[1024];
	int len = sprintf(msg,"connection/%s: ", get_transport_name());
	va_list ptr;
	va_start(ptr,fmt);
	vsprintf(msg+len,fmt,ptr);
	va_end(ptr);
	throw msg;
}

bool connection_transport::message_open()
{
	//if ( position>0 )
	//{
	//	error("message open received with message still pending");
	//	return false;
	//}
	memset(output,0,sizeof(output));
	position = 0;
	return true;
}

bool connection_transport::message_close()
{
	//if ( position<0 )
	//{
	//	error("message close received with no pending message");
	//	return false;
	//}
	//position = -1;
	return true;
}

bool connection_transport::message_continue()
{
	//if ( position<0 )
	//{
	//	error("message continue received with no pending message");
	//	return false;
	//}
	return true;
}

int connection_transport::message_append(const char *fmt,...)
{
	char temp[2048];
	va_list ptr;
	va_start(ptr,fmt);
	if ( get_position()<0 )
	{
		error("message append received with no pending message");
		return -1;
	}
	int len = vsprintf(temp,fmt,ptr);
	if ( len>get_size() )
	{
		error("message exceeds protocol size limit");
		return -1;
	}
	if ( delimiter!=NULL && field_count>0 )
	{
		if ( get_size()<strlen(delimiter) )
		{
			error("message exceeds protocol size limit");
			return -1;
		}
		position += sprintf(output+position,"%s",delimiter);
	}
	strncpy(output+position,temp,len);
	position += len;
	field_count++;
	return len;
}
