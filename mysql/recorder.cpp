/** $Id: recorder.cpp 4738 2014-07-03 00:55:39Z dchassin $
 DP Chassin - July 2012
 Copyright (C) 2012 Battelle Memorial Institute
 **/

#ifdef HAVE_MYSQL

#include <time.h>
#include "database.h"
#include "query_engine.h"

EXPORT_CREATE(recorder);
EXPORT_INIT(recorder);
EXPORT_COMMIT(recorder);

CLASS *recorder::oclass = NULL;
recorder *recorder::defaults = NULL;
using namespace std;

vector<string> split(char* str, const char* delim) {
	char* saveptr;
	char* token = strtok_r(str, delim, &saveptr);

	vector<string> result;

	while (token != NULL) {
		result.push_back(token);
		token = strtok_r(NULL, delim, &saveptr);
	}
	return result;
}

recorder::recorder(MODULE *module) {
	if (oclass == NULL) {
		// register to receive notice for first top down. bottom up, and second top down synchronizations
		oclass = gld_class::create(module, "recorder", sizeof(recorder), PC_AUTOLOCK | PC_OBSERVER);
		if (oclass == NULL)
			throw "unable to register class recorder";
		else
			oclass->trl = TRL_PROTOTYPE;

		defaults = this;
		if (gl_publish_variable(oclass,
				PT_char1024, "property", get_property_offset(), PT_DESCRIPTION, "target property name",
				//				PT_char1024, "group", get_property_offset(), PT_DESCRIPTION,  "group definition string",
				PT_char32, "trigger", get_trigger_offset(), PT_DESCRIPTION, "recorder trigger condition",
				PT_char1024, "table", get_table_offset(), PT_DESCRIPTION, "table name to store samples",
				PT_char1024, "file", get_table_offset(), PT_DESCRIPTION, "file name (for tape compatibility)",
				PT_char32, "mode", get_mode_offset(), PT_DESCRIPTION, "table output mode",
				PT_int32, "limit", get_limit_offset(), PT_DESCRIPTION, "maximum number of records to output",
				PT_double, "interval[s]", get_interval_offset(), PT_DESCRIPTION, "sampling interval",
				PT_object, "connection", get_connection_offset(), PT_DESCRIPTION, "database connection",
				PT_set, "options", get_options_offset(), PT_DESCRIPTION, "SQL options",
				PT_KEYWORD, "PURGE", (set) MO_DROPTABLES, PT_DESCRIPTION, "flag to drop tables before creation",
				PT_KEYWORD, "UNITS", (set) MO_USEUNITS, PT_DESCRIPTION, "include units in column names",
				PT_char32, "datetime_fieldname", get_datetime_fieldname_offset(), PT_DESCRIPTION, "name of date-time field",
				PT_char32, "recordid_fieldname", get_recordid_fieldname_offset(), PT_DESCRIPTION, "name of record-id field",
				PT_char1024, "header_fieldnames", get_header_fieldnames_offset(), PT_DESCRIPTION, "name of header fields to include",
				PT_char256, "data_type", get_data_type_offset(), PT_DESCRIPTION, "the data format MySQL should use to store the values (default is DOUBLE with no precision set). Acceptable types are DECIMAL(M,D), FLOAT(M,D), and DOUBLE(M,D)",
				PT_int32, "query_buffer_limit", get_query_buffer_limit_offset(), PT_DESCRIPTION, "max number of queries to buffer before pushing to database",
				NULL) < 1) {
			char msg[256];
			sprintf(msg, "unable to publish properties in %s", __FILE__);
			throw msg;
		}

		memset(this, 0, sizeof(recorder));
	}
}

int recorder::create(void) {
	memcpy(this, defaults, sizeof(*this));
	db = last_database;
	strcpy(datetime_fieldname, "t");
	strcpy(recordid_fieldname, "id");
	strcpy(data_type, "DOUBLE");
	query_buffer_limit = 200;

	return 1; /* return 1 on success, 0 on failure */
}

