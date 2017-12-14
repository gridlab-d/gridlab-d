/*
 * query_builder.cpp
 *
 *  Created on: Nov 21, 2017
 *      Author: eber205
 *
 *      Provides a unified MySQL Connection interface and query constructor.
 */

#include "query_engine.h"

using namespace std;

query_engine::query_engine(database* db_in, set options_in, int threshold_in, int column_in) {
	if (!initialized) {
		if (init(db_in)) {
			gl_debug("Query Engine startup successful.");
		} else {
			gl_debug("Query Engine startup failed.");
		}
		set_threshold(threshold_in);
		set_options(options_in);
		set_columns(column_in);
		set_formatter_max(1024);
		table_count = 0;
	}
}

int query_engine::init(database* db_in) {
	initialized = true;
	query_count = 0;
	db = db_in;
	if (db == NULL) {
		exception("no database connection available or specified");
		return 0;
	}
	if (!db->isa("database")) {
		exception("connection is not a mysql database");
		return 0;
	}
	gl_verbose("connection to mysql server '%s', schema '%s' ok",
			db->get_hostname(), db->get_schema());
	return 1;
}

//void query_engine::set_query(string header_in, string footer_in) {
//	header = header_in;
//	footer = footer_in;
//}
//
//int query_engine::push_value(string value_in) {
//	if (query_count < threshold) {
//		query_buffer.append(value_in);
//		query_count++;
//	} else {
//		if (!query_engine::commit()) {
//			exception("Database commit failed.");
//			return 0;
//		}
//		query_count = 1;
//		query_buffer = value_in;
//	}
//	return 1;
//}

//int query_engine::publish() {
//	if (!query_engine::commit()) {
//		exception("Database commit failed.");
//		return 0;
//	}
//	return 1;
//}

int query_engine::query_immediate(string query_in) {
	if (db->query(query_in.c_str()))
		return 1;
	return 0;
}

//int query_engine::commit() {
//	MYSQL* mysql = db->get_handle();
//	if (header.length() != 0) {
//		query_buffer.insert(0, header.append(" "));
//	}
//	if (footer.length() != 0) {
//		query_buffer.append(footer.insert(0, " "));
//	}
//	if (query_buffer.length() != 0) {
//		db->query(query_buffer.c_str());
//		return 1;
//	}
//	return 0;
//}

void query_engine::set_formatter_max(int max_in) {
	formatter_max_characters = max_in;
}

string query_engine::format(char *fmt, ...) {
	char command[formatter_max_characters];
	va_list ptr;
	va_start(ptr, fmt);
	vsnprintf(command, formatter_max_characters, fmt, ptr);
	va_end(ptr);

	return string(command);
}

void query_engine::next_table(){
	table_path = table_path->get_next_table();
}

void query_engine::init_tables() {
	table_path = new table_manager(db, options, threshold,
			column_limit,  0, &table);
	inc_table_count();
}

void query_engine::set_table_root(char1024 table_in){
	table = table_in;
}

char1024 query_engine::get_table(){
	return table_path->get_table_name();
}

void query_engine::set_tables_done(){
	for(int i = 0; i < table_count; i++){
		table_path->set_done();
		next_table();
	}
}


