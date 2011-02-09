/** Assert function
**/

#ifndef _double_assert_H
#define _doulbe_assert_H

#include <stdarg.h>
#include "gridlabd.h"

#ifndef _isnan
#define _isnan isnan
#endif

class double_assert {

private:
protected:
public:
	enum {ONCE_FALSE=0, ONCE_TRUE=1, ONCE_DONE=2} once;
	double once_value;
	enum {ASSERT_TRUE=1, ASSERT_FALSE, ASSERT_NONE} status; //Assert whether the target value should be
	char1024 target;											//within the range (True), outside of a 
	double value;											//range (False) or shouldn't be checked (None).
	double within;

public:
	/* required implementations */
	double_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);

public:
	static CLASS *oclass;
	static double_assert *defaults;
	complex *get_complex(OBJECT *obj, char *name);


};
#endif
