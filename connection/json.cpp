/** $Id$

   Object class implementation

 **/

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <gld_complex.h>

#include "json.h"

EXPORT_CREATE(json);
EXPORT_INIT(json);
EXPORT_PRECOMMIT(json);
EXPORT_SYNC(json);
EXPORT_COMMIT(json);
EXPORT_FINALIZE(json);
EXPORT_NOTIFY(json);
EXPORT_PLC(json);
EXPORT_LOADMETHOD(json,link);
EXPORT_LOADMETHOD(json,option);

CLASS *json::oclass = NULL;
json *json::defaults = NULL;

// translates data to cache
int json_translate(char *local, size_t local_len, char *remote, size_t remote_len, TRANSLATIONFLAG flag, ...)
{
	throw "obsolete code used";
	va_list ptr;
	va_start(ptr,flag);
	if ( flag==TF_DATA ) // default is data
	{
		VARMAP *var = va_arg(ptr,VARMAP*);
		if ( var->dir==DXD_READ ) // from remote to local
		{
			char name[1024];
			char value[1024];
			size_t len;
			if ( sscanf(remote,"<property><name>%1024[^<]</name><value>%1024[^<]</value></property>",name,value)!=2 )
			{
				gl_error("json_translate(char *remote='%s',VARMAP *var->local_name='%s'): remote data format is not correct",remote,var->local_name);
				return 0;
			}
			else if ( (len=strlen(value))>local_len )
			{
				gl_error("json_translate(char *remote='%s',VARMAP *var->local_name='%s'): remote data too big for local value",remote,var->local_name);
				return 0;
			}
			else if ( strcmp(name,var->local_name)!=0 )
			{
				gl_error("json_translate(char *remote='%s',VARMAP *var->local_name='%s'): local name does not match remote data",remote,var->local_name);
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
			throw "json_translate(...): invalid DATAEXCHANGEDIR";
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
		throw "json_translate(...): invalid flags";
	va_end(ptr);
}

json::json(MODULE *module) : native(module)
{
	// register to receive notice for first top down. bottom up, and second top down synchronizations
	oclass = gld_class::create(module,"json",sizeof(json),PC_AUTOLOCK|PC_PRETOPDOWN|PC_BOTTOMUP|PC_POSTTOPDOWN|PC_OBSERVER);
	if (oclass==NULL)
		throw "connection/json::json(MODULE*): unable to register class connection:json";
	else
		oclass->trl = TRL_UNKNOWN;

	defaults = this;
	if (gl_publish_variable(oclass,
		PT_INHERIT, "native",
		PT_double, "version", get_version_offset(), PT_DESCRIPTION, "json version",
		// TODO add published properties here
		NULL)<1)
			throw "connection/json::json(MODULE*): unable to publish properties of connection:json";

	if ( !gl_publish_loadmethod(oclass, "link", reinterpret_cast<int (*)(void *, char *)>(loadmethod_json_link)) )
		throw "connection/json::json(MODULE*): unable to publish link method of connection:json";
	if ( !gl_publish_loadmethod(oclass, "option", reinterpret_cast<int (*)(void *, char *)>(loadmethod_json_option)) )
		throw "connection/json::json(MODULE*): unable to publish option method of connection:json";
}

// export data to json
int json_export(connection_transport *transport,
				const char *tag,		// NULL for group close, name for group open (or NULL if none)
				char *v,		// NULL for group open/close
				size_t vlen,	// 0 for match only on read (ignored on export)
								// -1 for group begin
								// -2 for group end
				int options)	// ETO_QUOTES enclose value in quotes
{
	if ( v==NULL ) // group control only
	{
		if ( transport->get_size()<2 )
		{
			transport->error("json_export(): export buffer overrun");
			return -1;
		}
		switch ( vlen ) {
		case ET_GROUPOPEN:
			if ( tag==NULL )
				return transport->message_append("%s","{");
			else
				return transport->message_append("\"%s\": {",tag);
			break;
		case ET_GROUPCLOSE:
			return transport->message_append("%s","}");
			break;
		default:
			transport->error("json_export(): invalid group control code %d", vlen);
			return -1; // invalid group control code
		}
	}
	else
	{
		if ( vlen>transport->get_size() ) 
		{	
			transport->error("json_export(): export buffer overrun");
			return -1; // refuse to overrun output buffer
		}
		else if (options&ETO_QUOTES)
			return transport->message_append(R"("%s": "%s")", tag, v);
		else
			return transport->message_append("\"%s\": %s", tag, v);
	}
}

void *json_translator(char *buffer, void *translation=NULL)
{
	if ( translation!=NULL )
		json::destroy((JSONLIST*)translation);
	return json::parse(buffer);
}
// import data from json
int json_import(connection_transport *transport,
				const char *tag,		// NULL for group close, name for group open (or NULL if none)
				char *v,		// NULL for group open/close
				size_t vlen,	// 0 for match only on read (ignored on export)
								// -1 for group begin
								// -2 for group end
				int options)	// ETO_QUOTES enclose value in quotes
{
	// test sequence
	//JSONLIST *data = json::parse("{\"method\" : \"init\", \"params\" : {\"application\" : \"gridlabd\", \"version\" : 3.0, \"modelname\" : \"modelname.glm\"}, \"id\" : 1}");
	//char *method = json::get(data,"method");
	//char *id = json::get(data,"id");
	//char *version = json::get(data,"version");
	//char *application = json::get(data,"application");
	//char *modelname = json::get(data,"modelname");
	//gl_debug("method=%s; id=%s; version=%s; application=%s; modelname=%s", method,id,version,application,modelname);
	//JSONLIST *params = json::find(data,"params");
	//version = json::get(params,"version");
	//application = json::get(params,"application");
	//modelname = json::get(params,"modelname");
	//gl_debug("method=%s; id=%s; version=%s; application=%s; modelname=%s", method,id,version,application,modelname);
	//	JSONLIST *data = json::parse(transport->get_input());
	// TODO extract data
	JSONLIST *translation = (JSONLIST*)transport->get_translation();
	if ( translation==NULL )
	{
		transport->set_translator(json_translator);
		translation = (JSONLIST*)json_translator(transport->get_input());
		transport->set_translation(translation);
	}
	if ( tag==NULL ) // ignore grouping calls
		return 1;
	char *value = json::get(translation,tag);
	if ( value==NULL )
	{
		gl_error("json_import(tag='%s',...) tag not found in incoming data",tag);
		return 0;
	}
	if ( v==NULL ) // tag check (done)
		return 1;
	if ( vlen==0 ) // compare only
	{	
		if ( strcmp(v,value)==0 )
			return 1;
		gl_error("json_import(tag='%s',...) incoming value '%s' did not match expected/required value '%s'",tag,value,v);
		return 0;
	}
	else if ( strlen(value)<vlen )
	{
		strcpy(v,value);
		return 1;
	}
	else
	{
		gl_error("json_import(tag='%s',...) incoming longer than %d bytes allowed",tag,vlen);
		return 0;
	}
}

int json::create(void) 
{
	version = 1.0;
	return native::create();
}

int json::init(OBJECT *parent)
{
	native::init(parent,&json_translate);

	if ( get_connection()==NULL )
	{
		error("connection options not specified");
		return 0;
	}

	get_connection()->set_translators(json_export, json_import, json_translate);
	get_connection()->get_transport()->set_delimiter(", ");
	get_connection()->get_transport()->set_message_format("JSON");
	get_connection()->get_transport()->set_message_version(1.0);
	if ( get_connection()->init()==0 )
		return 0;

	// handshake exchange
	char appname[1024] = PACKAGE;
	double appversion = MAJOR + 0.1*MINOR;
	gld_global modelname("modelname");
	char name[1024];
	modelname.to_string(name,sizeof(name)-1);
	int64 id;
	if ( get_connection()->client_initiated(
			MSG_CRITICAL,
			MSG_INITIATE, 
				MSG_TAG, "method", "init",
				MSG_OPEN, "params",
					MSG_STRING, "application",sizeof(appname)-1,appname,
					MSG_REAL, "version",&appversion,
					MSG_STRING, "modelname",sizeof(name),name,
				MSG_CLOSE,
			MSG_COMPLETE, &id,
			NULL)<0 )
	{
		error("handshake initiation failed");
		return 0;
	}
	if ( get_connection()->server_response(
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_TAG,"result","init",
		MSG_COMPLETE, &id,
		NULL)<0 )
	{
		error("handshake response failed");
		return 0;
	}

	// input schema exchange
	if( get_connection()->client_initiated(
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_TAG,"method","input",
		MSG_OPEN,"schema",
		MSG_SCHEMA,DXD_READ,
		MSG_CLOSE,
		MSG_COMPLETE,  &id,
		NULL)<0 )
	{
		error("schema exchange failed");
		return 0;
	}
	if ( get_connection()->server_response(
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_TAG,"result","input",
		MSG_COMPLETE, &id,
		NULL)<0 )
	{
		error("handshake response failed");
		return 0;
	}

	// output schema exchange
	if( get_connection()->client_initiated(
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_TAG,"method","output",
		MSG_OPEN,"schema",
		MSG_SCHEMA,DXD_WRITE,
		MSG_CLOSE,
		MSG_COMPLETE, &id,
		NULL)<0 )
	{
		error("schema exchange failed");
		return 0;
	}
	if ( get_connection()->server_response(
		MSG_CRITICAL,
		MSG_INITIATE, 
		MSG_TAG,"result","output",
		MSG_COMPLETE, &id,
		NULL)<0 )
	{
		error("handshake response failed");
		return 0;
	}

	// first update
	return get_connection()->update(get_initmap(),"start",&json_translate)>=0;
}

int json::link(char *value)
{
	return native::link(value);
}

int json::option(char *value)
{
	char target[256];
	char command[1024];

	// parse the pseudo-property
	if ( sscanf(value,"%[^:]:%[^\n]", target, command)==2 )
	{
		gl_verbose("connection/json::option(char *value='%s') parsed ok", value);
		return native::option(target,command);
	}
	else
	{
		gl_error("connection/json::option(char *value='%s'): unable to parse option argument", value);
		return 0;
	}
}

int json::precommit(TIMESTAMP t)
{
	return native::precommit(t);
}

TIMESTAMP json::presync(TIMESTAMP t)
{
	return native::presync(t);
}

TIMESTAMP json::sync(TIMESTAMP t)
{
	return native::sync(t);
}

TIMESTAMP json::postsync(TIMESTAMP t)
{
	return native::postsync(t);
}

TIMESTAMP json::commit(TIMESTAMP t0,TIMESTAMP t1)
{
	return native::commit(t0,t1);
}

int json::prenotify(PROPERTY *p,char *v)
{
	return native::prenotify(p,v);
}

int json::postnotify(PROPERTY *p,char *v)
{
	return native::postnotify(p,v);
}

int json::finalize(void)
{
	return native::finalize();
}

TIMESTAMP json::plc(TIMESTAMP t)
{
	return native::plc(t);
}

void json::term(TIMESTAMP t)
{
	return native::term(t);
}

////////////////////////////////////////////////////////////////////////////
// JSON STATE MACHINE

// create a new list
static JSONLIST *json_new_list(JSONLIST *tail=NULL)
{
	JSONLIST *list = new JSONLIST;
	memset(list,0,sizeof(JSONLIST));
	if ( tail!=NULL ) {
		tail->next = list;
		list->parent = tail->parent;
	}
	return list;
}

// create a new sublist
static JSONLIST *json_new_sublist(JSONLIST *parent)
{
	JSONLIST *list = json_new_list();
	list->parent = parent;
	parent->list = list;
	return list;
}

// read number from string buffer
static void json_read_number(JSONLIST *item)
{
	if ( item->type==JT_REAL )
		item->real = atof(item->string);
	else if ( item->type==JT_INTEGER )
		item->integer = atoi64(item->string);
	else
		gl_warning("json_read_number(JSONLIST *item={string:'%s',...}): ignoring attempt to read non-number type", item->string);
}

// dump json structure (debug only)
static void json_dump(JSONLIST *item, unsigned int indent=0)
{
	char *indentchars = new char[indent+1];
	memset(indentchars,32,indent+1);
	indentchars[indent]='\0';
	if ( indent==0 ) gl_debug("{");
	for ( ; item!=NULL ; item=item->next )
	{
		if ( item->type==JT_LIST )
		{
			gl_debug(" %s\"%s\" : {",indentchars,item->tag);
			json_dump(item->list,indent+1);
			gl_debug(" %s}",indentchars);
		}
		else if ( item->type==JT_STRING )
			gl_debug(" %s\"%s\" : \"%s\"%s",indentchars,item->tag,item->string,item->next==NULL?"":",");
		else if ( item->type==JT_VOID )
			gl_debug(" %s\"%s\" : (void)%s",indentchars,item->tag,item->next==NULL?"":",");
		else
			gl_debug(" %s\"%s\" : %s%s",indentchars,item->tag,item->string,item->next==NULL?"":",");
	}
	if ( indent==0 ) gl_debug("}");
	delete indentchars;
}

// parse string into json structure
JSONLIST *json::parse(char *buffer)
{
	JSONLIST *head = json_new_list();
	JSONLIST *tail = head;
	tail->next = NULL;
	int nest = 0;
	enum {START, TAG0, TAG, COLON, PARAM, STRING, NUMBER, COMMA, END} state = START;
	char *p, *t;
	for ( p=buffer ; *p!='\0' ; p++ )
	{
		switch ( state ) {
		case START: 
			// white	ignore 
			// [{]		nest++, TAG0 
			// *		reject
			if ( isspace(*p) ) {}
			else if ( *p=='{' ) { nest++; state=TAG0; }
			else goto Syntax;
			break;
		case TAG0: 
			// white	ignore
			// ["]		TAG1
			// *		reject
			if ( isspace(*p) ) {}
			else if ( *p=='"' ) { state=TAG; t=tail->tag; /* TODO check tag length */ }
			else goto Syntax;
			break;
		case TAG: 
			// ["]		save tag, COLON
			// *		accept
			if ( *p=='"' ) { *t='\0'; state=COLON; }
			else { *t++=*p; }
			break;
		case COLON: 
			// white	ignore
			// [:]		PARAM
			// *		reject
			if ( isspace(*p) ) {}
			else if ( *p==':' ) { state=PARAM; t=tail->string; }
			else goto Syntax;
			break;
		case PARAM: 
			// white	ignore
			// ["]		STRING
			// [-+0-9.] accept, NUMBER
			// [{]		start list, next++, TAG0
			// *		reject
			if ( isspace(*p) ) {}
			else if ( *p=='"' ) { state=STRING; tail->type=JT_STRING; }
			else if ( isdigit(*p) || *p=='-' || *p=='+' || *p=='.' ) { *t++=*p; state=NUMBER; tail->type=JT_INTEGER; }
			else if ( *p=='{' ) { nest++; state=TAG0; tail->type=JT_LIST; tail=json_new_sublist(tail); }
			else goto Syntax;
			break;
		case STRING: 
			// ["]		save param, COMMA
			// *		accept
			if ( *p=='"' ) { *t='\0'; state=COMMA; }
			else { *t++=*p; }
			break;
		case NUMBER: // same as above but without sign
			// [:w,}]	save param, COMMA, put back
			// [.]		accept, promote
			// [-+0-9]  accept
			// *		reject
			if ( isspace(*p) || *p==',' || *p=='}' ) { *t='\0'; p--; state=COMMA; json_read_number(tail); }
			else if ( tail->type==JT_INTEGER && *p=='.' ) { *t++=*p; tail->type=JT_REAL; }
			else if ( isdigit(*p) ) { *t++=*p; }
			else goto Syntax;
			break;
		// TODO handle exponentials
		case COMMA:
			// [:w]		ignore
			// [}]		end list, save param, nest--, COMMA (END if nest==0)
			// [,]		extend list, TAG0
			if ( isspace(*p) ) {}
			else if ( *p=='}' ) { state=(--nest==0)?END:COMMA; tail = tail->parent; }
			else if ( *p==',' ) { state=TAG0; tail=json_new_list(tail); }
			else goto Syntax;
			break;
		case END:
			// [:w]		END
			// *		reject
			if ( isspace(*p) ) {}
			else goto Syntax;
			break;
		default:
			gl_error("json::parse(char *buffer='%s'): parser state error at position %d", buffer,(int)(p-buffer));
			goto Error;
		}
	}
	gl_debug("json parse dump...");
	json_dump(head);
	return head;
Syntax:
	gl_error("json::parse(char *buffer='%s'): syntax error at position %d: ...%-16.16s...", buffer,(int)(p-buffer), p);
Error:
	destroy(head);
	return NULL;
}

// find a tag in a json structure
JSONLIST *json::find(JSONLIST *list, const char *tag)
{
	if ( list==NULL ) return NULL;
	if ( strcmp(list->tag,tag)==0 ) return list;
	JSONLIST *sub = list->type==JT_LIST?find(list->list,tag):NULL;
	if ( sub!=NULL ) return sub;
	return find(list->next,tag);
}

// find a tag and return the value
char *json::get(JSONLIST *list, const char *tag)
{
	JSONLIST *found = find(list,tag);
	if ( found ) return found->string;
	else return NULL;
}

// destroy a json structure
void json::destroy(JSONLIST *list)
{
	if ( list!=NULL )
	{
		if ( list->type==JT_LIST ) destroy(list->list);
		destroy(list->next);
		delete list;
	}
}
