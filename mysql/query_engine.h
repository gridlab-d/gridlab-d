/*
 * query_builder.h
 *
 *  Created on: Nov 21, 2017
 *      Author: mark.eberlein@pnnl.gov
 */
#ifdef HAVE_MYSQL

#ifndef MYSQL_QUERY_ENGINE_H_
#define MYSQL_QUERY_ENGINE_H_

#include "gridlabd.h"
#include "database.h"

#include <string>
#include <sstream>
#include <vector>
class query_engine;
class table_manager;

class query_engine : public gld_object {
	public:
		query_engine(database*, int, int);
		~query_engine();
		int query_immediate(std::string);
		inline void set_threshold(int new_threshold) {
			threshold = new_threshold;
		}
		inline void set_columns(set new_columns) {
			column_limit = new_columns;
		}
		inline database* get_database() {
			return db;
		}
		void set_table_root(char1024);
		void next_table();
		void init_tables(char32, char32, bool);
		inline void inc_table_count() {
			table_count++;
		}
		inline table_manager* get_table_path() {
			return table_path;
		}
		char1024 get_table();
		char1024 get_table_references();
		void build_table_references(bool);
		inline int get_table_count() {
			return table_count;
		}
		void set_tables_done();

	protected:
		int init(database*&);
		bool initialized;
		database *db;
		int threshold, query_count, column_limit;

	private:
		int table_count;
		char1024 table, table_references;
		table_manager* table_path;
};

class table_manager : public gld_object, public query_engine {
	public:
		table_manager(database*, int, int, int, char1024*, char32, char32, bool);
		void init_table(table_manager*);
		bool query_table(std::string*);
		inline table_manager* get_next_table() {
			return next_table;
		}
		inline char1024 get_table_name() {
			return table;
		}
		int add_table_header(query_engine*, std::stringstream&, std::string*, char*);
		int add_table_header(std::string*, std::string*);
		void flush_value_row(TIMESTAMP*);
		inline void commit_state() {
			if (!done) {
				commit_values();
			}
		}
		inline void set_done() {
			done = true;
		}
		inline std::ostringstream& get_table_header_buffer() {
			return table_header_buffer;
		}
		inline std::ostringstream& get_insert_values_buffer() {
			return insert_values_buffer;
		}
		std::vector<std::string*>* get_table_headers();
		std::vector<char*>* get_table_units();
		int add_insert_values(query_engine*, std::string*, std::string);
		void set_custom_sql(std::string sql);
		std::string get_custom_sql_columns();

	private:
		int add_insert_values(query_engine*, std::string*, std::string, int);
		void extend_list(query_engine*);
		void commit_values();
		int table_index;
		int column_count, insert_count;
		char1024 table;
		char1024* table_root;
		bool insert_values_initialized, done;
		std::ostringstream table_header_buffer, insert_values_buffer;
		std::vector<std::string*> table_headers;
		std::vector<char*> table_units;
		std::vector<std::string> insert_values;
		std::vector<std::string> custom_sql_headers, custom_sql_datatypes, custom_sql_values;
		table_manager* next_table;
		char32 recordid_fieldname, datetime_fieldname;
};

#endif /* MYSQL_QUERY_ENGINE_H_ */

#endif