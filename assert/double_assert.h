/** Assert function
**/

#ifndef _double_assert_H
#define _doulbe_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class double_assert : public gld_object {
public:
	typedef enum {ONCE_FALSE=0, ONCE_TRUE=1, ONCE_DONE=2} ONCESTATUS;
	typedef enum {IN_ABS=0, IN_RATIO=1} WITHINMODE;
	typedef enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} ASSERTSTATUS;

	GL_ATOMIC(ASSERTSTATUS,status); 
	GL_STRING(char1024,target);		
	GL_ATOMIC(double,value);
	GL_ATOMIC(ONCESTATUS,once);
	GL_ATOMIC(double,once_value);
	GL_ATOMIC(WITHINMODE,within_mode);	//Assert whether the target value should be
										//within the range (True), outside of a 
										//range (False) or shouldn't be checked (None).
	GL_ATOMIC(double,within);

public:
	/* required implementations */
	double_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

public:
	static CLASS *oclass;
	static double_assert *defaults;
};
#endif
