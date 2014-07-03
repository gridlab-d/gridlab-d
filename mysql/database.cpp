/** $Id: database.cpp 4738 2014-07-03 00:55:39Z dchassin $
    DP Chassin
    Copyright (C) 2012 Battelle Memorial Institute
 **/

#ifdef HAVE_MYSQL

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <ctype.h>
#include <complex.h>

#include "database.h"

EXPORT_CREATE(database);
EXPORT_INIT(database);
EXPORT_COMMIT(database);

CLASS *database::oclass = NULL;
database *database::defaults = NULL;
database *database::first = NULL;
database *database::last = NULL;

database::database(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"database",sizeof(database),PC_AUTOLOCK);
		if (oclass==NULL)
			throw "unable to register class database";
		else
			oclass->trl = TRL_PROTOTYPE;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_char256,"hostname",get_hostname_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"server hostname",
			PT_char256,"schema",get_schema_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"schema name",
			PT_char32,"username",get_username_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"user name",
			PT_char32,"password",get_password_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"user password",
			PT_int32,"port",get_port_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"TCP port number",
			PT_char1024,"socketname",get_socketname_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"unix socket name",
			PT_set,"clientflags",get_clientflags_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"mysql client flags",
				PT_KEYWORD,"COMPRESS",(int64)CLIENT_COMPRESS,
				PT_KEYWORD,"FOUND_ROWS",(int64)CLIENT_FOUND_ROWS,
				PT_KEYWORD,"IGNORE_SIGPIPE",(int64)CLIENT_IGNORE_SIGPIPE,
				PT_KEYWORD,"INTERACTIVE",(int64)CLIENT_INTERACTIVE,
				PT_KEYWORD,"LOCAL_FILES",(int64)CLIENT_LOCAL_FILES,
				PT_KEYWORD,"MULTI_RESULTS",(int64)CLIENT_MULTI_RESULTS,
				PT_KEYWORD,"MULTI_STATEMENTS",(int64)CLIENT_MULTI_STATEMENTS,
				PT_KEYWORD,"NO_SCHEMA",(int64)CLIENT_NO_SCHEMA,
				PT_KEYWORD,"ODBC",(int64)CLIENT_ODBC,
				PT_KEYWORD,"SSL",(int64)CLIENT_SSL,
				PT_KEYWORD,"REMEMBER_OPTIONS",(int64)CLIENT_REMEMBER_OPTIONS,
			PT_set,"options",get_options_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"database connection options",
				PT_KEYWORD,"SHOWQUERY",(int64)DBO_SHOWQUERY,PT_DESCRIPTION,"show SQL queries when running",
				PT_KEYWORD,"NOCREATE",(int64)DBO_NOCREATE,PT_DESCRIPTION,"prevent automatic creation of non-existent schemas",
				PT_KEYWORD,"NEWDB",(int64)DBO_DROPSCHEMA,PT_DESCRIPTION,"destroy existing schemas before use them (risky)",
				PT_KEYWORD,"OVERWRITE",(int64)DBO_OVERWRITE,PT_DESCRIPTION,"destroy existing files before output dump/backup results (risky)",
			PT_char1024,"on_init",get_on_init_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"SQL script to run when initializing",
			PT_char1024,"on_sync",get_on_sync_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"SQL script to run when synchronizing",
			PT_char1024,"on_term",get_on_term_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"SQL script to run when terminating",
			PT_double,"sync_interval[s]",get_sync_interval_offset(),PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"interval at which on_sync is called",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(database));
		gl_verbose("MySQL client info: %s", mysql_get_client_info());
	}
}

int database::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	strcpy(hostname,default_hostname);
	strcpy(username,default_username);
	strcpy(password,default_password);
	strcpy(schema,default_schema);
	port = default_port;
	strcpy(socketname,default_socketname);
	clientflags = default_clientflags;
	last_database = this;

	// term list
	if ( first==NULL ) first = this;
	if ( last!=NULL ) last->next=this; 
	last=this;
	next=NULL;
	return 1; /* return 1 on success, 0 on failure */
}

