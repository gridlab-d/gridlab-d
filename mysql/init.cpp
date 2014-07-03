/** $Id: init.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/

#ifdef HAVE_MYSQL

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "database.h"

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

	// initialize the client
	mysql_client = mysql_init(mysql_client);
	if ( mysql_client==NULL )
	{
		errno = ENOENT;
		return NULL;
	}

	// set options
	my_bool report_truncate = false;
	if ( mysql_options(mysql_client,MYSQL_REPORT_DATA_TRUNCATION,&report_truncate)!=0 )
		gl_warning("unable to disable data truncation reporting in mysql");

	gl_global_create("mysql::hostname",PT_char256,default_hostname,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL server name",NULL);
	gl_global_create("mysql::username",PT_char32,default_username,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL user name",NULL);
	gl_global_create("mysql::password",PT_char32,default_password,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL user password",NULL);
	gl_global_create("mysql::schema",PT_char256,default_schema,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL database schema name",NULL);
	gl_global_create("mysql::port",PT_int32,&default_port,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL server port number (TCP/IP only)",NULL);
	gl_global_create("mysql::socketname",PT_char1024,default_socketname,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL socket name (unix only)",NULL);
	gl_global_create("mysql::clientflags",PT_set,&default_clientflags,PT_ACCESS,PA_PUBLIC,PT_DESCRIPTION,"default MySQL client flags",
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
		NULL);

	new database(module);
	new recorder(module);
	new player(module);
	new collector(module);

	static int32 mysql_flag = 1;
	gl_global_create("MYSQL",PT_int32,&mysql_flag,PT_ACCESS,PA_REFERENCE,PT_DESCRIPTION,"indicates that MySQL is available",NULL);

	/* always return the first class registered */
	return database::oclass;
}

EXPORT void term(void)
{
	database *db;
	for ( db=database::get_first() ; db!=NULL ; db=db->get_next() )
		db->term();
}


#else // !HAVE_MYSQL
#include "gridlabd.h"
EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
	gl_error("mysql module was built on a system without mysql libraries");
	return NULL;
}
#endif // HAVE_MYSQL

