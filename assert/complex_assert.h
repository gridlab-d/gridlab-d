/** Assert function
**/

#ifndef _complex_assert_H
#define _complex_assert_H

#include <stdarg.h>
#include "gridlabd.h"

class complex_assert {

private:
protected:
public:
	char32 target;
	complex value;
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