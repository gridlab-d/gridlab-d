// $Id: main.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute

#define DLMAIN
#include "config.h"

#ifdef HAVE_MYSQL
#include "database.h"

#define MYSQL_TS_NEVER "'2038-01-01 00:00:00'" // near maximum mysql timestamp is used for TS_NEVER
#define MYSQL_TS_ZERO "'1970-01-01 00:00:00'" // zero mysql timestamp is used for TS_ZERO

// connection data
char hostname[256] INIT("");
char username[32] INIT("");
char password[32] INIT("");
char schema[256] INIT("");
int32 port INIT(-1);
char socketname[1024] INIT("");
int64 clientflags INIT(-1);
char table_prefix[256] INIT(""); ///< table prefix

// options
bool new_database = false; ///< flag to drop a database before using it (very dangerous)
bool show_query = false; ///< flag to show queries when verbose is on
bool no_create = false; ///< flag to not create tables when exporting data
bool overwrite = true; ///< flag to not drop tables when exporting data

#else
#include "gridlabd.h"
#endif

/********************************************************
 * MODULE SUPPORT
 ********************************************************/

EXPORT int do_kill(void*)
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

EXPORT int check(){
	/* if any assert objects have bad filenames, they'll fail on init() */
	return 0;
}

#ifdef HAVE_MYSQL
/********************************************************
 * MYSQL SUPPORT
 ********************************************************/

const char *get_table_name(const char *format, ...)
{
	static char buffer[1024];
	char fullfmt[1024] = "";
	if ( table_prefix[0]!='\0' && sscanf(table_prefix,"%[A-Za-z0-9_]",fullfmt)==1 && strcmp(table_prefix,fullfmt)!=0 )
			gl_warning("mysql::table_prefix truncated to allowed prefix '%s'",fullfmt);
	strcat(fullfmt,format);
	va_list ptr;
	va_start(ptr,format);
	vsprintf(buffer,fullfmt,ptr);
	va_end(ptr);
	return buffer;
}

const char *process_command(const char *command)
{
	// copy defaults
	if ( hostname[0]=='\0' ) strcpy(hostname,default_hostname);
	if ( username[0]=='\0' ) strcpy(username,default_username);
	if ( password[0]=='\0' ) strcpy(password,default_password);
	strcpy(schema,"");
	if ( port==-1 ) port = default_port;
	if ( socketname[0]=='\0' )strcpy(socketname,default_socketname);
	if ( clientflags==-1 ) clientflags = default_clientflags;
	if ( table_prefix[0]=='\0' ) strcpy(table_prefix,default_table_prefix);

	// parse command
	char buffer[4096];
	strcpy(buffer,command);
	char *token=NULL, *next;
	enum { PS_SCHEMA, PS_HOSTNAME, PS_USERNAME, PS_PASSWORD, PS_PORT, PS_SOCKET, PS_FLAGS, PS_PREFIX} state = PS_SCHEMA;
	while ( (token=strtok_s(token?NULL:buffer," \t",&next))!=NULL )
	{
		if ( strcmp(token,"-h")==0 || strcmp(token,"--hostname")==0)
			state = PS_HOSTNAME;
		else if ( strcmp(token,"-u")==0 || strcmp(token,"--username")==0)
			state = PS_USERNAME;
		else if ( strcmp(token,"-p")==0 || strcmp(token,"--password")==0)
			state = PS_PASSWORD;
		else if ( strcmp(token,"-P")==0 || strcmp(token,"--port")==0)
			state = PS_PORT;
		else if ( strcmp(token,"-s")==0 || strcmp(token,"--socketname")==0)
			state = PS_SOCKET;
		else if ( strcmp(token,"-f")==0 || strcmp(token,"--flags")==0)
			state = PS_FLAGS;
		else if ( strcmp(token,"-t")==0 || strcmp(token,"--table_prefix")==0)
			state = PS_PREFIX;
		else if ( strcmp(token,"--new_database")==0 )
		{
			new_database = true;
			state = PS_SCHEMA;
		}
		else if ( strcmp(token,"--show_query")==0 )
		{
			gld_global verbose("verbose");
			if ( verbose.is_valid() && verbose.get_bool()==false )
				gl_warning("option '--show_query' has no effect unless verbose mode is enabled");
			show_query = true;
			state = PS_SCHEMA;
		}
		else if ( strcmp(token,"--no_create")==0 )
		{
			no_create = true;
			state = PS_SCHEMA;
		}
		else if ( strcmp(token,"--no_overwrite")==0 )
		{
			overwrite = false;
			state = PS_SCHEMA;
		}
		else if ( token[0]=='-' )
		{
			gl_error("mysql option '%s' is not recognized", token);
			return NULL;
		}
		else
		{
			if ( strlen(token)==0 ) continue; // ignore blank tokens
			switch ( state ) {
			case PS_SCHEMA:
				if ( strlen(schema)>0 )
				{
					gl_error("schema '%s' is not allowed because only one schema may be specified", schema);
					return NULL;
				}
				else if ( strlen(token)>=sizeof(schema) )
				{
					gl_error("schema name '%s' is too long to process", token);
					return NULL;
				}
				strcpy(schema,token);
				break;
			case PS_HOSTNAME:
				if ( strlen(token)>=sizeof(hostname) )
				{
					gl_error("hostname '%s' is too long to process", token);
					return NULL;
				}
				strcpy(hostname,token);
				state = PS_SCHEMA;
				break;
			case PS_USERNAME:
				if ( strlen(token)>=sizeof(username) )
				{
					gl_error("username '%s' is too long to process", token);
					return NULL;
				}
				strcpy(username,token);
				state = PS_SCHEMA;
				break;
			case PS_PASSWORD:
				if ( strlen(token)>=sizeof(password) )
				{
					gl_error("password '%s' is too long to process", token);
					return NULL;
				}
				strcpy(password,token);
				state = PS_SCHEMA;
				break;
			case PS_PORT:
				port = atoi(token);
				state = PS_SCHEMA;
				break;
			case PS_FLAGS:
				gl_warning("mysql import/export flags '%s' ignored, use module flags instead", token);
				state = PS_SCHEMA;
				break;
			case PS_PREFIX:
				if ( strlen(token)>=sizeof(table_prefix) )
				{
					gl_error("table prefix '%s' is too long to process", token);
					return NULL;
				}
				strcpy(table_prefix,token);
				state = PS_SCHEMA;
				break;
			default:
				gl_error("process_command(const char *command='%s'): state = %d is invalid", command, state);
				return NULL;
			}
		}
	}
	if ( strlen(schema)==0 )
		strcpy(schema,default_schema);
	if ( strlen(schema)==0 )
	{
		gl_error("no schema is specified");
		return NULL;
	}
	else if ( strcmp(schema,"mysql")==0 || strcmp(schema,"sys")==0 )
	{
		strcpy(schema,"");
		gl_error("access to the mysql system databases is not supported");
		return NULL;
	}
	else
		return schema;
}

