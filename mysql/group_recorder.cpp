#ifdef HAVE_MYSQL

#include "group_recorder.h"
#include <sstream>
#include <iostream>
#include <algorithm>
#include "database.h"
#include "query_engine.h"

EXPORT_CREATE(group_recorder);
EXPORT_INIT(group_recorder);

CLASS *group_recorder::oclass = NULL;
CLASS *group_recorder::pclass = NULL;
group_recorder *group_recorder::defaults = NULL;
using namespace std;

vector<string> group_recorder::gr_split(char* str, const char* delim) {
	char* saveptr;
	char* token = strtok_r(str, delim, &saveptr);

	vector<string> result;

	while (token != NULL) {
		result.push_back(token);
		token = strtok_r(NULL, delim, &saveptr);
	}
	return result;
}

void new_group_recorder(MODULE *mod) {
	new group_recorder(mod);
}

group_recorder::group_recorder(MODULE *mod) {
	if (oclass == NULL) {
#ifdef _DEBUG
		gl_debug("construction group_recorder class");
#endif
		oclass = gl_register_class(mod, "group_recorder", sizeof(group_recorder), PC_POSTTOPDOWN);
		if (oclass == NULL)
			GL_THROW("unable to register object class implemented by %s", __FILE__);

		if (gl_publish_variable(oclass,
				PT_char1024, "file", PADDR(table), PT_DESCRIPTION, "output file name (legacy compatibility, applied to table field)",
				PT_char1024, "table", PADDR(table), PT_DESCRIPTION, "database table name",
				PT_char1024, "group", PADDR(group_def), PT_DESCRIPTION, "group definition string",
				PT_bool, "strict", PADDR(strict), PT_DESCRIPTION, "causes the group_recorder to stop the simulation should there be a problem communicating with the database",
				PT_bool, "print_units", PADDR(print_units), PT_DESCRIPTION, "flag to record units for each written object, if applicable",
				PT_char256, "property", PADDR(property_name), PT_DESCRIPTION, "property to record",
				PT_int32, "limit", PADDR(limit), PT_DESCRIPTION, "the maximum number of lines to write to the database",
				PT_char32, "mode", PADDR(mode), PT_DESCRIPTION, "table output mode",
				PT_int32, "query_buffer_limit", PADDR(query_buffer_limit), PT_DESCRIPTION, "max number of queries to buffer before pushing to database",
				PT_int32, "column_limit", PADDR(column_limit), PT_DESCRIPTION, "maximum number of columns per MySQL table (default 200)",
				PT_char32, "datetime_fieldname", PADDR(datetime_fieldname), PT_DESCRIPTION, "name of date-time field",
				PT_char32, "recordid_fieldname", PADDR(recordid_fieldname), PT_DESCRIPTION, "name of record-id field",
				PT_char256, "complex_part", PADDR(complex_part), PT_DESCRIPTION, "the complex part(s) to record if complex properties are gathered (default REAL|IMAG)",
				PT_char256, "data_type", PADDR(data_type), PT_DESCRIPTION, "the data format MySQL should use to store the values (default is DOUBLE with no precision set). Acceptable types are DECIMAL(M,D), FLOAT(M,D), and DOUBLE(M,D)",
				PT_double, "interval[s]", PADDR(dInterval), PT_DESCRIPTION, "recording interval (0 'every iteration', -1 'on change')",
				PT_bool, "legacy_mode", PADDR(legacy_mode), PT_DESCRIPTION, "Produces output consistent with the tape group_recorder formatting, rather than the new database optimized formatting",

				// legacy compatibility components.
				PT_bool, "format", PADDR(format), PT_DESCRIPTION, "(unused) legacy variable for compatibility with tape group_recorder",
				PT_double, "flush_interval[s]", PADDR(dFlush_interval), PT_DESCRIPTION, "(unused) legacy variable for compatibility with tape group_recorder",
				NULL) < 1) {
			; //GL_THROW("unable to publish properties in %s",__FILE__);
		}
		defaults = this;
		memset(this, 0, sizeof(group_recorder));
	}
}

group_recorder::~group_recorder() {
	delete group_recorder_connection;
}

int group_recorder::create(void) {
	memcpy(this, defaults, sizeof(*this));
	db = last_database;
	strcpy(datetime_fieldname, "t");
	strcpy(recordid_fieldname, "id");
	strcpy(data_type, "DOUBLE");
	strcpy(complex_part, "REAL|IMAG");
	query_buffer_limit = 200;
	column_limit = 200;
	print_units = false;
	legacy_mode = false;

	return 1; /* return 1 on success, 0 on failure */
}

