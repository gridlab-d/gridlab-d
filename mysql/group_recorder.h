#ifdef HAVE_MYSQL

#ifndef _GROUP_RECORDER_H
#define _GROUP_RECORDER_H

#include "database.h"
#include "query_engine.h"
EXPORT void new_group_recorder(MODULE *);

class query_engine;

class quickobjlist {
	public:
		quickobjlist() {
			obj = 0;
			next = 0;
			memset(&prop, 0, sizeof(PROPERTY));
		}
		quickobjlist(OBJECT *o, PROPERTY *p) {
			obj = o;
			next = 0;
			memcpy(&prop, p, sizeof(PROPERTY));
		}
		~quickobjlist() {
			if (next != 0)
				delete next;
		}
		void tack(OBJECT *o, PROPERTY *p) {
			if (next) {
				next->tack(o, p);
			} else {
				next = new quickobjlist(o, p);
			}
		}
		OBJECT *obj;
		PROPERTY prop;
		quickobjlist *next;
};

class group_recorder : public gld_object {
	public:
		static group_recorder *defaults;
		static CLASS *oclass, *pclass;

		group_recorder(MODULE *);
		~group_recorder();
		int create();
		int init(OBJECT *);
		int isa(char *);
		TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);
		int commit(TIMESTAMP);
		std::vector<std::string> gr_split(char*, const char*);

		GL_STRING(char1024,table)
		GL_STRING(char1024, group_def)
		GL_ATOMIC(bool, strict)
		GL_ATOMIC(bool, print_units)
		GL_ATOMIC(bool, legacy_mode)
		GL_STRING(char256, property_name)
		GL_ATOMIC(int32, limit)
		GL_ATOMIC(bool, format)
		GL_ATOMIC(object,connection)
		GL_ATOMIC(set,options)
		GL_STRING(char32,mode)
		GL_ATOMIC(int32, query_buffer_limit)
//		GL_ATOMIC(int32, formatter_limit)
		GL_STRING(char32,datetime_fieldname)
		GL_STRING(char32,recordid_fieldname)
		GL_STRING(char1024,header_fieldnames)
		GL_STRING(char256, complex_part)
		GL_STRING(char256, data_type)
		GL_ATOMIC(int32, column_limit)
		GL_ATOMIC(double, dInterval)
		GL_ATOMIC(double, dFlush_interval)

	private:
		int write_header();
		int build_row(TIMESTAMP);
		int flush_line();
		int on_limit_hit();
		template<class T> std::string to_string(T); // This template exists in both Recorder and Group Recorder, and could be cleaned up.

		MYSQL *mysql;
		database *db;
		query_engine* group_recorder_connection;
		FILE *rec_file;
		FINDLIST *items;
		quickobjlist *obj_list;
		PROPERTY *prop_ptr;
		int obj_count;
		int write_count;
		TIMESTAMP next_write;
		TIMESTAMP last_write;
		TIMESTAMP last_flush;
		TIMESTAMP write_interval;
		TIMESTAMP flush_interval;
		int32 write_ct;
		char *prev_line_buffer;
		char *line_buffer;
		size_t line_size;
		bool interval_write;
};

#endif // _GROUP_RECORDER_H
// EOF

#endif