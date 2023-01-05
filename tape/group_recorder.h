// header stuff here

#ifndef _GROUP_RECORDER_H_
#define _GROUP_RECORDER_H_

#include "tape.h"

EXPORT void new_group_recorder(MODULE *);
EXPORT int group_recorder_postroutine(OBJECT *obj, double timedbl);

#ifdef __cplusplus
//FIXME take a look at quickobjlist. May be incorrect.
class quickobjlist{
public:
	quickobjlist(){
		obj = 0;
		next = 0;
		memset(&prop, 0, sizeof(PROPERTY));
	}
	quickobjlist(OBJECT *o, PROPERTY *p){
		obj = o;
		next = 0;
		memcpy(&prop, p, sizeof(PROPERTY));
	}
	~quickobjlist(){
		if(next != 0)
			delete next;
	}
	void tack(OBJECT *o, PROPERTY *p){if(next){next->tack(o, p);} else {next = new quickobjlist(o, p);}}
	OBJECT *obj;
	PROPERTY prop;
	quickobjlist *next;
};

class group_recorder : public gld_object {
public:
	static group_recorder *defaults;
	static CLASS *oclass, *pclass;

	explicit group_recorder(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP t1, double t1dbl, bool deltacall);

    GL_STRING(char256, filename)
    GL_STRING(char1024, group_def)
    GL_ATOMIC(bool, strict)
    GL_ATOMIC(bool, print_units)
    GL_STRING(char256, property_name)
    GL_ATOMIC(int32, limit)
    GL_ATOMIC(bool, format)
    GL_STRING(char32,mode)
    GL_STRING(char256, complex_part)
    GL_ATOMIC(double, dInterval)
    GL_ATOMIC(double, dFlush_interval)

	//Public this property, so it can access itself in scope
	TIMESTAMP write_interval;
private:
	int write_header();
	int read_line();
	int write_line(TIMESTAMP t1, double t1dbl, bool deltacall);
	int flush_line();
	int write_footer();
private:
	FILE *rec_file;
	FINDLIST *items;
	quickobjlist *obj_list;
	PROPERTY *prop_ptr;
	int obj_count;
	int write_count;
	TIMESTAMP next_write;
	TIMESTAMP last_write;
	TIMESTAMP last_flush;
	TIMESTAMP flush_interval;
	int32 write_ct;
	TAPESTATUS tape_status; // TS_INIT/OPEN/DONE/ERROR
	char *prev_line_buffer;
	char *line_buffer;
	size_t line_size;
	bool interval_write;
	bool offnominal_time;
};

#endif // C++

#endif // _GROUP_RECORDER_H_ 

// EOF