int group_recorder::init(OBJECT *obj) {
	OBJECT *gr_obj = 0;

	if (get_table()[0] == '\0')
			{
		gl_error("Both 'table' and 'file' fields are empty. Database target is unknown.");
		return 0;
	}

	// check the connection
	group_recorder_connection = new query_engine(
			get_connection() != NULL ? (database*) (get_connection() + 1) : db, query_buffer_limit, column_limit);

	// check mode
	if (strlen(mode) > 0) {
		options = 0xffffffff;
		struct {
				char *str;
				set bits;
		} modes[] = { { "r", 0xffff }, { "r+", 0xffff }, { "w", MO_DROPTABLES }, { "w+", MO_DROPTABLES }, { "a", 0x0000 }, { "a+", 0x0000 }, };
		int n;
		for (n = 0; n < sizeof(modes) / sizeof(modes[0]); n++) {
			if (strcmp(mode, modes[n].str) == 0) {
				options = modes[n].bits;
				break;
			}
		}
		if (options == 0xffffffff)
			exception("mode '%s' is not recognized", (const char*) mode);
		else if (options == 0xffff)
			exception("mode '%s' is not valid for group recorder", (const char*) mode);
	}

	// check for group
	if (0 == group_def[0]) {
		if (strict) {
			gl_error("group_recorder::init(): no group defined");
			/* TROUBLESHOOT
			 group_recorder must define a group in "group_def".
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): no group defined");
			return 1; // nothing more to do
		}
	}

	// check valid write interval
	write_interval = (int64) (dInterval);
	if (-1 > write_interval) {
		gl_error("group_recorder::init(): invalid write_interval of %i, must be -1 or greater", write_interval);
		/* TROUBLESHOOT
		 The group_recorder interval must be -1, 0, or a positive number of seconds.
		 */
		return 0;
	}

	// build group
	//	* invariant?
	//	* non-empty set?
	items = gl_find_objects(FL_GROUP, group_def.get_string());
	if (0 == items) {
		if (strict) {
			gl_error("group_recorder::init(): unable to construct a set with group definition");
			/* TROUBLESHOOT
			 An error occured while attempting to build and populate the find list with the specified group definition.
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): unable to construct a set with group definition");
			return 1; // nothing more to do
		}
	}
	if (1 > items->hit_count) {
		if (strict) {
			gl_error("group_recorder::init(): the defined group returned an empty set");
			/* TROUBLESHOOT
			 Placeholder.
			 */
			return 0;
		} else {
			gl_warning("group_recorder::init(): the defined group returned an empty set");
			return 1;
		}
	}

	// turn list into objlist, count items
	obj_count = 0;
	for (gr_obj = gl_find_next(items, 0); gr_obj != 0;
			gr_obj = gl_find_next(items, gr_obj)) {
		prop_ptr = gl_get_property(gr_obj, property_name.get_string());
		// might make this a 'strict-only' issue in the future
		if (prop_ptr == NULL) {
			gl_error("group_recorder::init(): unable to find property '%s' in an object of type '%s'", property_name.get_string(), gr_obj->oclass->name);
			/* TROUBLESHOOT
			 An error occured while reading the specified property in one of the objects.
			 */
			return 0;
		}
		++obj_count;
		if (obj_list == 0) {
			obj_list = new quickobjlist(gr_obj, prop_ptr);
		} else {
			obj_list->tack(gr_obj, prop_ptr);
		}
	}

	// check if we should expunge the units from our copied PROP structs
	if (!print_units) {
		quickobjlist *itr = obj_list;
		for (; itr != 0; itr = itr->next) {
			itr->prop.unit = NULL;
		}
	}

	if (0 == write_header()) {
		gl_error("group_recorder::init(): an error occured when writing the file header");
		/* TROUBLESHOOT
		 Unexpected IO error.
		 */
		return 0;
	}

	return 1;
}

TIMESTAMP group_recorder::postsync(TIMESTAMP t0, TIMESTAMP t1) {
	if (0 == write_interval) {
		if (0 == build_row(t1)) {
			gl_error("group_recorder::postsync(): error building query");
			return 0;
		}
	} else if (0 < write_interval) {
		// recalculate next_time, since we know commit() will fire
		if (last_write + write_interval <= t1) {
			interval_write = true;
			last_write = t1;
			next_write = t1 + write_interval;
		}
		return next_write;
	} else {
		// on-change intervals simply short-circuit
		return TS_NEVER;
	}

	// the interval recorders have already return'ed out, earlier in the sequence.
	return TS_NEVER;
}

int group_recorder::commit(TIMESTAMP t1) {
	// if periodic interval, check for write
	if (write_interval > 0) {
		if (interval_write) {
			if (0 == build_row(t1)) {
				gl_error("group_recorder::commit(): error building query");
				return 0;
			}
			last_write = t1;
			interval_write = false;
		}
	}

	// if every change,
	//	* compare to last values
	//	* if different, write
	if (-1 == write_interval) {
		if (0 == build_row(t1)) {
			gl_error("group_recorder::commit(): error building query");
			return 0;
		}
	}

	if (gl_globalclock == gl_globalstoptime) {
		flush_line();
		group_recorder_connection->set_tables_done();
	}

	// check if write limit
	if (limit > 0 && write_count >= limit) {
		on_limit_hit();
		group_recorder_connection->set_tables_done();
	}

	return 1;
}

int group_recorder::isa(char *classname) {
	return (strcmp(classname, oclass->name) == 0);
}

string setup_property_buffer(stringstream* property_name_buffer, const char* suffix) {
	ostringstream internal_buffer;
	internal_buffer << property_name_buffer->str() << suffix;
	return internal_buffer.str();
}

void cleanup_property_buffer(stringstream* property_name_buffer) {
	property_name_buffer->str("");
	property_name_buffer->clear();
}
/**
 @return 0 on failure, 1 on success
 **/
int group_recorder::write_header() {
	time_t now = time(NULL);
	quickobjlist *qol = 0;
	OBJECT *obj = OBJECTHDR(this);
	query_engine* grc = group_recorder_connection;
	grc->set_table_root(get_table());
	grc->init_tables(recordid_fieldname, datetime_fieldname, false);
	// check for table existence and create if not found
	if (get_property_name() != NULL) {
		stringstream property_list;

		char buffer[1024];
		strcpy(buffer, complex_part);
		vector<string> complex_part_vector = gr_split(buffer, "|");
		set complex_part_buffer = 0x00;

		for (size_t n = 0; n < complex_part_vector.size(); n++) {
			std::transform(complex_part_vector[n].begin(), complex_part_vector[n].end(), complex_part_vector[n].begin(), std::ptr_fun<
					int, int>(std::toupper));
			if (complex_part_vector[n].compare("REAL") == 0)
				complex_part_buffer += 0x01;
			else if (complex_part_vector[n].compare("IMAG") == 0)
				complex_part_buffer += 0x02;
			else if (complex_part_vector[n].compare("MAG") == 0)
				complex_part_buffer += 0x04;
			else if (complex_part_vector[n].compare("ANG") == 0)
				complex_part_buffer += 0x08;
			else if (complex_part_vector[n].compare("ANG_RAD") == 0)
				complex_part_buffer += 0x10;
		}

		for (qol = obj_list; qol != 0; qol = qol->next) {
			gld_property target_prop(qol->obj, &(qol->prop));
			if (!target_prop.is_valid())
			{
				gl_error("%s: target %s is not valid", &(qol->prop), get_name());
				/*  TROUBLESHOOT
				 Check to make sure the target you are specifying is a published variable for the object
				 that you are pointing to.  Refer to the documentation of the command flag --modhelp, or
				 check the wiki page to determine which variables can be published within the object you
				 are pointing to with the assert function.
				 */
				return 0;
			}

			stringstream property_name_buffer, property_name_local_buffer;
			string property_name_string;
			set compare = 0x01;
			set complex_part_local_buffer = complex_part_buffer;

			if (0 != target_prop.get_object()->name) {
				property_name_buffer << target_prop.get_object()->name;
//				target_prop.get_sql_safe_name()
			} else {
				property_name_buffer << target_prop.get_object()->name << ":" << target_prop.get_object()->id;
			}

			const string property_name_string_buffer = property_name_buffer.str();
			if (target_prop.is_complex()) {
				while (complex_part_local_buffer != NONE) {
					property_name_local_buffer << property_name_string_buffer;
					switch (complex_part_local_buffer & compare) {
						case NONE:
							default:
							// FIXME: this is setting and wiping the buffer each iteration. There's definitely a better way.
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
						case REAL:
							property_name_string = setup_property_buffer(&property_name_local_buffer, "_REAL");
							property_list << "`" << property_name_string << "` " << data_type << ", ";
							grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
							complex_part_local_buffer ^= compare;
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
						case IMAG:
							property_name_string = setup_property_buffer(&property_name_local_buffer, "_IMAG");
							property_list << "`" << property_name_string << "` " << data_type << ", ";
							grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
							complex_part_local_buffer ^= compare;
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
						case MAG:
							property_name_string = setup_property_buffer(&property_name_local_buffer, "_MAG");
							property_list << "`" << property_name_string << "` " << data_type << ", ";
							grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
							complex_part_local_buffer ^= compare;
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
						case ANG:
							property_name_string = setup_property_buffer(&property_name_local_buffer, "_ANG_DEG");
							property_list << "`" << property_name_string << "` " << data_type << ", ";
							grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
							complex_part_local_buffer ^= compare;
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
						case ANG_RAD:
							property_name_string = setup_property_buffer(&property_name_local_buffer, "_ANG_RAD");
							property_list << "`" << property_name_string << "` " << data_type << ", ";
							grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
							complex_part_local_buffer ^= compare;
							cleanup_property_buffer(&property_name_local_buffer);
							compare <<= 1;
							break;
					}
				}
			} else {
				// TODO: this can be cleaned up using the db->get_sql_type() method.
				property_name_local_buffer << property_name_string_buffer;
				property_name_string = setup_property_buffer(&property_name_local_buffer, "");
				if (target_prop.is_double()) {
					property_list << "`" << property_name_string << "` " << data_type << ", ";
					grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
				} else if (target_prop.is_integer()) {
					property_list << "`" << property_name_string << "` " << "BIGINT, ";
					grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
				} else if (target_prop.is_enumeration()) {
					property_list << "`" << property_name_string << "` " << "INTEGER UNSIGNED, ";
					grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
				} else if (target_prop.is_set()) {
					property_list << "`" << property_name_string << "` " << "BIGINT UNSIGNED, ";
					grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
				}
				//				else if (target_prop.is_character()) { // TODO: Identify element of gld_property we want saved in this case.
//					property_list << "`" << property_name_string << "` " << "VARCHAR(256), ";
//					grc->get_table_path()->add_table_header(grc, property_list, &property_name_string, target_prop.get_unit()->get_name());
//				}
				else {
					if (strict) {
						gl_error("group_recorder::write_header(): property is not an allowed type");
						return 0;
					}
					gl_warning("group_recorder::write_header(): property is not an allowed type");
					return 1;
				}
				cleanup_property_buffer(&property_name_local_buffer);
			}
		}

		grc->next_table(); // takes us back to first table group.

		for (int index = 0; index < grc->get_table_count(); index++) {
			// drop table if exists and drop specified
			if (db->table_exists(grc->get_table().get_string())) {
				if ((get_options() & MO_DROPTABLES) && !db->query("DROP TABLE IF EXISTS `%s`", grc->get_table().get_string()))
					exception("unable to drop table '%s'", grc->get_table().get_string());
			}

			if (db->table_exists(grc->get_table_references().get_string())) {
				if ((get_options() & MO_DROPTABLES) && !db->query("DROP TABLE IF EXISTS `%s`", grc->get_table_references().get_string()))
					exception("unable to drop table '%s'", grc->get_table_references().get_string());
			}

			// create table if not exists
			ostringstream query;
			string query_string;

			query << "CREATE TABLE IF NOT EXISTS `" << grc->get_table().get_string() << "` ("
					"`" << recordid_fieldname << "` INT AUTO_INCREMENT PRIMARY KEY, "
					"`" << datetime_fieldname << "` DATETIME, "
					"" << grc->get_table_path()->get_table_header_buffer().str() << ""
					"INDEX `i_" << datetime_fieldname << "` "
					"(`" << datetime_fieldname << "`))";
			if (!db->table_exists(grc->get_table().get_string())) {
				if (!(get_options() & MO_NOCREATE)) {
					query_string = query.str();
					const char* query_char_string = query_string.c_str();
					if (!db->query(query_char_string))
						exception("unable to create table '%s' in schema '%s'", get_table(), db->get_schema());
					else
						gl_verbose("table %s created ok", grc->get_table().get_string());
				} else
					exception("NOCREATE option prevents creation of table '%s'", grc->get_table().get_string());
			}

			// check row count
			else {
				if (db->select("SELECT count(*) FROM `%s`", grc->get_table().get_string()) == NULL)
					exception("unable to get row count of table '%s'", grc->get_table().get_string());

				gl_verbose("table '%s' ok", grc->get_table().get_string());
			}

			grc->next_table();
		}
		ostringstream query;
		string query_string;

		query << "CREATE TABLE IF NOT EXISTS `" << grc->get_table_references().get_string() << "` ("
				"`table_name` VARCHAR(256), "
				"`header` VARCHAR(256)" <<
				(print_units ? ", `units` VARCHAR(64));" : ");");
		if (!db->table_exists(grc->get_table_references().get_string())) {
			if (!(get_options() & MO_NOCREATE)) {
				query_string = query.str();
				const char* query_char_string = query_string.c_str();
				if (!db->query(query_char_string))
					exception("unable to create table '%s' in schema '%s'", grc->get_table_references().get_string(), db->get_schema());
				else
					gl_verbose("table %s created ok", grc->get_table_references().get_string());
			} else
				exception("NOCREATE option prevents creation of table '%s'", grc->get_table_references().get_string());
		}

		// check row count
		else {
			if (db->select("SELECT count(*) FROM `%s`", grc->get_table_references().get_string()) == NULL)
				exception("unable to get row count of table '%s'", grc->get_table_references().get_string());

			gl_verbose("table '%s' ok", grc->get_table_references().get_string());
		}

		grc->build_table_references(print_units);
	} else {
		exception("no property specified");
		return 0;
	}
	return 1;
}

/**
 @return 0 on failure, 1 on success
 **/

int group_recorder::build_row(TIMESTAMP t1) {
	char time_str[64];
	DATETIME dt;
	quickobjlist *curr = 0;
	char objname[128];

	query_engine* grc = group_recorder_connection;
	char buffer[1024];
	strcpy(buffer, complex_part);
	vector<string> complex_part_vector = gr_split(buffer, "|");
	set complex_part_buffer = 0x00;

	for (size_t n = 0; n < complex_part_vector.size(); n++) {
		std::transform(complex_part_vector[n].begin(), complex_part_vector[n].end(), complex_part_vector[n].begin(), std::ptr_fun<
				int, int>(std::toupper));
		if (complex_part_vector[n].compare("REAL") == 0)
			complex_part_buffer += 0x01;
		else if (complex_part_vector[n].compare("IMAG") == 0)
			complex_part_buffer += 0x02;
		else if (complex_part_vector[n].compare("MAG") == 0)
			complex_part_buffer += 0x04;
		else if (complex_part_vector[n].compare("ANG") == 0)
			complex_part_buffer += 0x08;
		else if (complex_part_vector[n].compare("ANG_RAD") == 0)
			complex_part_buffer += 0x10;
	}

	for (curr = obj_list; curr != 0; curr = curr->next) {

		gld_property target_prop(curr->obj, &(curr->prop));
		if (!target_prop.is_valid())
		{
			gl_error("%s: target %s is not valid", &(curr->prop), get_name());
			/*  TROUBLESHOOT
			 Check to make sure the target you are specifying is a published variable for the object
			 that you are pointing to.  Refer to the documentation of the command flag --modhelp, or
			 check the wiki page to determine which variables can be published within the object you
			 are pointing to with the assert function.
			 */
			return 0;
		}

		set compare = 0x01;
		set complex_part_local_buffer = complex_part_buffer;
		stringstream property_name_buffer, property_name_local_buffer;
		string property_name_string;

		if (0 != target_prop.get_object()->name) {
			property_name_buffer << target_prop.get_object()->name;
		} else {
			property_name_buffer << target_prop.get_object()->name << ":" << target_prop.get_object()->id;
		}

		if (target_prop.is_complex()) {
			complex cptr = target_prop.get_complex();
			while (complex_part_local_buffer != NONE) {
				property_name_local_buffer << property_name_buffer.str();

				switch (complex_part_local_buffer & compare) {
					case NONE:
						default:
						cleanup_property_buffer(&property_name_local_buffer);
						compare <<= 1;
						break;
					case REAL:
						property_name_string = setup_property_buffer(&property_name_local_buffer, "_REAL");
						grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(cptr.Re()));
						complex_part_local_buffer ^= compare;
						compare <<= 1;
						cleanup_property_buffer(&property_name_local_buffer);
						break;
					case IMAG:
						property_name_string = setup_property_buffer(&property_name_local_buffer, "_IMAG");
						grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(cptr.Im()));
						complex_part_local_buffer ^= compare;
						compare <<= 1;
						cleanup_property_buffer(&property_name_local_buffer);
						break;
					case MAG:
						property_name_string = setup_property_buffer(&property_name_local_buffer, "_MAG");
						grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(cptr.Mag()));
						complex_part_local_buffer ^= compare;
						compare <<= 1;
						cleanup_property_buffer(&property_name_local_buffer);
						break;
					case ANG:
						property_name_string = setup_property_buffer(&property_name_local_buffer, "_ANG_DEG");
						grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(cptr.Arg() * 180 / PI));
						complex_part_local_buffer ^= compare;
						compare <<= 1;
						cleanup_property_buffer(&property_name_local_buffer);
						break;
					case ANG_RAD:
						property_name_string = setup_property_buffer(&property_name_local_buffer, "_ANG_RAD");
						grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(cptr.Arg()));
						complex_part_local_buffer ^= compare;
						compare <<= 1;
						cleanup_property_buffer(&property_name_local_buffer);
						break;
				}
			}
		} else {
			property_name_local_buffer << property_name_buffer.str();
			property_name_string = setup_property_buffer(&property_name_local_buffer, "");
			if (target_prop.is_double()) {
				double property_value = target_prop.get_double();
				grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(property_value));
			} else if (target_prop.is_integer()) {
				int property_value = target_prop.get_integer();
				grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(property_value));
			} else if (target_prop.is_enumeration()) {
				enumeration property_value = target_prop.get_enumeration();
				grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(property_value));
			} else if (target_prop.is_set()) {
				set property_value = target_prop.get_set();
				grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(property_value));
			}
			//			else if (target_prop.is_character()) { // TODO: Identify element of gld_property we want saved in this case.
