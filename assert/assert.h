/** $Id: assert.h 4738 2014-07-03 00:55:39Z dchassin $

 General purpose assert objects

 **/

#ifndef _ASSERT_H
#define _ASSERT_H

#include "gridlabd.h"

class g_assert : public gld_object {
public:
	typedef enum {AS_INIT=0, AS_TRUE=1, AS_FALSE=2, AS_NONE=3} ASSERTSTATUS;

	GL_ATOMIC(enumeration,status); 
	GL_STRING(char1024,target);		
	GL_STRING(char32,part);
	GL_ATOMIC(enumeration,relation);
	GL_STRING(char1024,value);
	GL_STRING(char1024,value2);

private:
	ASSERTSTATUS evaluate_status(void);

public:
	/* required implementations */
	g_assert(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	TIMESTAMP commit(TIMESTAMP t1, TIMESTAMP t2);
	int postnotify(PROPERTY *prop, char *value);
	inline int prenotify(PROPERTY *prop, char *value) { return 1; };

public:
	static CLASS *oclass;
	static g_assert *defaults;
};

#endif // _ASSERT_H
