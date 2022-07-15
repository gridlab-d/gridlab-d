/** $Id$

   Object class implementation

 **/

#include <cstdlib>

#include "xml.h"

EXPORT_CREATE(xml);
EXPORT_INIT(xml);
EXPORT_PRECOMMIT(xml);
EXPORT_SYNC(xml);
EXPORT_COMMIT(xml);
EXPORT_FINALIZE(xml);
EXPORT_NOTIFY(xml);
EXPORT_PLC(xml);
EXPORT_LOADMETHOD(xml,link);
EXPORT_LOADMETHOD(xml,option);

CLASS *xml::oclass = NULL;
xml *xml::defaults = NULL;

int xml_translate(char *local, size_t local_len, char *remote, size_t remote_len, TRANSLATIONFLAG flag, ...)
{
	va_list ptr;
	va_start(ptr,flag);

	if ( flag==TF_DATA ) 
	{
		VARMAP *var = va_arg(ptr,VARMAP*);
		if ( var->dir==DXD_READ ) // from remote to local
		{
			char name[1024];
			char value[1024];
			size_t len;
			if ( sscanf(remote,"<property><name>%1024[^<]</name><value>%1024[^<]</value></property>",name,value)!=2 )
			{
				gl_error("xml_translate(char *remote='%s',VARMAP *var->local_name='%s'): remote data format is not correct",remote,var->local_name);
				return 0;
			}
			else if ( (len=strlen(value))>local_len )
			{
				gl_error("xml_translate(char *remote='%s',VARMAP *var->local_name='%s'): remote data too big for local value",remote,var->local_name);
				return 0;
			}
			else if ( strcmp(name,var->local_name)!=0 )
			{
				gl_error("xml_translate(char *remote='%s',VARMAP *var->local_name='%s'): local name does not match remote data",remote,var->local_name);
				return 0;
			}
			else
			{
				strcpy(local,value);
				// TODO recursively process remaining payload in remote if varmap contains a next item
				return (int)len;
			}
		}
		else if ( var->dir==DXD_WRITE ) // from local to remote
		{
			return sprintf(remote,"<property><name>%s</name><value>%s</value></property>",var->remote_name,local);
		}
		else
			throw "xml_translate(...): invalid DATAEXCHANGEDIR";
	}
	else if ( flag==TF_SCHEMA )
	{
		return -1; // TODO implement schema translations
	}
	else if ( flag==TF_TUPLE )
	{
		return -1; // TODO implement tuple translation
	}
	else
		throw "xml_translate(...): invalid flags";
	va_end(ptr);
}

xml::xml(MODULE *module) : native(module)
{
	// register to receive notice for first top down. bottom up, and second top down synchronizations
	oclass = gld_class::create(module,"xml",sizeof(xml),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
	if (oclass==NULL)
		throw "connection/xml::xml(MODULE*): unable to register class connection:xml";
	else
		oclass->trl = TRL_UNKNOWN;

	defaults = this;
	if (gl_publish_variable(oclass,
		PT_INHERIT, "native",
		PT_enumeration, "encoding", get_encoding_offset(), PT_DESCRIPTION, "XML UTF encoding",
			PT_KEYWORD, "UTF8", (enumeration)UTF8,
			PT_KEYWORD, "UTF16", (enumeration)UTF16,
		PT_char8, "version", get_version_offset(), PT_DESCRIPTION, "XML version",
		PT_char1024, "schema", get_schema_offset(), PT_DESCRIPTION, "XSD url",
		PT_char1024, "stylesheet", get_stylesheet_offset(), PT_DESCRIPTION, "XSL url",
		// TODO add published properties here
		NULL)<1)
			throw "connection/xml::xml(MODULE*): unable to publish properties of connection:xml";

	if ( !gl_publish_loadmethod(oclass, "link", reinterpret_cast<int (*)(void *, char *)>(loadmethod_xml_link)) )
		throw "connection/xml::xml(MODULE*): unable to publish link method of connection:xml";
	if ( !gl_publish_loadmethod(oclass, "option", reinterpret_cast<int (*)(void *, char *)>(loadmethod_xml_option)) )
		throw "connection/xml::xml(MODULE*): unable to publish option method of connection:xml";
	encoding = UTF8;
}

int xml::create(void) 
{
	strcpy(version,"1.0");
	sprintf(schema,"http://www.gridlabd.org/gridlabd-%d_%d.xsd",gl_version_major(),gl_version_minor());
	sprintf(stylesheet,"http://www.gridlabd.org/gridlabd-%d_%d.xsl",gl_version_major(),gl_version_minor());
	return native::create();
}

int xml::init(OBJECT *parent)
{
	if ( get_connection()==NULL )
	{
		error("connection options not specified");
		return 0;
	}

	char appname[1024] = PACKAGE;
	char appversion[1024];
	sprintf(appversion,"%d.%d.%d-%d (%s)", gl_version_major(),gl_version_minor(), gl_version_patch(), gl_version_build(), gl_version_branch()); // TODO read from core
	int id = get_connection()->client_initiated(
		MSG_CRITICAL,
		MSG_INITIATE, 
			"app-name",(const char*)appname,
			"app-version",(const char*)appversion,
			"xml-version",(const char*)version,
			"xml-schema",(const char*)schema,
			"xml-stylesheet",(const char*)stylesheet,
			NULL, // required to end tag/value list
		MSG_COMPLETE, &id,
		NULL);
	if ( id<0 )
		error("client initiated initial message exchange failed");
	if ( get_connection()->server_response(			
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_CONTINUE,
		NULL)<0
// TODO FIX THIS:		|| get_connection()->server_response("id","%d",id)<0
		|| get_connection()->server_response(MSG_COMPLETE,&id,NULL)<0 )
	{
		error("server response initial message exchange failed");
		return 0;
	}
	else
		return native::init(parent,&xml_translate);
}

int xml::link(char *value)
{
	return native::link(value);
}

int xml::option(char *value)
{
	char target[256];
	char command[1024];

	// parse the pseudo-property
	if ( sscanf(value,"%[^:]:%[^\n]", target, command)==2 )
	{
		gl_verbose("connection/xml::option(char *value='%s') parsed ok", value);
		return native::option(target,command);
	}
	else
	{
		gl_error("connection/xml::option(char *value='%s'): unable to parse option argument", value);
		return 0;
	}
}

int xml::precommit(TIMESTAMP t)
{
	return native::precommit(t);
}

TIMESTAMP xml::presync(TIMESTAMP t)
{
	return native::presync(t);
}

TIMESTAMP xml::sync(TIMESTAMP t)
{
	return native::sync(t);
}

TIMESTAMP xml::postsync(TIMESTAMP t)
{
	return native::postsync(t);
}

TIMESTAMP xml::commit(TIMESTAMP t0,TIMESTAMP t1)
{
	return native::commit(t0,t1);
}

int xml::prenotify(PROPERTY *p,char *v)
{
	return native::prenotify(p,v);
}

int xml::postnotify(PROPERTY *p,char *v)
{
	return native::postnotify(p,v);
}

int xml::finalize()
{
	return native::finalize();
}

TIMESTAMP xml::plc(TIMESTAMP t)
{
	return native::plc(t);
}

void xml::term(TIMESTAMP t)
{
	return native::term(t);
}