//				gld_object* property_value = target_prop.get_property();
//				grc->get_table_path()->add_insert_values(grc, &property_name_string, to_string(property_value));
//			}

			else {
				if (strict) {
					gl_error("group_recorder::build_row(): property is not an allowed type");
					return 0;
				}
				gl_warning("group_recorder::build_row(): property is not an allowed type");
				return 1;
			}
			cleanup_property_buffer(&property_name_local_buffer);
		}
	}

	for (int i = 0; i < grc->get_table_count(); i++) {
		grc->next_table();
		grc->get_table_path()->flush_value_row(&t1);
	}
	write_count++;
	return 1;
}

/**
 @return 0 on failure, 1 on success
 **/
int group_recorder::flush_line() {
	for (int i = 0; i < group_recorder_connection->get_table_count(); i++) {
		group_recorder_connection->get_table_path()->commit_state();
		group_recorder_connection->next_table();
	}
	return 1;
}

int group_recorder::on_limit_hit() {
	if (limit > 0 && write_count >= limit) {
		return flush_line();
	}
	return 0;
}

EXPORT TIMESTAMP sync_group_recorder(OBJECT *obj, TIMESTAMP t0, PASSCONFIG pass) {
	group_recorder *my = OBJECTDATA(obj, group_recorder);
	TIMESTAMP rv = 0;
	try {
		switch (pass) {
			case PC_PRETOPDOWN:
				rv = TS_NEVER;
				break;
			case PC_BOTTOMUP:
				rv = TS_NEVER;
				break;
			case PC_POSTTOPDOWN:
				rv = my->postsync(obj->clock, t0);
				obj->clock = t0;
				break;
			default:
				throw "invalid pass request";
		}
	} catch (char *msg) {
		gl_error("sync_group_recorder: %s", msg);
	} catch (const char *msg) {
		gl_error("sync_group_recorder: %s", msg);
	}
	return rv;
}

EXPORT int commit_group_recorder(OBJECT *obj) {
	int rv = 0;
	group_recorder *my = OBJECTDATA(obj, group_recorder);
	try {
		rv = my->commit(obj->clock);
	} catch (char *msg) {
		gl_error("commit_group_recorder: %s", msg);
	} catch (const char *msg) {
		gl_error("commit_group_recorder: %s", msg);
	}
	return rv;
}

template<class T> std::string group_recorder::to_string(T t) {
	std::string returnBuffer;
	std::ostringstream string_conversion_buffer;
	string_conversion_buffer.precision(15);
	string_conversion_buffer << t;
	returnBuffer = string_conversion_buffer.str();
	return returnBuffer;
}
#endif // HAVE_MYSQL
// EOF
