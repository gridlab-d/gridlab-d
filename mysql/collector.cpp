/** $Id: collector.cpp 4738 2014-07-03 00:55:39Z dchassin $
    DP Chassin - July 2012
    Copyright (C) 2012 Battelle Memorial Institute
 **/

#ifdef HAVE_MYSQL

#include "database.h"

EXPORT_CREATE(collector);
EXPORT_INIT(collector);
EXPORT_COMMIT(collector);

CLASS *collector::oclass = NULL;
collector *collector::defaults = NULL;

collector::collector(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"collector",sizeof(collector),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class collector";
		else
			oclass->trl = TRL_PROTOTYPE;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_char1024,"property",get_property_offset(),PT_DESCRIPTION,"target property name",
			PT_char1024,"group",get_group_offset(),PT_DESCRIPTION,"collector group specification",
			PT_char1024,"table",get_table_offset(),PT_DESCRIPTION,"table name to store samples",
			PT_char1024,"file",get_table_offset(),PT_DESCRIPTION,"file name (for tape compatibility)",
			PT_char32,"mode",get_mode_offset(),PT_DESCRIPTION,"table output mode",
			PT_int32,"limit",get_limit_offset(),PT_DESCRIPTION,"maximum number of records to output",
			PT_double,"interval[s]",get_interval_offset(),PT_DESCRIPTION,"sampling interval",
			PT_object,"connection",get_connection_offset(),PT_DESCRIPTION,"database connection",
			PT_set,"options",get_options_offset(),PT_DESCRIPTION,"SQL options",
				PT_KEYWORD,"PURGE",(int64)MO_DROPTABLES,PT_DESCRIPTION,"flag to drop tables before creation",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(collector));
	}
}

int collector::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	db = last_database;
	return 1; /* return 1 on success, 0 on failure */
}

int collector::init(OBJECT *parent)
{
	// check the connection
	if ( get_connection()!=NULL )
		db = (database*)(get_connection()+1);
	if ( db==NULL )
		exception("no database connection available or specified");
	if ( !db->isa("database") )
		exception("connection is not a mysql database");
	gl_verbose("connection to mysql server '%s', schema '%s' ok", db->get_hostname(), db->get_schema());

	// check mode
	if ( strlen(mode)>0 )
	{
		options = 0xffffffff;
		struct {
			char *str;
			set bits;
		} modes[] = {
			{"r",	0xffff},
			{"r+",	0xffff},
			{"w",	MO_DROPTABLES},
			{"w+",	MO_DROPTABLES},
			{"a",	0x0000},
			{"a+",	0x0000},
		};
		int n;
		for ( n=0 ; n<sizeof(modes)/sizeof(modes[0]) ; n++ )
		{
			if ( strcmp(mode,modes[n].str)==0 )
			{
				options = modes[n].bits;
				break;
			}
		}
		if ( options==0xffffffff )
			exception("mode '%s' is not recognized",(const char *)mode);
		else if ( options==0xffff )
			exception("mode '%s' is not valid for a recorder", (const char *)mode);
	}

	// verify group is defined
	if ( strcmp(get_group(),"")==0 )
		exception("group must be specified");

	// verify property is defined
	if ( strcmp(get_property(),"")==0 )
		exception("at least one property aggregation must be specified");

	// copy property spec into working buffer
	char1024 propspecs;
	strcpy(propspecs,get_property());

	// count properties in specs
	char *p = propspecs;
	n_aggregates = 0;
	do {
		n_aggregates++;
		p = strchr(p,',');
	} while (p++!=NULL);
	list = new gld_aggregate[n_aggregates];
	names = new char*[n_aggregates];

	// load property structs
	int n;
	for ( p=propspecs,n=0 ; n<n_aggregates ; n++ )
	{
		char *np = strchr(p,',');
		if ( np!=NULL ) *np='\0';
		while ( *p!='\0' && isspace(*p) ) p++; // left trim
		int len = strlen(p);
		while ( len>0 && isspace(p[len-1]) ) p[--len]='\0'; // right trim
		if ( !list[n].set_aggregate(p,get_group()) )
			exception("unable to aggregate '%s' over group '%s'", p, get_group());
		gl_debug("%s: group '%s' aggregate '%s' initial value is '%lf'", get_name(), get_group(), p, list[n].get_value());
		names[n] = new char[strlen(p)+1];
		strcpy(names[n],p);
		p = np+1;
	}
	gl_debug("%s: %d aggregates ok", get_name(), n_aggregates);

	// drop table if exists and drop specified
	if ( db->table_exists(get_table()) )
	{
		if ( get_options()&MO_DROPTABLES && !db->query("DROP TABLE IF EXISTS `%s`", get_table()) )
			exception("unable to drop table '%s'", get_table());
	}
	
	// create table if not exists
	if ( !db->table_exists(get_table()) )
	{
		if ( !(options&MO_NOCREATE) )
		{
			char buffer[4096];
			size_t eos = sprintf(buffer,"CREATE TABLE IF NOT EXISTS `%s` ("
				"id INT AUTO_INCREMENT PRIMARY KEY, "
				"t TIMESTAMP, ", get_table());
			int n;
			for ( n=0 ; n<n_aggregates ; n++ )
				eos += sprintf(buffer+eos,"`%s` double, ",names[n]);
			eos += sprintf(buffer+eos,"%s","INDEX i_t (t))");

			if ( !db->query(buffer) )
				exception("unable to create table '%s' in schema '%s'", get_table(), db->get_schema());
			else
				gl_verbose("table %s created ok", get_table());
		}
		else
			exception("NOCREATE option prevents creation of table '%s'", get_table());
	}

	// check row count
	else 
	{
		if ( db->select("SELECT count(*) FROM `%s`", get_table())==NULL )
			exception("unable to get row count of table '%s'", get_table());

		gl_verbose("table '%s' ok", get_table());
	}
	
	// first event time
	TIMESTAMP dt = (TIMESTAMP)get_interval();
	if ( dt>0 )
		next_t = (TIMESTAMP)(gl_globalclock/dt+1)*dt;
	else if ( dt==0 )
		next_t = TS_NEVER;
	else
		exception("%s: interval must be zero or positive");
		
	// @todo use prepared statement instead of insert

	// set heartbeat
	if ( interval>0 )
		set_heartbeat((TIMESTAMP)interval);

	return 1;
}

