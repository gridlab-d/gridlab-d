/** $Id: test_callbacks.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _TEST_CALLBACKS_H
#define _TEST_CALLBACKS_H

#ifndef _NO_CPPUNIT

#include <stdio.h>
#include "object.h"
#include "class.h"
#include "globals.h"

/* static const PASSCONFIG passtype[] = {PC_PRETOPDOWN, PC_BOTTOMUP, PC_POSTTOPDOWN}; */

typedef struct s_test_callbacks{

	CLASS* (*get_class_by_name)(char* name);
	TIMESTAMP (*get_global_clock)(void);
	TIMESTAMP (*myobject_sync)(OBJECT *obj, TIMESTAMP ts,PASSCONFIG pass);
	STATUS (*sync_all)(PASSCONFIG pass);
	STATUS (*init_objects)(void);
	STATUS (*setup_test_ranks)(void);
	void (*remove_objects)(void);

} TEST_CALLBACKS;

CLASS* get_class_by_name(char* name);
TIMESTAMP get_global_clock(void);
TIMESTAMP myobject_sync(OBJECT *obj, TIMESTAMP ts,PASSCONFIG pass);
STATUS sync_all(PASSCONFIG pass);
STATUS init_objects(void);
STATUS setup_test_ranks(void);
void remove_objects(void);

#endif

#endif
