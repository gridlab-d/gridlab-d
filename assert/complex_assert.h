/** Assert function
**/

#ifndef _complex_assert_H
#define _complex_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class complex_assert : public gld_object {
public:
	enum {FULL=0,REAL=1,IMAGINARY=2,MAGNITUDE=3,ANGLE=4};	
	enum {ONCE_FALSE=0, ONCE_TRUE=1, ONCE_DONE=2};
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE};

	GL_ATOMIC(enumeration,status);
	GL_STRING(char1024,target);											
	GL_ATOMIC(complex,value);											
	GL_ATOMIC(enumeration,operation); 
	GL_ATOMIC(enumeration,once);				
	GL_STRUCT(complex,once_value);
	GL_ATOMIC(double,within);

public:
	/* required implementations */
	complex_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int postnotify(PROPERTY *prop, char *value);
	inline int prenotify(PROPERTY*,char*) { return 1; };

public:
	static CLASS *oclass;
	static complex_assert *defaults;
};

#endif
