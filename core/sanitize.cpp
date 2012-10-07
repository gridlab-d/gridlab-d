// $Id$
// Copyright (C) 2012 Battelle Memorial Institute
// Author: DP Chassin
//

#include "gridlabd.h"
#include "output.h"
#include "sanitize.h"

extern "C" int sanitize(int argc, char *argv[])
{
	OBJECT *obj;
	char buffer[65536];
	for ( obj=object_get_first() ; obj!=NULL ; obj=object_get_next(obj) )
	{
		if ( object_save(buffer,sizeof(buffer),obj)>0 )
			output_raw("%s\n", buffer);

	}
	return 0;
}
