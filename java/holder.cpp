#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <complex.h>

#include "holder.h"


CLASS *oclass = 0;

CLASS *new_holder(MODULE *module){
	if(oclass == NULL){
		oclass = gl_register_class(module, "holder", 0, 0);
		if(oclass == NULL){
			gl_error("unable to register class holder");
			return 0;
		} else {
			oclass->trl = TRL_PROOF;
		}
		// no need to publish variables...
	}
	return oclass;
}


EXPORT int create_holder(OBJECT **obj, OBJECT *parent){
	int rv = 0;
	try
	{
		*obj = gl_create_object(oclass);
		if (*obj!=NULL)
		{
			rv = 1;
		}
		else
		{
			rv = 0;
		}
	}
	CREATE_CATCHALL(holder);
	return rv;
}

EXPORT TIMESTAMP sync_holder(OBJECT *obj, TIMESTAMP t0){
	return TS_NEVER;
}
// EOF