int database::init(OBJECT *parent)
{
	gld_string flags = get_clientflags_property().get_string();
	
	gl_verbose("mysql_connect(hostname='%s',username='%s',password='%s',schema='%s',port=%u,socketname='%s',clientflags=0x%016llx[%s])",
		(const char*)hostname,(const char*)username,(const char*)password,(const char*)schema,port,(const char*)socketname,get_clientflags(),(const char*)flags);

	mysql = mysql_real_connect(mysql_client,hostname,username,strcpy(password,"")?password:NULL,NULL,port,socketname,(unsigned long)clientflags);
	if ( mysql==NULL )
		exception("mysql connect failed - %s", mysql_error(mysql_client));
	else
		gl_verbose("MySQL server info: %s", mysql_get_server_info(mysql));

	// autoname schema
	if ( strcmp(get_schema(),"")==0 )
	{
		char buffer[1024];
		gld_global model("modelname");
		if ( model.to_string(buffer,sizeof(buffer))>0 )
			set_schema(buffer);
	}

	// drop schema
	if ( get_options()&DBO_DROPSCHEMA && query("DROP DATABASE IF EXISTS `%s`", get_schema()) )
	{
		if ( strcmp(get_schema(),"gridlabd")==0 )
			gl_warning("%s uses NEWDB option on the default schema '%s' - this is extremely risky", get_name(), get_schema());
		gl_verbose("schema '%s' dropped ok", get_schema());
	}

	// check schema
	MYSQL_RES *res = mysql_list_dbs(mysql,get_schema());
	if ( mysql_num_rows(res)==0 )
	{
		if ( !(get_options()&DBO_NOCREATE) )
		{
			if ( query("CREATE DATABASE IF NOT EXISTS `%s`", get_schema()) )
				gl_verbose("created schema '%s' ok", get_schema());
		}
		else
			exception("NOCREATE option prevents automatic creation of schema '%s'", get_schema());
	}
	else
		gl_verbose("schema '%s' found ok", get_schema());

	// use schema
	if ( mysql_select_db(mysql,get_schema())!=0 )
		exception("unable to select schema '%s'", get_schema());

	// execute on_init script
	if ( strcmp(get_on_init(),"")!=0 )
	{
		gl_verbose("%s running on_init script '%s'", get_name(), get_on_init());
		int res = run_script(get_on_init());
		if ( res<=0 )
			exception("on_init script '%s' failed at line %d: %s", get_on_init(), -res, get_last_error());
	}
	return 1;
}

void database::term(void)
{	
	if ( strcmp(get_on_term(),"")!=0 )
	{
		gl_verbose("%s running on_term script '%s'", get_name(), get_on_term());
		try {
			int res = run_script(get_on_term());
			if ( res<=0 )
				exception("on_term script '%s' failed at line %d: %s", get_on_term(), -res, get_last_error());
		}
		catch (const char *msg)
		{
			gl_error("%s", msg);
		}
		mysql_close(mysql);
	}
}

TIMESTAMP database::commit(TIMESTAMP t0, TIMESTAMP t1)
{
	set_clock();
	if ( get_sync_interval()>0 )
	{
		gld_clock ts(t1);
		int mod = ts.get_localtimestamp()%(TIMESTAMP)get_sync_interval();
		if ( strcmp(get_on_sync(),"")!=0 && mod==0 )
		{
			int mod1 = t1%86400, mod2 = ts.get_timestamp()%86400, mod3 = ts.get_localtimestamp()%86400;
			gld_clock ts(t0);
			char buffer[64];
			gl_verbose("%s running on_init script '%s' at %s", get_name(), get_on_sync(), ts.to_string(buffer,sizeof(buffer))?buffer:"(unknown time)");
			int res = run_script(get_on_sync());
			if ( res<=0 )
				exception("on_init script '%s' failed at line %d: %s", get_on_sync(), -res, get_last_error());
		}
		return -(t1+get_sync_interval()-mod);
	}
	return TS_NEVER;
}

