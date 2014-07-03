/** Assert function
**/

#ifndef _enum_assert_H
#define _enum_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class enum_assert : public gld_object {
public:
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE}; 
	
	GL_ATOMIC(enumeration,status);
	GL_STRING(char1024,target);
	GL_ATOMIC(int32,value);

public:
	/* required implementations */
	enum_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);

public:
	static CLASS *oclass;
	static enum_assert *defaults;
};

#endif
