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
	typedef enum {FULL=0,REAL=1,IMAGINARY=2,MAGNITUDE=3,ANGLE=4} OPERATION;	//If you want to look at only a part of 
																			//  the complex number.
	typedef enum {ONCE_FALSE=0, ONCE_TRUE=1, ONCE_DONE=2} ONCEMODE;
	typedef enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} ASSERTSTATUS;	//Assert whether the target value should be
																			//within the range (True), outside of a 
																			//range (False) or shouldn't be checked (None).
	GL_ATOMIC(ASSERTSTATUS,status);
	GL_STRING(char1024,target);											
	GL_ATOMIC(complex,value);											
	GL_ATOMIC(OPERATION,operation); 
	GL_ATOMIC(ONCEMODE,once);				
	GL_STRUCT(complex,once_value);
	GL_ATOMIC(double,within);

public:
	/* required implementations */
	complex_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

public:
	static CLASS *oclass;
	static complex_assert *defaults;
};

#endif
