/** $Id: recorder.cpp 4738 2014-07-03 00:55:39Z dchassin $
    DP Chassin - July 2012
    Copyright (C) 2012 Battelle Memorial Institute
 **/

#ifdef HAVE_MYSQL

#include "database.h"

EXPORT_CREATE(recorder);
EXPORT_INIT(recorder);
EXPORT_COMMIT(recorder);

CLASS *recorder::oclass = NULL;
recorder *recorder::defaults = NULL;

recorder::recorder(MODULE *module)
{
	if (oclass==NULL)
	{
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module,"recorder",sizeof(recorder),PC_AUTOLOCK|PC_OBSERVER);
		if (oclass==NULL)
			throw "unable to register class recorder";
		else
			oclass->trl = TRL_PROTOTYPE;

		defaults = this;
		if (gl_publish_variable(oclass,
			PT_char1024,"property",get_property_offset(),PT_DESCRIPTION,"target property name",
			PT_char32,"trigger",get_trigger_offset(),PT_DESCRIPTION,"recorder trigger condition",
			PT_char1024,"table",get_table_offset(),PT_DESCRIPTION,"table name to store samples",
			PT_char1024,"file",get_table_offset(),PT_DESCRIPTION,"file name (for tape compatibility)",
			PT_char32,"mode",get_mode_offset(),PT_DESCRIPTION,"table output mode",
			PT_int32,"limit",get_limit_offset(),PT_DESCRIPTION,"maximum number of records to output",
			PT_double,"interval[s]",get_interval_offset(),PT_DESCRIPTION,"sampling interval",
			PT_object,"connection",get_connection_offset(),PT_DESCRIPTION,"database connection",
			PT_set,"options",get_options_offset(),PT_DESCRIPTION,"SQL options",
				PT_KEYWORD,"PURGE",(int64)MO_DROPTABLES,PT_DESCRIPTION,"flag to drop tables before creation",
				PT_KEYWORD,"UNITS",(int64)MO_USEUNITS,PT_DESCRIPTION,"include units in column names",
			NULL)<1){
				char msg[256];
				sprintf(msg, "unable to publish properties in %s",__FILE__);
				throw msg;
		}

		memset(this,0,sizeof(recorder));
	}
}

int recorder::create(void) 
{
	memcpy(this,defaults,sizeof(*this));
	db = last_database;
	return 1; /* return 1 on success, 0 on failure */
}

int recorder::init(OBJECT *parent)
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
			exception("mode '%s' is not recognized",(const char*)mode);
		else if ( options==0xffff )
			exception("mode '%s' is not valid for a recorder", (const char*)mode);
	}

	// connect the target property
	if ( get_parent()==NULL )
		exception("parent is not set");
	target.set_object(get_parent());
	char propname[64]="", propunit[64]="";
	switch ( sscanf(get_property(),"%[^[][%[^]]",propname,propunit) ) {
	case 2:
		if ( !unit.set_unit(propunit) )
			exception("property '%s' has an invalid unit", get_property());
		// drop through
	case 1:
		strncpy(field,propname,sizeof(field)-1);
		target.set_property(propname);
		scale = 1.0;
		if ( unit.is_valid() && target.get_unit()!=NULL )
		{
			target.get_unit()->convert(unit,scale);
			sprintf(field,"%s[%s]",propname,propunit);
		}
		else if ( propunit[0]=='\0' && options&MO_USEUNITS && target.get_unit() )
			sprintf(field,"%s[%s]",propname,target.get_unit()->get_name());
		break;
	default:
		exception("property '%s' is not valid", get_property());
		break;
	}

	// check for table existence and create if not found
	if ( target.is_valid() )
	{
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
				if ( !db->query("CREATE TABLE IF NOT EXISTS `%s` ("
					"id INT AUTO_INCREMENT PRIMARY KEY, "
					"t TIMESTAMP, "
					"`%s` %s, "
					"INDEX i_t (t) "
					")", 
					get_table(), (const char*)field, db->get_sqltype(target)) )
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
	}
	else
	{
		exception("property '%s' is not valid", get_property());
		return 0;
	}

	// set heartbeat
	if ( interval>0 )
	{
		set_heartbeat((TIMESTAMP)interval);
		enabled = true;
	}

	// arm trigger, if any
	if ( enabled && trigger[0]!='\0' )
	{
		// read trigger condition
		if ( sscanf(trigger,"%[<>=!]%s",compare_op,compare_val)==2 )
		{
			// rescale comparison value if necessary
			if ( scale!=1.0 )
				sprintf(compare_val,"%g",atof(compare_val)/scale);

			// enable trigger and suspend data collection
			trigger_on=true;
			enabled=false;
		}
	}

	// prepare insert statement
	char statement[1024];
	int len = sprintf(statement,"INSERT INTO `%s` (`%s`) VALUES ('?')", get_table(), (const char*)field);
	gl_verbose("preparing statement '%s'", statement);
	insert = mysql_stmt_init(db->get_handle());
	if ( !db->get_sqlbind(value,target,&stmt_error) )
	{
		gl_warning("unable to bind target '%s'", target.get_name());
		mysql_stmt_close(insert);
		insert = NULL;
	}
	else if ( mysql_stmt_prepare(insert,statement,len)!=0 )
	{
		gl_warning("insert statement '%s' prepare failed (error='%s'), using slowing 'INSERT' method", statement, mysql_stmt_error(insert));
		mysql_stmt_close(insert);
		insert = NULL;
	}
	else if ( mysql_stmt_bind_param(insert,&value)!=0 )
	{
		gl_warning("insert statement '%s' bind failed (error='%s'), using slowing 'INSERT' method", statement, mysql_stmt_error(insert));
		mysql_stmt_close(insert);
		insert = NULL;
	}

	return 1;
}