static bool query_quiet(MYSQL *mysql, const char *format, ...)
{
	char query_string[65536];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(query_string,format,ptr);
	va_end(ptr);
	if ( show_query )
		gl_verbose("running query [%s]",query_string);
	if ( mysql_query(mysql,query_string)==0 )
	{
		if ( show_query ) gl_verbose("query ok");
		return true; // true on success
	}
	else
		return false;
}
static bool query(MYSQL *mysql, const char *format, ...)
{
	char query_string[65536];
	va_list ptr;
	va_start(ptr,format);
	vsprintf(query_string,format,ptr);
	va_end(ptr);
	if ( !query_quiet(mysql,"%s",query_string) )
	{
		gl_error("query [%s] failed: %s", query_string, mysql_error(mysql));
		return 0; // false on failure
	}
	else
		return 1; // true on success
}
static bool create_schema(MYSQL *mysql, const char *schema)
{
if ( !query(mysql,"CREATE SCHEMA IF NOT EXISTS `%s`", schema) )
		return false;
	else if ( !query(mysql,"USE `%s`", schema))
		return false;
	else
		return true;
}
static MYSQL *get_connection(const char *schema, bool autocreate=false)
{
	// connect to server
	MYSQL *mysql = mysql_init(NULL);
	if ( mysql==NULL )
	{
		gl_error("mysql_init memory allocation failure");
		return 0;
	}
	gl_debug("mysql_connect(hostname='%s',username='%s',password='%s',schema='%s',port=%u,socketname='%s',clientflags=0x%016llx)",
		(const char*)hostname,(const char*)username,(const char*)password,(const char*)schema,port,(const char*)socketname,clientflags);
	mysql = mysql_real_connect(mysql,hostname,username,password,NULL,port,socketname,(unsigned long)clientflags);
	if ( mysql==NULL )
	{
		gl_error("mysql connect failed - %s", mysql_error(mysql_client));
		return 0;
	}
	else
		gl_debug("MySQL server info: %s", mysql_get_server_info(mysql));

	// connect to database
	if ( new_database && !query(mysql,"DROP SCHEMA IF EXISTS `%s`", schema))
		return 0;
	else if ( mysql_select_db(mysql,schema)!=0 )
	{
		if ( !autocreate )
		{
			gl_error("unable to select schema '%s'", schema);
			return 0;
		}
		else if ( create_schema(mysql,schema) )
			return mysql;
		else
		{
			gl_error("unable to establish connection to schema '%s'", schema);
			return NULL;
		}
	}
	else
		gl_verbose("schema '%s' selected ok", schema);

	return mysql;
}

static MYSQL_RES *query_result(MYSQL *mysql, const char *fmt,...)
{
	char command[4096];
	va_list ptr;
	va_start(ptr,fmt);
	vsnprintf(command,sizeof(command),fmt,ptr);

	// query mysql
	if ( show_query )
		gl_verbose("result query [%s]",command);
	if ( mysql_query(mysql,command)!=0 )
	{
		gl_error("query [%s] failed - %s", command, mysql_error(mysql));
		return NULL;
	}
	else if ( show_query )
		gl_verbose("query [%s] ok", command);

	// get result
	MYSQL_RES *res = mysql_store_result(mysql);
	if ( res==NULL )
	{
		gl_error("query [%s] store result failed - %s", command, mysql_error(mysql));
		return NULL;
	}
	int n = mysql_num_rows(res);
	gl_debug("query [%s] returned %d rows", command, n);

	va_end(ptr);
	return res;
}

static TIMESTAMP get_mysql_timestamp(char *string)
{
	gld_clock t(string);
	TIMESTAMP ts = t.get_timestamp();
	if ( !t.is_valid() )
		return TS_INVALID;
	else if ( ts <= gld_clock(MYSQL_TS_ZERO).get_timestamp() )
		return TS_ZERO;
	else if ( ts >= gld_clock(MYSQL_TS_NEVER).get_timestamp() )
		return TS_NEVER;
	else
		return ts;
}

/********************************************************
 * IMPORT
 ********************************************************/

