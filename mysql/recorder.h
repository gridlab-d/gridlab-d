/* $Id: recorder.h 4738 2014-07-03 00:55:39Z dchassin $
 * DP Chassin - July 2012
 * Copyright (C) 2012 Battelle Memorial Institute
 */

#ifdef HAVE_MYSQL

#ifndef _RECORDER_H
#define _RECORDER_H

#define MO_DROPTABLES	0x0001	// drop tables option flag
#define MO_USEUNITS		0x0002	// add units to column names
#define MO_NOCREATE		0x0004	// do not automatically create tables

#include <string>
#include <vector>

#include "database.h"
#include "query_engine.h"


class recorder_quickobjlist {
	public:
		recorder_quickobjlist() {
			obj = 0;
			next = 0;
			memset(&prop, 0, sizeof(PROPERTYSTRUCT));
		}
		recorder_quickobjlist(OBJECT *o, PROPERTYSTRUCT *p) {
			obj = o;
			next = 0;
			memcpy(&prop, p, sizeof(PROPERTYSTRUCT));
		}
		~recorder_quickobjlist() {
			if (next != 0)
				delete next;
		}
		void tack(OBJECT *o, PROPERTYSTRUCT *p) {
			if (next) {
				next->tack(o, p);
			} else {
				next = new recorder_quickobjlist(o, p);
			}
		}
		OBJECT *obj;
		PROPERTYSTRUCT prop;
		recorder_quickobjlist *next;
};


class recorder : public gld_object {
public:
	GL_STRING(char1024,property);
	GL_STRING(char32,trigger);
	GL_STRING(char1024,table);
	GL_STRING(char1024,group);
	GL_STRING(char1024,recorder_name);
	GL_STRING(char32,mode);
	GL_ATOMIC(int32,limit);
	GL_ATOMIC(double,interval);
	GL_ATOMIC(object,connection);
	GL_ATOMIC(set,options); // this options set is generated based on the mode.
	GL_ATOMIC(set,sql_options); // this is the actual flag set the user can set with the 'options' input.
	GL_ATOMIC(char32,datetime_fieldname);
	GL_ATOMIC(char32,recordid_fieldname);
	GL_ATOMIC(char1024,header_fieldnames);
	GL_STRING(char256, data_type);
	GL_ATOMIC(int32, query_buffer_limit);
	GL_ATOMIC(bool, minified);
	GL_STRING(char1024,custom_sql);

private:
	bool enabled;
	bool group_mode;
	bool tag_mode;
	database *db;
	query_engine* recorder_connection;
	bool trigger_on;
	char compare_op[16];
	char compare_val[32];
	size_t n_properties;
	std::vector<gld_property> property_target;
	std::vector<gld_unit> property_unit;
	std::vector<std::string> property_specs;
	char header_data[1024];
	recorder_quickobjlist *group_obj_list;
	gld_objlist* group_items;
	int group_obj_count;
	int group_limit_counter;
	template<class T> std::string to_string(T); // This template exists in both Recorder and Group Recorder, and could be cleaned up.
public:
	inline bool get_trigger_on(void) { return trigger_on; };
	inline bool get_enabled(void) { return enabled; };
public:
	recorder(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t0, TIMESTAMP t1);
	STATUS finalize();
public:
	static CLASS *oclass;
	static recorder *defaults;
};

#endif

#endif