EXPORT TIMESTAMP heartbeat_recorder(OBJECT *obj)
{
	recorder *my = OBJECTDATA(obj,recorder);
	if ( !my->get_trigger_on() && !my->get_enabled() ) return TS_NEVER;
	obj->clock = gl_globalclock;
	TIMESTAMP dt = (TIMESTAMP)my->get_interval();
	
	// recorder is always a soft event
	return -(obj->clock/dt+1)*dt;
}

TIMESTAMP recorder::commit(TIMESTAMP t0, TIMESTAMP t1)
{
	// check trigger
	if ( trigger_on )
	{
		// trigger condition
		if ( target.compare(compare_op,compare_val) )
		{
			// disable trigger and enable data collection
			trigger_on = false;
			enabled = true;
		}
#ifdef _DEBUG
		else
		{
			char buffer[1024];
			target.to_string(buffer,sizeof(buffer));
			gl_verbose("trigger %s.%s not activated - '%s %s %s' did not pass", get_name(), get_property(), buffer, compare_op,compare_val);
		}
#endif
	}

	// collect data
	if ( enabled )
	{
		// convert data
		bool have_data = false;
		if ( target.is_double() )
		{
			real = target.get_double()*scale;
			gl_verbose("%s sampling: %s=%g", get_name(), target.get_name(), real);
			have_data = true;
		}
		else if ( target.is_integer() ) 
		{
			integer = target.get_integer();
			gl_verbose("%s sampling: %s=%lli", get_name(), target.get_name(), integer);
			have_data = true;
		}
		else if ( db->get_sqldata(string,sizeof(string)-1,target) )
		{
			gl_verbose("%s sampling: %s='%s'", get_name(), target.get_name(), (const char*)string);
			have_data = true;
		}
		else
		{
			gl_verbose("%s sampling: unable to sample %s", get_name(), target.get_name());
			have_data = false;
		}

		if ( have_data )
		{
			// use prepared statement if possible
			if ( insert )
			{
				if ( mysql_stmt_execute(insert)!=0 || stmt_error || mysql_stmt_affected_rows(insert)==0 )
				{
					int64 n = mysql_stmt_affected_rows(insert);
					gl_warning("unable to execute insert statement for target '%s' (%s) - reverting to slower INSERT", target.get_name(), mysql_stmt_error(insert));
					mysql_stmt_close(insert);
					insert = NULL;

					// if insert totally failed to add the row
					if ( n==0 )
						goto Insert;
				}
			}

			// use slower INSERT statement
			else
			{
Insert:
				// send data
				if ( target.is_double() )
				{
					db->query("INSERT INTO `%s` (t, `%s`) VALUES (from_unixtime('%"FMT_INT64"d'), '%.8g')",
						get_table(), (const char*)field, gl_globalclock, real);
				}
				else if ( target.is_integer() )
				{
					db->query("INSERT INTO `%s` (t, `%s`) VALUES (from_unixtime('%"FMT_INT64"d'), '%lli')",
						get_table(), (const char*)field, gl_globalclock, integer);
				}
				else
					db->query("INSERT INTO `%s` (t, `%s`) VALUES (from_unixtime('%"FMT_INT64"d'), '%s')",
						get_table(), (const char*)field, gl_globalclock, (const char*)string);
			}

			// check limit
			if ( get_limit()>0 && db->get_last_index()>=get_limit() )
			{
				// shut off recorder
				enabled=false;
				gl_verbose("table '%s' size limit %d reached", get_table(), get_limit());
			}
		}
	}
	
	return TS_NEVER;
}

#endif // HAVE_MYSQL