EXPORT TIMESTAMP heartbeat_collector(OBJECT *obj)
{
	collector *my = OBJECTDATA(obj,collector);
	obj->clock = gl_globalclock;
	TIMESTAMP dt = (TIMESTAMP)my->get_interval();
	
	// recorder is always a soft event
	return my->next_t==TS_NEVER ? TS_NEVER : -(obj->clock/dt+1)*dt;
}
TIMESTAMP collector::commit(TIMESTAMP t0, TIMESTAMP t1)
{
	TIMESTAMP dt = (TIMESTAMP)get_interval();
	if ( dt==0 || ( t1==next_t && next_t!=TS_NEVER ) )
	{
		char buffer[4096];
		size_t eos = sprintf(buffer,"INSERT INTO `%s` (t", get_table());
		int n;
		for ( n=0 ; n<n_aggregates ; n++ )
			eos += sprintf(buffer+eos,",`%s`",names[n]);
		eos += sprintf(buffer+eos,") VALUES (from_unixtime(%lli)",gl_globalclock);
		for ( n=0 ; n<n_aggregates ; n++ )
			eos += sprintf(buffer+eos,",%g",list[n].get_value());
		sprintf(buffer+eos,"%s",")");

		if ( !db->query(buffer) )
			exception("unable to add data to '%s' - %s", get_table(), mysql_error(mysql));
		else
			gl_verbose("%s: sample added to '%s' ok", get_name(), get_table());

		// check limit
		if ( get_limit()>0 && db->get_last_index()>=get_limit() )
		{
			gl_verbose("%s: limit of %d records reached", get_name(), get_limit());
			next_t = TS_NEVER;
		}
		else
		{
			next_t = (dt==0 ? TS_NEVER : (TIMESTAMP)(t1/dt+1)*dt);
		}
		set_clock(t1);
	}
	return TS_NEVER;
}

#endif // HAVE_MYSQL
