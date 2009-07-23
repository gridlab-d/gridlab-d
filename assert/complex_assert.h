/** Assert function
**/

#ifndef _complex_assert_H
#define _complex_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class complex_assert {

private:
protected:
public:
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} status; //Assert whether the target value should be
	char32 target;											//within the range (True), outside of a 
	complex value;											//range (False) or shouldn't be checked (None).
	double within;

public:
	/* required implementations */
	complex_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);


public:
	static CLASS *oclass;
	static complex_assert *defaults;
	complex *get_complex(OBJECT *obj, char *name);


};

#endif