int recorder::init(OBJECT *parent) {
// check the connection
	recorder_connection = new query_engine(
			get_connection() != NULL ? (database*) (get_connection() + 1) : db, query_buffer_limit, 200);

	query_engine* rc = recorder_connection;
	rc->set_table_root(get_table());
	rc->init_tables(recordid_fieldname, datetime_fieldname, true);

	// check mode
	if (strlen(mode) > 0) {
		options = 0xffffffff;
		struct {
				char *str;
				set bits;
		} modes[] = {
				{ "r", 0xffff },
				{ "r+", 0xffff },
				{ "w", MO_DROPTABLES },
				{ "w+", MO_DROPTABLES },
				{ "a", 0x0000 },
				{ "a+", 0x0000 },
		};
		int n;
		for (n = 0; n < sizeof(modes) / sizeof(modes[0]); n++) {
			if (strcmp(mode, modes[n].str) == 0)
					{
				options = modes[n].bits;
				break;
			}
		}
		if (options == 0xffffffff)
			exception("mode '%s' is not recognized", (const char*) mode);
		else if (options == 0xffff)
			exception("mode '%s' is not valid for a recorder", (const char*) mode);
	}

	// connect the target properties
	vector<string> property_specs = split(get_property(), ", \t;");
	char property_list[65536] = "";

	for (size_t n = 0; n < property_specs.size(); n++) {
		char buffer[1024];
		strcpy(buffer, (const char*) property_specs[n].c_str());
		vector<string> spec = split(buffer, "[]");
		if (spec.size() > 0) {
			strcpy(buffer, (const char*) spec[0].c_str());
			gld_property prop;
			if (get_parent() == NULL)
				prop = gld_property(buffer);
			else
				prop = gld_property(get_parent(), buffer);
			if (prop.get_object() == NULL) {
				if (get_parent() == NULL)
					exception("parent object is not set");
				prop = gld_property(get_parent(), buffer);
			}
			if (!prop.is_valid())
				exception("property %s is not valid", buffer);

			property_target.push_back(prop);
			gl_debug("adding field from property '%s'", buffer);
			double scale = 1.0;
			gld_unit unit;
			if (spec.size() > 1) {
				char buffer[1024];
				strcpy(buffer, (const char*) spec[1].c_str());
				unit = gld_unit(buffer);
			}
			else if (prop.get_unit() != NULL && (options & MO_USEUNITS))
				unit = *prop.get_unit();
			property_unit.push_back(unit);
			n_properties++;
			char tmp[2][2][128];
			sprintf(tmp[0][0], "%s", prop.get_sql_safe_name());
			sprintf(tmp[0][1], "`%s` %s, ", prop.get_sql_safe_name(), db->get_sqltype(prop));
			sprintf(tmp[1][0], "%s_units", prop.get_sql_safe_name());
			sprintf(tmp[1][1], "`%s_units` %s, ", prop.get_sql_safe_name(), "CHAR(10)");

			if (unit.is_valid()) {
				rc->get_table_path()->add_table_header(new string(tmp[0][0]), new string(tmp[0][1]));
				rc->get_table_path()->add_table_header(new string(tmp[1][0]), new string(tmp[1][1]));
			} else {
				rc->get_table_path()->add_table_header(new string(tmp[0][0]), new string(tmp[0][1]));
			}
		}
	}

	// get header fields
	if (strlen(header_fieldnames) > 0) {
		if (get_parent() == NULL)
			exception("cannot find header fields without a parent");
		char buffer[1024];
		strcpy(buffer, header_fieldnames);
		vector<string> header_specs = split(buffer, ",");
		size_t header_pos = 0;
		for (size_t n = 0; n < header_specs.size(); n++) {
			if (header_specs[n].compare("name") == 0) {
				rc->get_table_path()->add_table_header(new string("name"), new string("name CHAR(64), index i_name (name), "));
			}
			else if (header_specs[n].compare("class") == 0) {
				rc->get_table_path()->add_table_header(new string("class"), new string("class CHAR(32), index i_class (class), "));
			}
			else if (header_specs[n].compare("latitude") == 0) {
				rc->get_table_path()->add_table_header(new string("latitude"), new string("latitude DOUBLE, index i_latitude (latitude), "));
			}
			else if (header_specs[n].compare("longitude") == 0) {
				rc->get_table_path()->add_table_header(new string("longitude"), new string("longitude DOUBLE, index i_longitude (longitude), "));
			}
			else
				exception("header field %s does not exist", (const char*) header_specs[n].c_str());
		}
		gl_verbose("header_fieldname=[%s]", (const char*) header_fieldnames);
	}

	// check for table existence and create if not found
	if (n_properties > 0) {
		// drop table if exists and drop specified
		if (db->table_exists(get_table())) {
			if (get_options() & MO_DROPTABLES && !db->query("DROP TABLE IF EXISTS `%s`", rc->get_table().get_string()))
				exception("unable to drop table '%s'", rc->get_table().get_string());
		}

		// create table if not exists
		ostringstream query;
		string query_string;
		string temp = rc->get_table_path()->get_table_header_buffer().str();

		query << "CREATE TABLE IF NOT EXISTS `" << rc->get_table().get_string() << "` ("
				"`" << recordid_fieldname << "` INT AUTO_INCREMENT PRIMARY KEY, "
				"`" << datetime_fieldname << "` DATETIME, "
				"" << rc->get_table_path()->get_table_header_buffer().str() << ""
				"INDEX `i_" << datetime_fieldname << "` "
				"(`" << datetime_fieldname << "`))";
		if (!db->table_exists(rc->get_table().get_string())) {
			if (!(get_options() & MO_NOCREATE)) {
				query_string = query.str();
				const char* query_char_string = query_string.c_str();
				if (!db->query(query_char_string))
					exception("unable to create table '%s' in schema '%s'", rc->get_table().get_string(), db->get_schema());
				else
					gl_verbose("table %s created ok", rc->get_table().get_string());
			} else
				exception("NOCREATE option prevents creation of table '%s'", rc->get_table().get_string());
		}

// check row count
		else {
			if (db->select("SELECT count(*) FROM `%s`", rc->get_table().get_string()) == NULL)
				exception("unable to get row count of table '%s'", rc->get_table().get_string());

			gl_verbose("table '%s' ok", rc->get_table().get_string());
		}
	}
	else {
		exception("no properties specified");
		return 0;
	}

	// set heartbeat
	if (interval > 0) {
		set_heartbeat((TIMESTAMP) interval);
		enabled = true;
	}

	// arm trigger, if any
	if (enabled && trigger[0] != '\0') {
		// read trigger condition
		if (sscanf(trigger, "%[<>=!]%s", compare_op, compare_val) == 2) {
			// enable trigger and suspend data collection
			trigger_on = true;
			enabled = false;
			gl_debug("%s: trigger '%s' enabled", get_name(), get_trigger());
		}
	}

	return 1;
}

