/* $Id: recorder.h 4738 2014-07-03 00:55:39Z dchassin $
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifndef _RECORDER_H
#define _RECORDER_H

#define MO_DROPTABLES	0x0001	// drop tables option flag
#define MO_USEUNITS		0x0002	// add units to column names
#define MO_NOCREATE		0x0004	// do not automatically create tables

#include <string>
#include <vector>

#include "database.h"
#include "query_engine.h"

class recorder : public gld_object {
public:
	GL_STRING(char1024,property);
//	GL_STRING(char1024, group_def);
	GL_STRING(char32,trigger);
	GL_STRING(char1024,table);
	GL_STRING(char32,mode);
	GL_ATOMIC(int32,limit);
	GL_ATOMIC(double,interval);
	GL_ATOMIC(object,connection);
	GL_ATOMIC(set,options);
	GL_ATOMIC(char32,datetime_fieldname);
	GL_ATOMIC(char32,recordid_fieldname);
	GL_ATOMIC(char1024,header_fieldnames);
	GL_STRING(char256, complex_part);
	GL_STRING(char256, data_type);
	GL_ATOMIC(int32, query_buffer_limit);

private:
	bool enabled;
	database *db;
	query_engine* recorder_connection;
	bool trigger_on;
	char compare_op[16];
	char compare_val[32];
	size_t n_properties;
	std::vector<gld_property> property_target;
	std::vector<gld_unit> property_unit;
	char header_data[1024];
	template<class T> std::string to_string(T); // This template exists in both Recorder and Group Recorder, and could be cleaned up.
public:
	inline bool get_trigger_on(void) { return trigger_on; };
	inline bool get_enabled(void) { return enabled; };
public:
	recorder(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
public:
	static CLASS *oclass;
	static recorder *defaults;
};

#endif