static bool import_modules(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `name`,`major`,`minor` FROM `%s` ORDER BY `id`", get_table_name("modules"));
	if ( data==NULL)
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("modules table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		gl_debug("import_modules(MYSQL*): row %d: `%s` = [%s]", n, fields[0].name, row[0]);
		unsigned int major = row[1] ? atoi(row[1]) : 0;
		unsigned int minor = row[2] ? atoi(row[2]) : 0;
		if ( !gl_module_depends(row[0],major,minor) )
		{
			gl_error("import_modules(MYSQL*): module %s dependency failed", row[0]);
			return false;
		}
	}
	mysql_free_result(data);
	return true;
}
static bool import_globals(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `name`,`type`,`value` FROM `%s`", get_table_name("globals"));
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("globals table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		gl_debug("import_globals(MYSQL*): row %d: `%s` = [%s], `%s` = [%s]", n, fields[0].name, row[0], fields[1].name, row[2]);
		char old[1024]="(undefined)";
		gld_global var(row[0]);
		if ( var.is_valid() )
		{
			var.to_string(old,sizeof(old));
			if ( !var.from_string(row[2]) )
			{
				gl_error("import_globals(MYSQL*): global set %s='%s' failed", row[0], row[2]);
				return false;
			}
			else
				gl_verbose("set global %s: '%s' -> '%s'", row[0], old, row[2]);
		}
		else if ( !var.create(row[0],(PROPERTYTYPE)atoi(row[1]),row[2]) )
		{
			gl_error("import_globals(MYSQL*): global create type %s='%s' failed", row[0], row[2]);
			return false;
		}
		else
		{
			gld_type ptype((PROPERTYTYPE)atoi(row[1]));
			gl_verbose("create global %s type %s: '%s'", row[0], ptype.get_spec()->name, row[2]);
		}
	}
	mysql_free_result(data);
	return true;
}
static bool import_classes(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `name`, `module`, `property`, `type`, `flags`, `units`, `description` "
			"FROM `%s` WHERE type<%d", get_table_name("classes"), _PT_LAST);
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("classes table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		unsigned int flags = atoi(row[4]);
		if ( (flags&PF_EXTENDED)==0 ) continue; // ignore classes that are not runtime extensions
		char *name = row[0];
		char *module = row[1];
		char *property = row[2];
		PROPERTYTYPE type = (PROPERTYTYPE)atoi(row[3]);
		char *unit = row[5];
		char *description = row[6];
		gl_debug("import_classes(MYSQL*): row %d, name=%s, module=%s, property=%s, type=%s, flags=%s, units=%s, description=%s",
				n, name, module, property, type, flags, unit, description);

		// create runtime classes
		CLASS *cls = gl_class_get_by_name(name);
		if ( module==NULL && cls==NULL && (cls=gl_register_class(NULL,name,0,0))==NULL )
			return false;

		// create extended properties in classes
		if ( flags&PF_EXTENDED )
		{
			if ( gl_class_add_extended_property(cls,property,type,unit)==NULL )
				return false;
			else
				gl_verbose("property '%s[%s]' added to class '%s'", property, unit, name);
		}
		else
		{
			// TODO check definition matches module's
		}
	}
	mysql_free_result(data);
	return true;
}