bool database::table_exists(char *t)
{
	if ( query("SHOW TABLES LIKE '%s'", t) )
	{
		MYSQL_RES *res = mysql_store_result(mysql);
		if ( res )
		{
			int n = mysql_num_rows(res);
			mysql_free_result(res);
			return n>0;
		}
	}
	return false;
}

char *database::get_sqltype(gld_property &prop)
{
	switch ( prop.get_type() ) {
	case PT_double:
	case PT_random:
	case PT_loadshape:
	case PT_enduse:
		return "DOUBLE";
	case PT_int16:
	case PT_int32:
	case PT_int64:
		return "INT(20)";
	case PT_char8:
		return "CHAR(8)";
	case PT_char32:
		return "CHAR(32)";
	case PT_enumeration:
	case PT_char256:
		return "CHAR(256)";
	case PT_char1024:
		return "CHAR(1024)";
	case PT_complex:
		return "CHAR(40)";
	case PT_set:
		return "LARGETEXT";
	case PT_bool:
		return "CHAR(1)";
	case PT_timestamp:
		return "DATETIME";
	case PT_double_array:
	case PT_complex_array:
		return "HUGETEXT";
	default:
		return NULL;
	}
}
const char *database::get_last_error(void)
{
	return mysql_error(mysql);
}
bool database::get_sqlbind(MYSQL_BIND &value, gld_property &target, my_bool *error)
{
	memset(&value,0,sizeof(value));
	switch ( target.get_type() ) {
	case PT_double:
	case PT_random:
	case PT_loadshape:
	case PT_enduse:
		value.buffer = target.get_addr();
		value.buffer_length = 0;
		value.buffer_type = MYSQL_TYPE_DOUBLE;
		value.error = error;
		return true;
	default:
		return false;
	}
}
char *database::get_sqldata(char *buffer, size_t size, gld_property &prop, double scale)
{
	switch ( prop.get_type() ) {
	case PT_double:
	case PT_random:
	case PT_loadshape:
	case PT_enduse:
		if ( prop.get_unit() )
		{
			double *value = (double*)prop.get_addr();
			if ( sprintf(buffer,"%g",*value*scale)>size )
				return NULL;
			return buffer;
		}
	}
	return prop.to_string(buffer,size) ? buffer : NULL;
}

