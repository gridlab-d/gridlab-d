/*
 * table_manager.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: mark.eberlein@pnnl.gov
 */

#include <sstream>
#include <vector>
#include "query_engine.h"
using namespace std;

table_manager::table_manager(database* db_in,
		int threshold_in, int column_in, int table_index_in, char1024* table_name_in) :
		query_engine(db_in, threshold_in, column_in) {
	char* table_temp_name = new char[1024];
	sprintf(table_temp_name, "%s_%d", table_name_in->get_string(), table_index_in);
	table_index = table_index_in;
	table_root = table_name_in;
	table.copy_from(table_temp_name);

	insert_values_initialized = false;

	next_table = this;
	column_count = 0;
	insert_count = 0;
	done = false;
}

std::vector<std::string*>* table_manager::get_table_headers() {
	return &table_headers;
}

std::vector<char*>* table_manager::get_table_units() {
	return &table_units;
}

void table_manager::init_table(table_manager* next_table_in) {
	next_table = next_table_in;
}

void table_manager::extend_list(query_engine* parent) {
	table_manager* new_table = new table_manager(db, threshold, column_limit, table_index + 1, table_root);
	new_table->init_table((next_table != NULL) ? next_table : this);
	next_table = new_table;
	parent->inc_table_count();
}

bool table_manager::query_table(string* column_name_in) {
	for (int i = 0; i < table_headers.size(); i++) {
		if (table_headers[i]->compare(*column_name_in) == 0) {
			return true;
		}
	}
	return false;
}

int table_manager::add_table_header(query_engine* parent, stringstream& property_list, string* property_name, char* property_unit) {
	if (column_count < column_limit) {
		string* property_name_buffer = new string();
		char* property_unit_buffer = new char[64]();

		table_header_buffer << property_list.str();
		property_list.str("");
		property_list.clear();

		if (property_unit != NULL)
		{
			strcpy(property_unit_buffer, property_unit);
		} else {
			strcpy(property_unit_buffer, "N/A");
		}

		property_name_buffer->assign(*property_name);
		table_headers.push_back(property_name_buffer);
		table_units.push_back(property_unit_buffer);

		column_count++;

		return 1; // Success
	} else {
		this->extend_list(parent);
		parent->next_table();
		return next_table->add_table_header(parent, property_list, property_name, property_unit);
	}
}

int table_manager::add_insert_values(query_engine* parent, string* column_name, string value) {
	if (query_table(column_name)) {
		insert_values.push_back(value);
		insert_count++;
	} else {
		parent->next_table();
		return parent->get_table_path()->add_insert_values(parent, column_name, value, table_index);
	}
	return 1; // Success
}

int table_manager::add_insert_values(query_engine* parent, string* column_name, string value, int start_table) {
	if (start_table == table_index)
		return 0;

	if (query_table(column_name)) {
		insert_values.push_back(value);
		insert_count++;
	} else {
		parent->next_table();
		return parent->get_table_path()->add_insert_values(parent, column_name, value, start_table);
	}
	return 1; // Success
}

void table_manager::flush_value_row(TIMESTAMP* timestamp) {
	if (!done) {
		if (query_count == threshold) {
			commit_values();
			query_count = 0;
		}

		int value_count = insert_values.size();
		if (value_count > 0) {
			if (!insert_values_initialized) {
				insert_values_buffer << "VALUES(";
				insert_values_initialized = true;
			} else {
				insert_values_buffer << ", (";
			}
			insert_values_buffer << "FROM_UNIXTIME('" << (long long) *timestamp << "'), ";
			for (int i = 0; i < value_count - 1; i++) {
				insert_values_buffer << insert_values[i] << ", ";
			}
			insert_values_buffer << insert_values[value_count - 1] << ")" << std::flush;

			insert_values.clear();
			query_count++;
		}
	}
}

void table_manager::commit_values() {
	ostringstream query;
	string query_string, value_buffer;

	if (insert_count > 0) {
		int column_count = table_headers.size();
		query << "INSERT INTO `" << table.get_string() << "` (`t`, `";

		for (int i = 0; i < column_count - 1; i++) {
			query << *table_headers[i] << "`, `";
		}
		value_buffer = insert_values_buffer.str();
		query << *table_headers[column_count - 1] << "`) " << value_buffer << ";" << std::flush;
		query_string = query.str();
		if (value_buffer.length() > 0)
			db->query(query_string.c_str());

		insert_values_buffer.str("");
		insert_values_buffer.clear();
		insert_values_initialized = false;
		insert_count = 0;
	}
}
