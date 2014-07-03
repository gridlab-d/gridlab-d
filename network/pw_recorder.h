/** $Id: pw_recorder.h 4738 2014-07-03 00:55:39Z dchassin $

 PowerWorld interface recorder object.

 **/

#ifndef _PW_RECORDER_H_
#define _PW_RECORDER_H_

#include "gridlabd.h"
#include "network.h"
#include "pw_model.h"

#ifdef HAVE_POWERWORLD
#ifndef PWX64

#ifdef int64
#undef int64
#endif

#include <WTypes.h>
#include <comutil.h>

// variation on core/platform.h!
#ifndef int64
#ifdef WIN32
#define int64 __int64
#endif
#endif


class pw_recorder : public gld_object {
public:
	pw_recorder(MODULE *module);

	static CLASS *oclass;
	static pw_recorder *defaults;
	int create();
	int init(OBJECT *parent);
	int precommit(TIMESTAMP t1);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int isa(char *classname);

	int build_keys();
	int get_pw_values();
	int GPSE();
	int write_header();
	int write_line(TIMESTAMP t1);

public:
	GL_ATOMIC(OBJECT *, model);
	GL_STRING(char1024, outfile_name);
	GL_STRING(char256, obj_classname);
	GL_STRING(char1024, key_strings);
	GL_STRING(char1024, key_values);
	GL_STRING(char1024, properties);
	GL_STRING(char1024, line_output);
	GL_ATOMIC(int64, interval);
	GL_ATOMIC(int64, limit);
private:
	
	char1024 last_line_output;
	int64 last_write;
	int write_ct;
	bool interval_write;
	pw_model *cModel;
	FILE *outfile;
	int key_count;
	int prop_count;
	char1024 key_strings_copy, key_values_copy, props_copy;
	BSTR type_bstr;
	SAFEARRAYBOUND bounds[1];
	_variant_t fields, values;
	char **out_values; // char [prop_ct][64] in practice
	bool is_ready;
};
#endif //PWX64
#endif //HAVE_POWERWORLD
#endif // _PW_RECORDER_H_