bool database::query(char *fmt,...)
{
	char command[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsnprintf(command,sizeof(command),fmt,ptr);
	va_end(ptr);

	// query mysql
	if ( mysql_query(mysql,command)!=0 )
		exception("%s->query[%s] failed - %s", get_name(), command, mysql_error(mysql));
	else if ( get_options()&DBO_SHOWQUERY )
		gl_verbose("%s->query[%s] ok", get_name(), command);

	return true;
}

MYSQL_RES *database::select(char *fmt,...)
{
	char command[1024];
	va_list ptr;
	va_start(ptr,fmt);
	vsnprintf(command,sizeof(command),fmt,ptr);

	// query mysql
	if ( mysql_query(mysql,command)!=0 )
		exception("%s->select[%s] query failed - %s", get_name(), command, mysql_error(mysql));
	else if ( get_options()&DBO_SHOWQUERY )
		gl_verbose("%s->select[%s] ok", get_name(), command);

	// get result
	MYSQL_RES *res = mysql_store_result(mysql);
	if ( res==NULL )
		exception("%s->select[%s] result failed - %s", get_name(), command, mysql_error(mysql));
	int n = mysql_num_rows(res);
	gl_verbose("%s->select[%s] %d rows returned", get_name(), command, n);
	//if ( n==0 )
	//	return mysql_free_result(res),NULL;

	// TODO map variables
	va_end(ptr);
	return res;
}

unsigned int64 database::get_last_index(void)
{
	return mysql_insert_id(mysql);
}

int database::run_script(char *file)
{
	int num=0;
	char line[1024];
	char buffer[65536]="";
	int eol=0;
	FILE *fp = fopen(file,"r");
	if ( fp==NULL )
		exception("run_script(char *file='%s'): unable to open script", file); 
	while ( fgets(line,sizeof(line)-1,fp)!=NULL )
	{
		num++;
		int len = strlen(line);
		if ( len+eol>=sizeof(buffer) )
			exception("run_script(char *file='%s'): line '%s' command too long", num, file); 
		strcpy(buffer+eol,line);
		eol+=len;

		// trim trailing white space
		while ( eol>0 && isspace(buffer[eol-1]) ) buffer[--eol]='=0';

		// check for sql command terminating ';'
		if ( buffer[eol-1]==';' )
		{
			char *cmd = buffer;

			// terminate buffer here
			buffer[--eol]='\0';

			// trim left spaces
			while ( isspace(*cmd) && *cmd!='\0' ) cmd++;

			// ignore null commands
			if ( *cmd=='\0' )
				continue;

			// check for special commands
			if ( strnicmp(cmd,"BACKUP",6)==0 )
			{
				char *file = cmd+6;
				while ( isspace(*file) && *file!='\0' ) file++;
				if ( backup(file)<0 )
					exception("BACKUP failed - %s", mysql_error(mysql));
			}
			else if ( strnicmp(cmd,"DUMP",4)==0 )
			{
				char *table = cmd+4;
				while ( isspace(*table) && *table!='\0' ) table++;
				if ( dump(table)<0 )
					exception("DUMP failed - %s", mysql_error(mysql));
			}

			// regular SQL commands
			else
			{
				// run SQL query
				if ( !query("%s",buffer) )
					exception("run_script(char *file='%s'): line '%s' command failed", num, file); 
				MYSQL_RES *res = mysql_store_result(mysql);
				if ( res )
				{
					gl_verbose("query [%.32s%s] ok - %d rows returned", buffer,eol>32?"...":"", mysql_num_rows(res));
					mysql_free_result(res);
				}
				else
					gl_verbose("query [%.32s%s] ok - %d rows affected", buffer,eol>32?"...":"", mysql_affected_rows(mysql));
			}

			// clear buffer
			buffer[eol=0]='\0';
		}
		else
		{
			// add a single space before adding next line of script
			if ( eol>0 )
				strcpy(buffer+eol++," ");
		}
	}
Done:
	fclose(fp);
	return num;
}

size_t database::dump(char *table, char *file, unsigned long options)
{
	// prepare for output
	if ( !(options&TD_APPEND) )
	{
		if ( !file ) file=table;
		if ( get_options()&&DBO_OVERWRITE ) unlink(file);
		if ( access(file,0x00)==0 )
			exception("unable to dump '%s' to '%s' - OVERWRITE not specified but file exists",table,file);
	}

	// open the file
	FILE *fp = fopen(file,"at");
	if ( fp==NULL ) return -1;

	// request data
	MYSQL_RES *result = select("SELECT * FROM `%s`", table, file);
	if ( result==NULL )
	{
		fclose(fp);
		gl_error("dump of '%s' to '%s' failed - %s", table, file, mysql_error(mysql));
		return -1;
	}

	// retrieve data
	MYSQL_ROW row;
	int nfields = 0;
	int nrows = 0;
	while ( (row=mysql_fetch_row(result))!=NULL )
	{
		int n;

		// first time through
		if ( nfields==0 )
		{
			// get the field info
			nfields = mysql_num_fields(result);
			MYSQL_FIELD *fields = mysql_fetch_fields(result);

			// SQL format
			if ( options&TD_BACKUP )
			{
				// table drop/create commands
				fprintf(fp,"DROP TABLE IF EXISTS `%s`;\n",table);
				fprintf(fp,"CREATE TABLE IF NOT EXISTS `%s` (",table);
				for ( n=0 ; n<nfields ; n++ )
				{
					// determine type of data
					char *type=NULL;
					int size=0;
					switch ( fields[n].type ) {
					case MYSQL_TYPE_TINY: type="SMALLINT"; break;
					case MYSQL_TYPE_LONG: type="INTEGER"; break;
					case MYSQL_TYPE_LONGLONG: type="BIGINT"; break;
					case MYSQL_TYPE_DOUBLE: type="DOUBLE"; break;
					case MYSQL_TYPE_BIT: type="BIT"; break;
					case MYSQL_TYPE_STRING: type="CHAR"; size=fields[n].length; break;
					case MYSQL_TYPE_TIMESTAMP: type="TIMESTAMP"; break;
					default: break;
					}
					if ( type==NULL )
					{
						gl_warning("unable to determine type of field '%s' in table '%s' - using DOUBLE", table,fields[n].name);
						type = "DOUBLE";
					}

					// dump data
					if ( size>0 )
						fprintf(fp,"%s\n\t`%s` %s(%u)",n==0?"":", ",fields[n].name,type,size);
					else
						fprintf(fp,"%s\n\t`%s` %s",n==0?"":",",fields[n].name,type);
					if ( fields[n].flags&NOT_NULL_FLAG ) fprintf(fp," %s","NOT NULL");
					if ( fields[n].flags&AUTO_INCREMENT_FLAG ) fprintf(fp," %s","AUTO_INCREMENT");
					if ( fields[n].flags&PRI_KEY_FLAG ) fprintf(fp," %s","PRIMARY KEY");
				}
				fprintf(fp,"%s","\n);\n");
				fprintf(fp,"INSERT INTO `%s` (",table);
				for ( n=0 ; n<nfields ; n++ )
					fprintf(fp,"%s`%s`",n==0?"":",",fields[n].name);
				fprintf(fp,"%s",") VALUES\n");
			}

			// CSV format
			else
			{
				// field names
				for ( n=0 ; n<nfields ; n++ )
					fprintf(fp,"%s %s", n==0?"#":",", fields[n].name);
				fprintf(fp,"%s","\n");
			}
		}

		// dump row content
		if ( options&TD_BACKUP )
		{
			fprintf(fp,"%s",nrows>0?",\n\t(":"\t(");
			for ( n=0 ; n<nfields ; n++ )
				fprintf(fp,"%s'%s'",n==0?"":",", row[n]);
			fprintf(fp,"%s",")");
		}
		else
		{
			for ( n=0 ; n<nfields ; n++ )
				fprintf(fp,"%s%s",n==0?"":", ", row[n]);
			fprintf(fp,"%s","\n");
		}
		nrows++;
	}
	if ( nrows>0 )
	{
		if ( options&TD_BACKUP )
			fprintf(fp,"%s",";\n");
	}
Done:
	gl_verbose("dumped %u rows from table '%s' to file '%s'", nrows, table, file);
	mysql_free_result(result);
	fclose(fp);
	return nrows;
}
size_t database::backup(char *file)
{
	// prepare for output
	if ( get_options()&&DBO_OVERWRITE ) unlink(file);
	if ( access(file,0x00)==0 )
		exception("unable to backup of '%s' to '%s' - OVERWRITE not specified but file exists",get_schema(),file);

	// get list of tables
	MYSQL_RES *res = select("SHOW TABLES");
	if ( res==NULL )
	{
		gl_error("backup of '%s' failed - %s", file, mysql_error(mysql));
		return -1;
	}
	size_t ntables = mysql_num_rows(res);
	char **tables = new char*[ntables];
	int n;
	for ( n=0 ; n<ntables ; n++ )
	{
		MYSQL_ROW row;
		row = mysql_fetch_row(res);
		unsigned long *len = mysql_fetch_lengths(res);
		tables[n] = new char[len==NULL?1024:(len[0]+1)];
		strcpy(tables[n],row[0]);
	}
	mysql_free_result(res);

	// dump tables
	int nrows = 0;
	for ( n=0 ; n<ntables ; n++ )
	{
		size_t nr = dump(tables[n],file,TD_APPEND|TD_BACKUP);
		if ( nr<0 ) { nrows=-1; break; }
		nrows += nr;
	}
	
	for ( n=0 ; n<ntables ; n++ )
		delete [] tables[n];
	delete[] tables;
	return nrows;
}

#endif // HAVE_MYSQL
