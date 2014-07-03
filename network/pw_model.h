/** $Id: pw_model.h 4738 2014-07-03 00:55:39Z dchassin $

 PowerWorld model interface object.  

 **/

#ifndef _PW_MODEL_H_
#define _PW_MODEL_H_

#include "network.h"

#ifdef HAVE_POWERWORLD
#ifndef PWX64

// something in the import'ed stuff includes wtypes.h, which uses int64 as a member name
#ifdef int64
#undef int64
#endif
#import "libid:C99F1760-277E-11D5-A106-00C04F469176"
#ifndef int64
#define int64 __int64
#endif

using namespace pwrworld;

class pw_model : public gld_object {
public:
	pw_model(MODULE *module);

	static CLASS *oclass;
	static pw_model *defaults;
	int create();
	int init(OBJECT *parent);
	TIMESTAMP presync(TIMESTAMP t1);
	TIMESTAMP sync(TIMESTAMP t1);
	TIMESTAMP postsync(TIMESTAMP t1);
	int isa(char *classname);
	int finalize();
	void pw_close_COM(void);
public:
	GL_STRING(char1024, model_name);
	GL_STRING(char1024, load_list_file);
	GL_STRING(char1024, out_file);
	GL_STRING(char32, out_file_type);
	GL_STRING(char1024, field_file);
	GL_STRING(char32, field_type);
	GL_ATOMIC(bool, update_flag);
	GL_ATOMIC(int32, exchange_count);
	//GL_ATOMIC(bool, model_invalid_state);
	GL_ATOMIC(bool, valid_flag);
public:
	ISimulatorAutoPtr A;
	CLSID clsid;
	HRESULT hr;

};

#endif // PWX64
#endif // HAVE_POWERWORLD
#endif // _PW_MODEL_H_

// EOF
