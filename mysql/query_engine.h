/*
 * query_builder.h
 *
 *  Created on: Nov 21, 2017
 *      Author: eber205
 */

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
	query_engine(database*, set, int, int);
	void set_query(std::string, std::string);
	int push_value(std::string);
	int publish();
	int query_immediate(std::string);
	inline void set_threshold(int new_threshold){ threshold = new_threshold; }
	inline void set_options(set new_options){ options = new_options; }
	inline void set_columns(set new_columns){ column_limit = new_columns; }
	inline database* get_database(){ return db; }
	void set_formatter_max(int);
	std::string format(char *,...);
	char1024 find_entry(char1024); // optimize this to better than linear time.
	void set_table_root(char1024);
	void next_table();
	void init_tables();
	inline void inc_table_count(){table_count++;}
	inline table_manager* get_table_path() {return table_path;}
	char1024 get_table();
	inline int get_table_count() {return table_count;}
	void set_tables_done();
protected:
	int init(database*);
	int commit();
	bool initialized;
	database *db;
//	std::string header, query_buffer, footer;
	int threshold, query_count, formatter_max_characters, column_limit;
	set options;
private:
	int table_count;
	char1024 table;
	table_manager* table_path;
};

class table_manager : public gld_object, public query_engine {
public:
	table_manager(database*, set, int, int, int, char1024*);
	void init_table(table_manager*, table_manager*);
	bool query_table(std::string*);
	inline table_manager* get_next_table(){return next_table;}
	inline table_manager* get_prev_table(){return prev_table;}
	inline char1024 get_table_name(){return table;}
	int add_table_header(query_engine*, std::stringstream&, std::string*);
	int add_insert_values(query_engine*, std::string*, double);
	void clear_values_init();
	void flush_value_row(TIMESTAMP*);
	inline void commit_state(){if(!done){commit_values();}}
	inline void set_done(){done = true;}

	inline std::ostringstream& get_table_header_buffer(){return table_header_buffer;}
	inline std::ostringstream& get_insert_values_buffer(){return insert_values_buffer;}
private:
	int add_insert_values(query_engine*, std::string*, double, int);
	void extend_list(query_engine*);
	void commit_values();
	int table_index;
	int column_count, insert_count;
	char1024 table;
	char1024* table_root;
	bool insert_values_initialized, done;
	std::ostringstream table_header_buffer, insert_values_buffer;
	std::vector<std::string*> table_headers;
	std::vector<double> insert_values;
	table_manager* next_table;
	table_manager* prev_table;
};

#endif /* MYSQL_QUERY_ENGINE_H_ */
