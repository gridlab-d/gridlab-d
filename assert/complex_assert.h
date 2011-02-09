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
	enum {FULL=0,REAL=1,IMAGINARY=2,MAGNITUDE=3,ANGLE=4} operation; //If you want to look at only a part of 
	enum {ONCE_FALSE=0, ONCE_TRUE=1, ONCE_DONE=2} once;				//  the complex number.
	complex once_value;
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} status; //Assert whether the target value should be
	char1024 target;											//within the range (True), outside of a 
	complex value;											//range (False) or shouldn't be checked (None).
	double within;

public:
	/* required implementations */
	complex_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

public:
	static CLASS *oclass;
	static complex_assert *defaults;
	complex *get_complex(OBJECT *obj, char *name);


};

#endif
