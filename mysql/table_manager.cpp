/*
 * table_manager.cpp
 *
 *  Created on: Dec 4, 2017
 *      Author: mark.eberlein@pnnl.gov
 */

#include "config.h"
#ifdef HAVE_MYSQL

#include <sstream>
#include <vector>

#include "query_engine.h"
using namespace std;

table_manager::table_manager(database* db_in,
		int threshold_in, int column_in, int table_index_in, char1024* table_name_in, char32 recordid, char32 datetime, bool single_table_mode) :
		query_engine(db_in, threshold_in, column_in) {
	char* table_temp_name = new char[1024];
	sprintf(table_temp_name, "%s_%d", table_name_in->get_string(), table_index_in);
	table_index = table_index_in;
	table_root = table_name_in;
	if (single_table_mode)
		table.copy_from(table_name_in->get_string());
	else
		table.copy_from(table_temp_name);

	recordid_fieldname = recordid;
	datetime_fieldname = datetime;

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
	table_manager* new_table = new table_manager(db, threshold, column_limit, table_index + 1, table_root, recordid_fieldname, datetime_fieldname, false);
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

// table safe column builder
int table_manager::add_table_header(query_engine* parent, stringstream& property_list, string* property_name, char* property_unit) {
	if (column_count < column_limit) {
		string* property_name_buffer = new string();
		char* property_unit_buffer = new char[64]();

		table_header_buffer << property_list.str();
		property_list.str("");
		property_list.clear();

		if (property_unit != NULL) {
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

// table unsafe column builder (do not use in multi-table systems)
int table_manager::add_table_header(string* property_name, string* property_full_header) {
	char* property_unit_buffer = new char[4]();
	strcpy(property_unit_buffer, "N/A\0");

	table_headers.push_back(property_name);
	table_units.push_back(property_unit_buffer);

	table_header_buffer << (column_count > 0 ? ", " : "") << *property_full_header;
	column_count++;

	// cleanup input variable which does not need to persist.
	delete property_full_header;

	return 1;
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

// Sadly C++ lacks a slice() or split() function, so this mess was necessary.
void table_manager::set_custom_sql(std::string sql) {
	if(sql.length() == 0) return;
	std::vector<std::string> header, datatype, value;
	std::string delimiter = ", ";
	size_t pos = 0;
	bool last_run = false;
	std::string token, inner_token;
	while ((pos = sql.find(delimiter)) != std::string::npos || !last_run) {
		last_run = (sql.find(delimiter) == std::string::npos);
		bool inner_last_run = false;
		token = sql.substr(0, pos);

		std::string inner_delimiter = " ";
		size_t inner_pos = 0;
		std::vector<std::string> buffer;

		while ((inner_pos = token.find(inner_delimiter)) != std::string::npos || !inner_last_run) {
			inner_last_run = (token.find(inner_delimiter) == std::string::npos);
			inner_token = token.substr(0, inner_pos);
			buffer.push_back(inner_token);
			token.erase(0, inner_pos + inner_delimiter.length());
		}
		header.push_back(buffer[0]);
		datatype.push_back(buffer[1]);
		value.push_back(buffer[2]);

		sql.erase(0, pos + delimiter.length());
	}

	custom_sql_headers = header;
	custom_sql_datatypes = datatype;
	custom_sql_values = value;
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
			insert_values_buffer << insert_values[value_count - 1];

			if (custom_sql_headers.size() != 0)
				for (int index = 0; index < custom_sql_values.size(); index++)
					insert_values_buffer << ", " << custom_sql_values[index];

			insert_values_buffer << ")" << std::flush;

			insert_values.clear();
			query_count++;
		}
	}
}

std::string table_manager::get_custom_sql_columns() {
	string column_values;
	for (int index = 0; index < custom_sql_headers.size(); index++)
			{
		column_values += "`" + custom_sql_headers[index] + "` " + custom_sql_datatypes[index] + ", ";
	}
	return column_values;
}

void table_manager::commit_values() {
	ostringstream query;
	string query_string, value_buffer;

	if (insert_count > 0) {
		int column_count = table_headers.size();
		query << "INSERT INTO `" << table.get_string() << "` (`" << datetime_fieldname << "`, `";

		for (int i = 0; i < column_count - 1; i++) {
			query << *table_headers[i] << "`, `";
		}
		value_buffer = insert_values_buffer.str();
		query << *table_headers[column_count - 1] << "`";

		if (custom_sql_headers.size() != 0)
			for (int index = 0; index < custom_sql_values.size(); index++)
				query << ", `" << custom_sql_headers[index] << "`";

		query << ") " << value_buffer << ";" << std::flush;
		query_string = query.str();
		if (value_buffer.length() > 0)
			db->query(query_string.c_str());

		insert_values_buffer.str("");
		insert_values_buffer.clear();
		insert_values_initialized = false;
		insert_count = 0;
	}
}

#endif // HAVE_MYSQL
