/** Assert function
**/

#ifndef _enum_assert_H
#define _enum_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class enum_assert {

private:
protected:
public:
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} status; //Assert whether the target value should be
	char1024 target;											//within the range (True), outside of a 
	int32 value;											//range (False) or shouldn't be checked (None).
	//double within;

public:
	/* required implementations */
	enum_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

public:
	static CLASS *oclass;
	static enum_assert *defaults;
	complex *get_complex(OBJECT *obj, char *name);


};

#endif