static bool import_objects(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `id`,`class`,`name`,`groupid`,`parent`,`rank`,`clock`,`valid_to`,`schedule_skew`,"
			"`latitude`,`longitude`,`in_svc`,`in_svc_micro`,`out_svc`,`out_svc_micro`,`rngstate`,`heartbeat`,`flags`,`module` FROM `%s` ORDER by `id`", get_table_name("objects"));
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("objects table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	OBJECT *first_object = NULL;
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		gl_verbose("import_objects(MYSQL*): row %d, id=%s, class=%s, name=%s, groupid=%s, parent=%s, rank=%s, clock='%s', valid_to='%s', schedule_skew='%s',"
				" latitude=%s, longtitude=%s, in_svc='%s', in_svc_micro=%s, out_svc='%s', out_svc_micro=%s, rngstate=%s, heartbeat='%s', flags=%s", n,
				row[0], row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8], row[9],
				row[10], row[11], row[12], row[13], row[14], row[15], row[16], row[17]);

		// find class for this object (include module match)
		CLASS *cls = gl_class_get_by_name(row[1]) ;;
		while ( cls!=NULL && !( ( cls->module==NULL && row[18]==NULL ) || strcmp(cls->module->name,row[18])==0 ) )
		{
			gl_verbose("searching for class that matches module: '%s' -> %s:%s ", row[18], cls->module, cls->name);
			do { cls=cls->next; } while ( cls!=NULL && strcmp(cls->name,row[1])!=0 );
		}
		if ( cls==NULL )
		{
			gl_error("class '%s' not found", row[1]);
			return false;
		}

		// create new objects
		OBJECT *obj = gl_create_object(cls);
		if ( obj==NULL )
		{
			gl_error("unable to create object of class '%s'", cls->name);
			return false;
		}
		if ( row[2]!=NULL )
		{
			obj->name = (char*)malloc(strlen(row[2])+1);
			if ( obj->name==NULL )
			{
				gl_error("memory allocation failed");
				return false;
			}
			strcpy(obj->name,row[2]);
		}
		if ( row[3]!=NULL ) strncpy(obj->groupid,row[3],sizeof(obj->groupid));
		obj->parent = row[4]==NULL ? NULL : gl_object_find_by_id(atoi(row[4]));
		obj->rank = atoi(row[5]);
		obj->clock = get_mysql_timestamp(row[6]);
		obj->valid_to = get_mysql_timestamp(row[7]);
		obj->schedule_skew = get_mysql_timestamp(row[8]);
		obj->latitude = row[9]==NULL ? QNAN : atof(row[9]);
		obj->longitude = row[10]==NULL ? QNAN : atof(row[10]);
		obj->in_svc = get_mysql_timestamp(row[11]);
		obj->in_svc_micro = atoi(row[12]);
		obj->out_svc = get_mysql_timestamp(row[13]);
		obj->out_svc_micro = atoi(row[14]);
		obj->rng_state = atoi(row[15]);
		obj->heartbeat = get_mysql_timestamp(row[16]);
		obj->flags = atoi(row[17]);
		if ( first_object==NULL )
			first_object = obj;
	}

	// load object properties
	for ( OBJECT *obj=first_object ; obj!=NULL ; obj=obj->next )
	{
		// read properties of object from each class
		for ( CLASS *cls = obj->oclass ; cls!=NULL ; cls=cls->parent )
		{
			MYSQL_RES *vars = query_result(mysql,"SELECT * FROM `%s` WHERE id=%d", get_table_name("%s_%s", cls->module?cls->module->name:"", cls->name), obj->id);
			if ( vars==NULL )
				return false;
			unsigned n_vars = mysql_num_rows(vars);
			unsigned n_props = mysql_num_fields(vars);
			gl_verbose("object properties: %d rows x %d fields", n_vars, n_props);
			for ( unsigned long m=0 ; m<n_vars ; m++ )
			{
				MYSQL_ROW var = mysql_fetch_row(vars);
				MYSQL_FIELD *prop = mysql_fetch_fields(vars);
				gl_verbose("row/field fetches ok");
				for ( unsigned long p=1 ; p<n_props ; p++ ) // don't process the first field, it's the id
				{
					if ( var[p]==NULL ) continue; // NULLs are ignored because core does not allow unsetting properties
					gl_debug("gl_set_value_by_name(obj=%0x, property='%s', value='%s')", obj,prop[p].name, var[p]);
					gld_property value(obj,prop[p].name);
					if ( !value.is_valid() )
						gl_warning("object %s:%d property '%s' is not valid, value [%s] cannot be imported", obj->oclass->name, obj->id, prop[p].name, var[p]);
					else if ( !value.get_access(PA_W|PA_L) )
						gl_warning("object %s:%d property '%s' is not writeable and loadable, value [%s] cannot be imported", obj->oclass->name, obj->id, prop[p].name, var[p]);
					else if ( value.from_string(var[p])<=0 )
					{
						gl_error("unable to set object %s:%d property '%s' to value [%s]", obj->oclass->name, obj->id, prop[p].name, var[p]);
						return false;
					}
					else
						gl_verbose("setting object %s:%d property '%s' to value [%s] -> [%s]", obj->oclass->name, obj->id, value.get_name(), var[p], (const char*)value.get_string());
				}
			}
			mysql_free_result(vars);
		}
	}
	mysql_free_result(data);

	// resolve parents
	data = query_result(mysql,"SELECT `id`, `parent` FROM `%s` WHERE `parent`>`id`", get_table_name("objects"));
	n_rows = mysql_num_rows(data);
	n_fields = mysql_num_fields(data);
	gl_debug("unresolved parents: %d rows x %d fields", n_rows, n_fields);
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		gl_verbose("import_objects(MYSQL*): row %d, id=%s, parent=%s", n, row[0], row[1]);
		if ( row[0]!=NULL && row[1]!=NULL )
		{
			unsigned int obj_id = atoi(row[0]);
			unsigned int parent_id = atoi(row[1]);
			OBJECT *obj = gl_object_find_by_id(obj_id);
			OBJECT *parent = gl_object_find_by_id(parent_id);
			if ( obj==NULL )
			{
				gl_error("import_objects(MYSQL*): object id %d is not found", obj->id);
				return false;
			}
			else if ( parent==NULL )
			{
				gl_error("import_objects(MYSQL*): parent object id %d is not found", obj->id);
				return false;
			}
			else
			{
				obj->parent = parent;
				gl_verbose("resolved parent of %s:%d ok", obj->oclass->name, obj->id);
			}
		}
	}
	mysql_free_result(data);
	return true;
}
bool import_properties(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `id`,`property`,`type`,`specs` FROM `%s`", get_table_name("properties"));
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("properties table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	OBJECT *first_object = NULL;
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		MYSQL_FIELD *fields = mysql_fetch_fields(data);
		gl_verbose("import_objects(MYSQL*): row %d, id=%s, property=%s, type=%s, specs=%s",
				n, row[0], row[1], row[2], row[3]);
		if ( row[0]==NULL || row[1]==NULL || row[2]==NULL || row[3]==NULL )
		{
			gl_error("import_objects(MYSQL*): properties specs contains null values");
			return false;
		}
		unsigned int obj_id = atoi(row[0]);
		OBJECT *obj = gl_object_find_by_id(obj_id);
		if ( obj==NULL )
		{
			gl_error("import_objects(MYSQL*): object id %d is not found", obj_id);
			return false;
		}
		gld_property prop(obj,row[1]);
		if ( !prop.is_valid() )
		{
			gl_error("import_objects(MYSQL*): object %s:%d property %s is not found", obj->oclass->name, obj->id, row[1]);
			return false;
		}
		if ( strcmp(row[2],"randomvar")==0 )
		{
			if ( prop.from_string(row[3])<=0 )
			{
				gl_error("import_objects(MYSQL*): object %s:%d property %s value %s is not valid", obj->oclass->name, obj->id, row[1], row[3]);
				return false;
			}
		}
		else
		{
			gl_error("import_objects(MYSQL*): object %s:%d property %s type %s is not recognized", obj->oclass->name, obj->id, row[1], row[2]);
			return false;
		}
	}
	mysql_free_result(data);
	return true;
}
EXPORT int import_schedules(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `name`,`definition` FROM `%s`", get_table_name("schedules"));
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("schedules table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		if ( row[0]==NULL || row[1]==NULL )
		{
			gl_error("NULL schedule name or definition is not valid");
			return false;
		}
		SCHEDULE *schedule = gl_schedule_find(row[0]);
		if ( schedule==NULL )
		{
			if ( gl_schedule_create(row[0],row[1])==NULL )
			{
				gl_error("unable to create schedule '%s'=[%s]", row[0], row[1]);
				return false;
			}
			else
				gl_verbose("created schedule '%s'", row[0]);
		}
		else if ( strcmp(schedule->definition,row[1])!=0 )
			gl_warning("schedule '%s' is already defined but differs", row[0]);
	}
	mysql_free_result(data);
	return true;
}
static bool add_linear_transform(unsigned int source_type, char *source, char *target, char *spec)
{
	char name[64], prop[64]="";
	if ( sscanf(target,"%[^.].%s",name,prop)!=2 )
	{
		gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): target is not valid", source, target, spec);
		return false;
	}

	double scale=1, bias=0;
	if ( sscanf(spec,"*%g+%g",&scale,&bias)!=2 )
	{
		gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): linear transform specification is invalid", source, target, spec);
		return false;
	}

	gld_property dst(name,prop);
	if ( !dst.is_valid() )
	{
		gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): target is not found", source, target, spec);
		return false;
	}

	int type = sscanf(source,"%[^.].%s",name,prop);
	switch ( type ) {
	case 0:
		gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): source is not valid", source, target, spec);
		return false;
	case 1: // source is a schedule
	{
		SCHEDULE *schedule = gl_schedule_find(name);
		if ( schedule==NULL )
		{
			gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): source schedule is not found", source, target, spec);
			return false;
		}
		// TODO
		gl_error("import schedule transformation not supported yet");
		return false;
		break;
	}
	case 2: // source is a property
	{
		gld_property src(name,prop);
		if ( !src.is_valid() )
		{
			gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): source is not found", source, target, spec);
			return false;
		}
		if ( !gl_transform_add_linear((TRANSFORMSOURCE)source_type,(double*)src.get_addr(),dst.get_addr(),scale,bias,dst.get_object(),dst.get_property(),NULL) )
		{
			gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): add transform failed - probable memory allocation failure", source, target, spec);
			return false;
		}
		break;
	}
	default:
		gl_error("add_linear_transform(char *source='%s', char *target='%s', char *spec='%s'): error parsing source", source, target, spec);
		return false;
	}
	return true;
}
static bool add_external_transform(unsigned int source_type, char *source, char *target, char *spec)
{
	// TODO
	gl_error("import external transforms not supported yet");
	return false;
}
EXPORT int import_transforms(MYSQL *mysql)
{
	MYSQL_RES *data = query_result(mysql,"SELECT `source`,`target`,`transform_type`,`source_type`,`specification` FROM `%s`", get_table_name("transforms"));
	if ( data==NULL )
		return false;

	// get size of result
	unsigned long n_rows = mysql_num_rows(data);
	unsigned long n_fields = mysql_num_fields(data);
	gl_debug("transforms table: %d rows x %d fields", n_rows, n_fields);

	// scan result
	for ( unsigned long n=0 ; n<n_rows ; n++ )
	{
		MYSQL_ROW row = mysql_fetch_row(data);
		char *source = row[0];
		char *target = row[1];
		char *ttype = row[2];
		char *stype = row[3];
		char *spec = row[4];
		if ( source==NULL || target==NULL || ttype==NULL || stype==NULL || spec==NULL)
		{
			gl_error("import_transforms cannot have a NULL source, target, type or specification");
			return false;
		}
		int transform_type = 0;
		int source_type = 0;
		switch (sscanf(ttype,"%d",&transform_type)==1,transform_type) {
		case XT_LINEAR:
			if ( sscanf(stype,"%d",&source_type)==0 || !add_linear_transform(source_type,source,target,spec) )
				return false;
			break;
		case XT_EXTERNAL:
			if (  sscanf(stype,"%d",&source_type)==0 || !add_external_transform(source_type,source,target,spec) )
				return false;
			break;
		default:
			gl_error("invalid transform type '%s'", stype);
			return false;
		}
	}
	mysql_free_result(data);
	return true;
}
EXPORT int import_file(const char *info)
{
	if ( process_command(info)==NULL )
		return 0;
	MYSQL *mysql = get_connection(schema,true);
	if ( mysql==NULL )
		return 0;
	int rc = import_modules(mysql)
			&& import_globals(mysql)
			&& import_classes(mysql)
			&& import_objects(mysql)
			&& import_properties(mysql)
			&& import_schedules(mysql)
			&& import_transforms(mysql);
			// TODO schedules
	mysql_close(mysql);
	return rc;
}

