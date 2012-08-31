/** $Id$

 PowerWorld interface recorder object.

 **/

#ifndef _PW_RECORDER_H_
#define _PW_RECORDER_H_

#include "gridlabd.h"

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

public:
	GL_ATOMIC(OBJECT *, model);
	GL_STRING(char1024, outfile_name);
	
};

#endif // _PW_RECORDER_H_