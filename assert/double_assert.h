/** Assert function
**/

#ifndef _double_assert_H
#define _doulbe_assert_H

#include <stdarg.h>
#include "gridlabd.h"

class double_assert {

private:
protected:
public:
	char32 target;
	double value;
	double within;

public:
	/* required implementations */
	double_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP postsync(TIMESTAMP t0, TIMESTAMP t1);


public:
	static CLASS *oclass;
	static double_assert *defaults;
	complex *get_complex(OBJECT *obj, char *name);


};
#endif