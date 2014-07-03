// header stuff here

#ifndef _GROUP_RECORDER_H_
#define _GROUP_RECORDER_H_

#include "tape.h"

EXPORT void new_group_recorder(MODULE *);

#ifdef __cplusplus

class quickobjlist{
public:
	quickobjlist(){obj = 0; next = 0; memset(&prop, 0, sizeof(PROPERTY));}
	quickobjlist(OBJECT *o, PROPERTY *p){obj = o; next = 0; memcpy(&prop, p, sizeof(PROPERTY));}
	~quickobjlist(){if(next != 0) delete next;}
	void tack(OBJECT *o, PROPERTY *p){if(next){next->tack(o, p);} else {next = new quickobjlist(o, p);}}
	OBJECT *obj;
	PROPERTY prop;
	quickobjlist *next;
};

class group_recorder{
public:
	static group_recorder *defaults;
	static CLASS *oclass, *pclass;

	group_recorder(MODULE *);
	int create();
	int init(OBJECT *);
	int isa(char *);
	TIMESTAMP postsync(TIMESTAMP, TIMESTAMP);

	int commit(TIMESTAMP);
public:
	char1024 group_def;
	double dInterval;
	double dFlush_interval;
	char256 property_name;
	int32 limit;
	char256 filename;
	bool strict;
	bool print_units;
	CPLPT complex_part;
private:
	int write_header();
	int read_line();
	int write_line(TIMESTAMP);
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
	TIMESTAMP write_interval;
	TIMESTAMP flush_interval;
	int32 write_ct;
	TAPESTATUS tape_status; // TS_INIT/OPEN/DONE/ERROR
	char *prev_line_buffer;
	char *line_buffer;
	size_t line_size;
	bool interval_write;
};

#endif // C++

#endif // _GROUP_RECORDER_H_ 

// EOF
