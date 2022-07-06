/*
 * query_builder.cpp
 *
 *  Created on: Nov 21, 2017
 *      Author: mark.eberlein@pnnl.gov
 *
 *      Provides a unified MySQL Connection interface and query constructor.
 */

#include "config.h"
#ifdef HAVE_MYSQL

#include <sstream>
#include <vector>

#include "query_engine.h"

using namespace std;

query_engine::query_engine(database* db_in, int threshold_in, int column_in) {
		if (init(db_in)) {
			gl_debug("Query Engine startup successful.");
		} else {
			gl_debug("Query Engine startup failed.");
		}
		set_threshold(threshold_in);
		set_columns(column_in);
		table_count = 0;
}

query_engine::~query_engine() {
	for (int i = 0; i < table_count; i++) {
		table_path->commit_state();
		next_table();
	}
}

int query_engine::init(database* &db_in) {
	query_count = 0;
	db = db_in;
	return 1;
}

int query_engine::query_immediate(string query_in) {
	if (db->query(query_in.c_str()))
		return 1;
	return 0;
}

void query_engine::next_table() {
	table_path = table_path->get_next_table();
}

void query_engine::init_tables(char32 recordid_fieldname, char32 datetime_fieldname, bool single_table_mode) {
	table_path = new table_manager(db, threshold, column_limit, 0, &table, recordid_fieldname, datetime_fieldname, single_table_mode);
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

void query_engine::build_table_references(bool print_units) {
	stringstream query;
	vector<string*>* table_headers;
	vector<char*>* table_units;
	int local_header_count;

	query << "INSERT INTO `" << table_references.get_string() << "` (`table_name`, `header`" <<
			(print_units ? ",`units`" : "") << ") VALUES";

	for (int i = 0; table_count > 0 && i < table_count; i++) {
		table_headers = table_path->get_table_headers();
		table_units = table_path->get_table_units();
		int header_count = table_headers->size();
		if (header_count > 0) {
			local_header_count += header_count;
			char1024 table_name = table_path->get_table_name();
			for (int j = 0; j < header_count - 1; j++) {
				query << "('" << table_name.get_string() << "','" << *((*table_headers)[j]);
				if (print_units)
					query << "', '" << (*table_units)[j];
				query << "'),";
			}
			query << "('" << table_name.get_string() << "','" << *((*table_headers)[header_count - 1]);
			if (print_units)
				query << "', '" << (*table_units)[header_count - 1];
			query << "')" << ((i == table_count - 1) ? " " : ",");
		}
		next_table();
	}
	query << ";";
	string query_string = query.str();
	if (local_header_count != 0 && table_count != 0)
		db->query(query_string.c_str());
}

#endif //HAVE_MYSQL