/********************************************************
 * EXPORT
 ********************************************************/
static bool export_modules(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("modules")) )
		return false;
	if ( !no_create && !query(mysql, "CREATE TABLE IF NOT EXISTS `%s` ("
			" `id` mediumint PRIMARY KEY AUTO_INCREMENT,"
			" `name` char(64),"
			" `major` smallint,"
			" `minor` smallint)"
			"", get_table_name("modules")) )
		return false;

	// start module list
	gld_module mod;
	while ( true )
	{
		if ( !query(mysql,"REPLACE INTO `%s` (`name`,`major`,`minor`) VALUES (\"%s\",%d,%d)", get_table_name("modules"),
				mod.get_name(), mod.get_major(), mod.get_minor()) )
			return false;
		if ( mod.is_last() )
			break;
		else
			mod.get_next();
	}
	return true;
}
static bool export_globals(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("globals")) )
		return false;
	if ( !no_create && !query(mysql, "CREATE TABLE IF NOT EXISTS `%s` ("
			" `name` char(64) PRIMARY KEY,"
			" `type` bigint,"
			" `flags` bigint,"
			" `value` text,"
			" `unit` text,"
			" `description` text)"
			"", get_table_name("globals")) )
		return false;
	gld_global var;
	while ( true )
	{
		PROPERTY *prop = var.get_property();
		char buffer[4096], quoted[4096*2+1+3]="", *value = buffer;
		strcpy(value,var.get_string());
		if ( value[0]=='"' )
		{
			value[strlen(value)-1] = '\0'; // remove quotes
			value++;
			gl_debug("quoted global property value [%s]",value);
		}
		if ( strlen(value)>0 )
			mysql_real_escape_string(mysql,quoted,value,strlen(value)); // protect SQL from contents
		char unit[1024] = "NULL";
		if ( prop->unit!=NULL )
			sprintf(unit,"\"%s\"",prop->unit->name);
		if ( !query(mysql,"REPLACE INTO `%s` (`name`,`type`,`flags`,`value`,`unit`,`description`) VALUES (\"%s\",%d,%d,\"%s\",%s,\"%s\")",
				get_table_name("globals"), prop->name, prop->ptype, var.get_flags(), quoted, unit, prop->description) )
			return false;
		if ( var.is_last() )
			return true;
		else
			var = var.get_next();
	}
}
static bool export_class(MYSQL *mysql, CLASS *cls)
{
	MODULE *mod = cls->module;
	char modname[128] = "NULL";
	if ( mod )
		sprintf(modname,"\"%s\"",mod->name);

	// handle parent class first
	if ( cls->parent!=NULL )
	{
		if ( !export_class(mysql,cls->parent) || !query(mysql,"REPLACE INTO `%s` (`name`,`module`,`property`,`type`) "
				"VALUES (\"%s\",%s,\"%s\",%d)",
				get_table_name("classes"), cls->name, modname, cls->parent->name, PT_INHERIT) )
			return false;
	}

	// create class tables
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("%s_%s",mod?mod->name:"", cls->name)) )
		return false;
	char query_string[65536] = "";
	int len = sprintf(query_string,"CREATE TABLE IF NOT EXISTS `%s` ("
			"`id` mediumint primary key",
			get_table_name("%s_%s",mod?mod->name:"", cls->name));
	for ( PROPERTY *prop = cls->pmap ; prop!=NULL && prop->oclass==cls ; prop=prop->next )
	{
		if ( !(prop->access&(PA_R|PA_S) ) )
			continue; // ignore unreadable non-saved properties

		// write class structure info
		char units[1024] = "NULL";
		if ( prop->unit!=NULL ) sprintf(units,"\"%s\"",prop->unit->name);
		char description[1024] = "NULL";
		if ( prop->description!=NULL ) sprintf(description,"\"%s\"",prop->description);
		if ( !query(mysql,"REPLACE INTO `%s` (`name`,`module`,`property`,`type`,`flags`,`units`,`description`) "
				"VALUES (\"%s\",%s,\"%s\",%d,%d,%s,%s)",
				get_table_name("classes"), prop->oclass->name,modname,prop->name,prop->ptype,prop->flags,units,description) )
			return false;

		// write class table
		len += sprintf(query_string+len, ", `%s` text", prop->name);
		if ( prop->description!=NULL )
		{
			char quoted[4096];
			mysql_real_escape_string(mysql,quoted,prop->description,strlen(prop->description));
			len += sprintf(query_string+len, " comment \"%s\"", quoted);
		}

		// write keyword list (if any)
		for ( KEYWORD *keyword=prop->keywords; keyword!=NULL ; keyword=keyword->next )
		{
			if ( !query(mysql,"REPLACE INTO `%s` (`name`,`module`,`property`,`keyword`,`type`,`flags`) "
					"VALUES (\"%s\",%s,\"%s\",\"%s\",%d,%lld)",
					get_table_name("classes"), prop->oclass->name,modname,prop->name,keyword->name,PT_KEYWORD,keyword->value) )
				return false;
		}
	}
	len += sprintf(query_string+len,")");
	if ( !no_create && !query(mysql,"%s",query_string) )
		return false;
	return true;
}
static bool export_classes(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("classes")) )
		return false;
	if ( !no_create && !query(mysql,"CREATE TABLE IF NOT EXISTS `%s` ("
			"`id` mediumint NOT NULL AUTO_INCREMENT PRIMARY KEY, "
			"`name` char(64), "
			"`module` char(64),"
			"`property` char(64), "
			"`keyword` char(64) DEFAULT NULL, "
			"`type` bigint, "
			"`flags` bigint, "
			"`units` text, "
			"`description` text, "
			" KEY (`name`,`module`,`property`,`keyword`))", get_table_name("classes")) )
		return false;

	for ( CLASS *cls = gl_class_get_first(); cls!=NULL ; cls=cls->next )
	{
		if ( cls->profiler.numobjs==0 )
			continue; // ignore unused classes

		if ( !export_class(mysql,cls) )
			return false;
	}
	return true;
}
static bool export_properties(MYSQL *mysql, OBJECT *obj, CLASS *cls = NULL)
{
	// default is object's class
	if ( cls==NULL )
		cls = obj->oclass;
	// output parent class first
	if ( cls->parent!=NULL && !export_properties(mysql,obj,cls->parent) )
		return false;
	MODULE *mod = cls->module;
	char names[65536] = "";
	char values[65536] = "";
	unsigned int len_names = 0;
	unsigned int len_values = 0;
	for ( PROPERTY *prop=cls->pmap ; prop!=NULL && prop->oclass==cls; prop=prop->next )
	{
		gld_property var(obj,prop);
		if ( !var.get_access(PA_R|PA_S) )
			continue; // ignore properties that not readable or saveable
		len_names += sprintf(names+len_names,",`%s`", prop->name);
		char buffer[4096]="", quoted[4096*2+1+3]="", *value = buffer;
		TIMESTAMP ts;
		if ( prop->ptype==PT_timestamp && (var.getp(ts),ts)==TS_ZERO )
			strcpy(value,MYSQL_TS_ZERO);
		else if ( prop->ptype==PT_object )
		{
			OBJECT **os = (OBJECT**)var.get_addr();
			if ( os!=NULL && *os!=NULL )
				sprintf(value,"%s:%d", (*os)->oclass->name, (*os)->id);
		}
		else
			strcpy(value,var.get_string());
		if ( value[0]=='"' )
		{
			value[strlen(value)-1] = '\0'; // remove quotes
			value++;
			gl_debug("quoted object property value [%s]",value);
		}
		if ( strlen(value)>0 )
		{
			mysql_real_escape_string(mysql,quoted,value,strlen(value)); // protect SQL from contents
			len_values += sprintf(values+len_values,",\"%s\"", quoted);
		}
		else
			len_values += sprintf(values+len_values,",NULL");
	}
	if ( !query(mysql,"REPLACE INTO `%s` (`id`%s) VALUES (%d%s)", get_table_name("%s_%s",mod?mod->name:"", cls->name), names, obj->id, values) )
		return false;
	return true;
}
static bool export_objects(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("objects")) )
		return false;
	if ( !no_create && !query(mysql,"CREATE TABLE IF NOT EXISTS `%s` ("
			" `id` mediumint PRIMARY KEY,"
			" `module` char(64),"
			" `class` char(64),"
			" `name` char(64),"
			" `groupid` char(32),"
			" `parent` mediumint,"
			" `rank` mediumint,"
			" `clock` timestamp default " MYSQL_TS_ZERO ","
			" `valid_to` timestamp default " MYSQL_TS_ZERO ","
			" `schedule_skew` timestamp default " MYSQL_TS_ZERO ","
			" `latitude` double,"
			" `longitude` double,"
			" `in_svc`  timestamp default " MYSQL_TS_ZERO ","
			" `in_svc_micro` mediumint default 0,"
			" `out_svc` timestamp default " MYSQL_TS_ZERO ","
			" `out_svc_micro` mediumint default 0,"
			" `rngstate` bigint,"
			" `heartbeat` timestamp default " MYSQL_TS_ZERO ","
			" `flags` bigint,"
			" KEY (`name`))", get_table_name("objects")) )
		return false;
	for ( OBJECT *obj = gl_object_get_first(); obj!=NULL ; obj=obj->next )
	{
		// objects table
		CLASS *cls = obj->oclass;
		MODULE *mod = cls->module;
		char modname[64]="NULL";
		char name[1024]="NULL";
		char parent[64]="NULL";
		char groupid[32]="NULL";
		char latitude[64] = "NULL";
		char longitude[64] = "NULL";
		char clock[64] = MYSQL_TS_NEVER;
		char valid_to[64] = MYSQL_TS_NEVER;
		char schedule_skew[64] = MYSQL_TS_NEVER;
		char in_svc[64] = MYSQL_TS_NEVER;
		char out_svc[64] = MYSQL_TS_NEVER;
		char heartbeat[64] = MYSQL_TS_NEVER;
		if ( mod!=NULL ) sprintf(modname,"\"%s\"", mod->name);
		if ( obj->name!=NULL ) sprintf(name,"\"%s\"", obj->name);
		if ( strcmp(obj->groupid,"")!=0 ) sprintf(groupid,"\"%s\"", (const char*)obj->groupid);
		if ( obj->parent!=NULL ) sprintf(parent,"%d", obj->parent->id);
		if ( !isnan(obj->latitude) ) sprintf(latitude,"%g", obj->latitude);
		if ( !isnan(obj->longitude) ) sprintf(longitude,"%g", obj->longitude);
		if ( obj->clock<TS_NEVER ) if ( obj->clock==TS_ZERO) strcpy(clock,MYSQL_TS_ZERO); else sprintf(clock,"from_unixtime(%lld)", obj->clock);
		if ( obj->valid_to<TS_NEVER ) if ( obj->valid_to==TS_ZERO) strcpy(valid_to,MYSQL_TS_ZERO); else sprintf(valid_to,"from_unixtime(%lld)", obj->valid_to);
		if ( obj->schedule_skew<TS_NEVER ) if ( obj->schedule_skew==TS_ZERO) strcpy(schedule_skew,MYSQL_TS_ZERO); else sprintf(schedule_skew,"from_unixtime(%lld)", obj->schedule_skew);
		if ( obj->in_svc<TS_NEVER ) if ( obj->in_svc==TS_ZERO) strcpy(in_svc,MYSQL_TS_ZERO); else sprintf(in_svc,"from_unixtime(%lld)", obj->in_svc);
		if ( obj->out_svc<TS_NEVER ) if ( obj->out_svc==TS_ZERO) strcpy(out_svc,MYSQL_TS_ZERO); else sprintf(out_svc,"from_unixtime(%lld)", obj->out_svc);
		if ( obj->heartbeat<TS_NEVER ) if ( obj->heartbeat==TS_ZERO) strcpy(heartbeat,MYSQL_TS_ZERO); else sprintf(heartbeat,"from_unixtime(%lld)", obj->heartbeat);
		if ( !query(mysql,"REPLACE INTO `%s`"
				" (`id`,`module`,`class`,`name`,`groupid`,`parent`,`rank`,`latitude`,`longitude`,"
				" `clock`,`valid_to`,`schedule_skew`,`in_svc`,`in_svc_micro`,`out_svc`,`out_svc_micro`,"
				" `rngstate`,`heartbeat`,`flags`)"
				" VALUES (%d,%s,\"%s\",%s,%s,%s,%d,%s,%s,"
				" %s,%s,%s,%s,%d,%s,%d,"
				" %d,%s,%d)",
				get_table_name("objects"),
				obj->id,modname,cls->name,name,groupid,parent,obj->rank,latitude,longitude,
				clock,valid_to,schedule_skew,in_svc,obj->in_svc_micro,out_svc,obj->out_svc_micro,
				obj->rng_state,heartbeat,obj->flags) )
			return false;

		// data table
		if ( !export_properties(mysql,obj) )
			return false;
	}
	return true;
}
bool export_properties(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("properties")) )
		return false;
	if ( !no_create && !query(mysql,"CREATE TABLE IF NOT EXISTS `%s` ("
					" `id` mediumint NOT NULL,"
					" `property` char(64) NOT NULL,"
					" `type` char(16) NOT NULL,"
					" `specs` text NOT NULL,"
					" UNIQUE (`id`,`property`,`type`))", get_table_name("properties")) )
		return false;
	for ( OBJECT *obj = gl_object_get_first(); obj!=NULL ; obj=obj->next )
	{
		CLASS *cls = obj->oclass;
		for ( PROPERTY *prop=cls->pmap ; prop!=NULL && prop->oclass==cls; prop=prop->next )
		{
			gld_property var(obj,prop);
			char specs[1024] = "";
			char *type = NULL;
			void *addr = var.get_addr();
			switch ( prop->ptype ) {
			case PT_random:
				gl_randomvar_getspec(specs,sizeof(specs),(randomvar_struct*)addr);
				type = "randomvar";
				break;
			default:
				break;
			}
			if ( type!=NULL && specs[0]=='\0' )
			{
				gl_error("unable to get extended property specs for object %s:%d property %s", cls->name, obj->id, prop->name);
				return false;
			}
			if ( type!=NULL && !query(mysql,"REPLACE INTO `%s` (`id`,`property`,`type`,`specs`) VALUES "
					"(%d,'%s','%s','%s')", get_table_name("properties"), obj->id, prop->name, type, specs) )
				return false;
		}
	}
	return true;
}
gld_property find_property_at_addr(void *addr)
{
	for ( OBJECT *obj = gl_object_get_first(); obj!=NULL ; obj=obj->next )
	{
		CLASS *cls = obj->oclass;
		for ( PROPERTY *prop=cls->pmap ; prop!=NULL && prop->oclass==cls; prop=prop->next )
		{
			gld_property var(obj,prop);
			if ( addr==var.get_addr() )
				return var;
		}
	}
	return gld_property();
}
bool export_transforms(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("transforms")) )
		return false;
	if ( !no_create && !query(mysql,"CREATE TABLE IF NOT EXISTS `%s` ("
					" `source` char(255) NOT NULL,"
					" `target` char(255) NOT NULL,"
					" `transform_type` mediumint NOT NULL,"
					" `source_type` mediumint NOT NULL,"
					" `specification` text NOT NULL,"
					" UNIQUE (`source`,`target`))", get_table_name("transforms")) )
		return false;
	for ( TRANSFORM *xform=gl_transform_getfirst() ; xform!=NULL ; xform=gl_transform_getnext(xform) )
	{
		char source[256];
		switch ( xform->source_type ) {
		case XS_DOUBLE:
		case XS_COMPLEX:
		case XS_LOADSHAPE:
		case XS_ENDUSE: {
			gld_property prop = find_property_at_addr(xform->source_addr);
			if ( !prop.is_valid() )
			{
				gl_error("export transform source address cannot be resolved to a valid object");
				return false;
			}
			sprintf(source,"%s:%u.%s",prop.get_object()->oclass->name, prop.get_object()->id, prop.get_property()->name);
			break; }
		case XS_SCHEDULE:
			strcpy(source,xform->source_schedule->name);
			break;
		default:
			gl_error("export transform source type is not valid (type=%d)", xform->source_type);
			return false;
		}

		char specs[65536]="";
		size_t len;
		const char *function;
		switch (xform->function_type) {
		case XT_LINEAR:
			len = sprintf(specs,"*%g+%g",xform->scale,xform->bias);
			break;
		case XT_EXTERNAL:
			if ( xform->nlhs>1 )
				len += sprintf(specs+len,"%s","(");
			for ( unsigned int n=1 ; n<xform->nlhs ; n++)
			{
				gld_property prop = find_property_at_addr(xform->plhs[n].addr);
				if ( !prop.is_valid() )
				{
					gl_error("export transform external function lhs[%u] property '%s' address cannot be resolved to an object",n, xform->plhs[n].prop->name);
					return false;
				}
				len += sprintf(specs+len,"%s%s:%d.%s",n>1?",":"", prop.get_object()->oclass->name, prop.get_object()->id, prop.get_property()->name);
			}
			if ( xform->nlhs>1 )
				len += sprintf(specs+len,"%s",")");
			function = gl_module_find_transform_function(xform->function);
			if ( function==NULL )
			{
				gl_error("export transform cannot resolve a module transfer function");
				return false;
			}
			len += sprintf(specs+len,"=%s(",xform->function);
			for ( unsigned int n=1; n<xform->nrhs ; n++ )
			{
				gld_property prop = find_property_at_addr(xform->prhs[n].addr);
				if ( !prop.is_valid() )
				{
					gl_error("export transform external function rhs[%u] property '%s' address cannot be resolved to an object",n, xform->prhs[n].prop->name);
					return false;
				}
				len += sprintf(specs+len,"%s%s:%d.%s",n>1?",":"", prop.get_object()->oclass->name, prop.get_object()->id, prop.get_property()->name);
			}
			len += sprintf(specs+len,"%s",")");
			return false;
		default:
			gl_error("transform type %d not supported", xform->function_type);
			return false;
		}
		if ( !query(mysql,"REPLACE INTO `%s` (`source`,`target`,`transform_type`,`source_type`,`specification`) "
				"VALUES (\"%s\",\"%s:%u.%s\",%d,%d,\"%s\")", get_table_name("transforms"), source,
				xform->target_obj->oclass->name, xform->target_obj->id, xform->target_prop->name, xform->function_type, xform->source_type, specs))
			return false;
	}
	return true;
}
bool export_schedules(MYSQL *mysql)
{
	if ( overwrite && !query(mysql,"DROP TABLE IF EXISTS `%s`", get_table_name("schedules")) )
		return false;
	if ( !no_create && !query(mysql,"CREATE TABLE IF NOT EXISTS `%s` ("
					" `name` char(64) PRIMARY KEY,"
					" `definition` text NOT NULL)", get_table_name("schedules")) )
		return false;
	for ( SCHEDULE *schedule=gl_schedule_getfirst() ; schedule!=NULL ; schedule=schedule->next )
	{
		char quoted[sizeof(schedule->definition)*2+1];
		mysql_real_escape_string(mysql,quoted,schedule->definition,strlen(schedule->definition));
		if ( !query(mysql,"REPLACE INTO `%s` (`name`,`definition`) VALUES (\"%s\",\"%s\")", get_table_name("schedules"),
				schedule->name, quoted) )
			return false;
	}
	return true;
}
EXPORT int export_file(char *info)
{
	if ( process_command(info)==NULL )
		return 0;
	MYSQL *mysql = get_connection(schema,true);
	if ( mysql==NULL )
		return 0;
	int rc = export_modules(mysql)
			&& export_globals(mysql)
			&& export_classes(mysql)
			&& export_objects(mysql)
			&& export_properties(mysql)
			&& export_transforms(mysql)
			&& export_schedules(mysql);
	mysql_close(mysql);
	return rc;
}
#endif

