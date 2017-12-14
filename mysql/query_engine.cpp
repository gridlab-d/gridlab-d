/*
 * query_builder.cpp
 *
 *  Created on: Nov 21, 2017
 *      Author: eber205
 *
 *      Provides a unified MySQL Connection interface and query constructor.
 */

#include <sstream>
#include <vector>
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

query_engine::~query_engine() {
	for (int i = 0; i < table_count; i++) {
		table_path->commit_state();
		next_table();
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

int query_engine::query_immediate(string query_in) {
	if (db->query(query_in.c_str()))
		return 1;
	return 0;
}

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

void query_engine::next_table() {
	table_path = table_path->get_next_table();
}

void query_engine::init_tables() {
	table_path = new table_manager(db, options, threshold,
			column_limit, 0, &table);
	inc_table_count();
}

void query_engine::set_table_root(char1024 table_in) {
	table = table_in;
	table_references.format("%s_%s", table_in.get_string(), "INDEX");
}

char1024 query_engine::get_table() {
	return table_path->get_table_name();
}
char1024 query_engine::get_table_references() {
	return table_references;
}

void query_engine::set_tables_done() {
	for (int i = 0; i < table_count; i++) {
		table_path->set_done();
		next_table();
	}
}

void query_engine::build_table_references() {
	stringstream query;
	vector<string*>* table_headers;

	query << "INSERT INTO `" << table_references.get_string() << "` (`table_name`, `header`) VALUES";

	for (int i = 0; i < table_count; i++) {
		table_headers = table_path->get_table_headers();
		int header_count = table_headers->size();
		char1024 table_name = table_path->get_table_name();
		for (int j = 0; j < header_count-1; j++) {
			query << "('" << table_name.get_string() << "','" << *((*table_headers)[j]) << "'),";
		}
		query << "('" << table_name.get_string() << "','" << *((*table_headers)[header_count-1]) << "')" << ((i == table_count-1) ? " " : ",");
		next_table();
	}
	query << ";";
	string query_string = query.str();
	const char* temp = query_string.c_str();
	db->query(query_string.c_str());
}