EXPORT TIMESTAMP heartbeat_recorder(OBJECT *obj) {
	recorder *my = OBJECTDATA(obj, recorder);
	if (!my->get_trigger_on() && !my->get_enabled())
		return TS_NEVER;
	obj->clock = gl_globalclock;
	TIMESTAMP dt = (TIMESTAMP) my->get_interval();

	// recorder is always a soft event
	return -(obj->clock / dt + 1) * dt;
}

TIMESTAMP recorder::commit(TIMESTAMP t0, TIMESTAMP t1) {
	query_engine* rc = recorder_connection;

	// check trigger
	if (trigger_on) {
		// trigger condition
		if (property_target[0].compare(compare_op, compare_val))
				{
			// disable trigger and enable data collection
			trigger_on = false;
			enabled = true;
		}
	}
	else if (trigger[0] == '\0')
		enabled = true;

	// check sampling interval
	gl_debug("%s: interval=%.0f, clock=%lld", get_name(), interval, gl_globalclock);
	if (interval > 0) {
		if ( gl_globalclock % ((TIMESTAMP) interval) != 0)
			return TS_NEVER;
		else
			gl_verbose("%s: sampling time has arrived", get_name());
	}

	// collect data
	if (enabled) {
		gl_debug("header_fieldname=[%s]", (const char*) header_fieldnames);
		gl_debug("header_fielddata=[%s]", header_data);
		char fieldlist[65536] = "", valuelist[65536] = "";
		size_t fieldlen = 0;
		if (header_fieldnames[0] != '\0')
			fieldlen = sprintf(fieldlist, ",%s", (const char*) header_fieldnames);
		strcpy(valuelist, header_data);
		size_t valuelen = strlen(valuelist);
		for (size_t n = 0; n < property_target.size(); n++) {
			string* name_string = new string(property_target[n].get_sql_safe_name());
			char buffer[1024] = "NULL";

			if (property_unit[n].is_valid()) {
				db->get_sqldata(buffer, sizeof(buffer), property_target[n], &property_unit[n]);
				rc->get_table_path()->add_insert_values(rc, name_string, string(buffer));
				rc->get_table_path()->add_insert_values(rc, new string(*name_string + "_units"), string(property_unit[n].get_name()));
			}
			else {
				db->get_sqldata(buffer, sizeof(buffer), property_target[n], &property_unit[n]);
				rc->get_table_path()->add_insert_values(rc, name_string, string(buffer));
			}
		}

		// This partially duplicates some logic used above, and I don't like it, so it may be reworked later.
		char header_buffer[1024];
		strcpy(header_buffer, header_fieldnames);
		vector<string> header_specs = split(header_buffer, ",");
		for (size_t n = 0; n < header_specs.size(); n++) {
			if (header_specs[n].compare("name") == 0) {
				rc->get_table_path()->add_insert_values(rc, new string("name"), string("'" + string(get_parent()->get_name()) + "'"));
			}
			else if (header_specs[n].compare("class") == 0) {
				rc->get_table_path()->add_insert_values(rc, new string("class"), string("'" + string(get_parent()->get_oclass()->get_name()) + "'"));
			}
			else if (header_specs[n].compare("latitude") == 0) {
				if (isnan(get_parent()->get_latitude()))
					rc->get_table_path()->add_insert_values(rc, new string("latitude"), string("NULL"));
				else
					rc->get_table_path()->add_insert_values(rc, new string("latitude"), string("'" + to_string(get_parent()->get_latitude()) + "'"));
			}
			else if (header_specs[n].compare("longitude") == 0) {
				if (isnan(get_parent()->get_longitude()))
					rc->get_table_path()->add_insert_values(rc, new string("longitude"), string("NULL"));
				else
					rc->get_table_path()->add_insert_values(rc, new string("longitude"), string("'" + to_string(get_parent()->get_longitude()) + "'"));
			}
			else
				exception("header field %s does not exist", (const char*) header_specs[n].c_str());
		}

// check limit
		if (get_limit() > 0 && db->get_last_index() >= get_limit()) {
			rc->get_table_path()->commit_state();

			// shut off recorder
			rc->set_tables_done();
			enabled = false;
			gl_verbose("table '%s' size limit %d reached", rc->get_table().get_string(), get_limit());
		}
	}
	else {
		gl_debug("%s: sampling is not enabled", get_name());
	}

	rc->get_table_path()->flush_value_row(&t0);

	if (gl_globalclock == gl_globalstoptime) {
		rc->get_table_path()->flush_value_row(&t1);
		rc->get_table_path()->commit_state();
		rc->set_tables_done();
	}

	return TS_NEVER;
}

template<class T> std::string recorder::to_string(T t) {
	std::string returnBuffer;
	std::ostringstream string_conversion_buffer;
	string_conversion_buffer.precision(15);
	string_conversion_buffer << t;
	returnBuffer = string_conversion_buffer.str();
	return returnBuffer;
}

#endif // HAVE_MYSQL
