// $Id: main.cpp 4738 2014-07-03 00:55:39Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute


#define DLMAIN

#include <stdlib.h>
#include "gridlabd.h"


#include "tape.h"
extern "C" VARMAP varmap[];

CDECL EXPORT int setvar(char *varname, char *value)
{
	VARMAP *p;

	for (p=varmap; p->name!=NULL; p++)
	{
		if (strcmp(p->name,varname)==0)
		{
			if (p->type==VT_INTEGER)
			{
				int64 v = atoi64(value);
				if (v>=(int64)p->min && v<=(int64)p->max)
				{	*(int64*)(p->addr) = v; return 1;}
				else return 0;
			}
			else if (p->type==VT_DOUBLE)
			{
				double v = atof(value);
				if (v>=p->min && v<=p->max)
				{	*(double*)(p->addr) = v; return 1;}
				else return 0;
			}
			else if (p->type==VT_STRING)
			{
				strncpy((char*)(p->addr),value,(unsigned)p->min-1);
				return 1;
			}
		}
	}
	return 0;
}

