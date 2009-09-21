// $Id: main.cpp 1182 2008-12-22 22:08:36Z dchassin $
//	Copyright (C) 2008 Battelle Memorial Institute


#define DLMAIN
#define MAJOR 2
#define MINOR 0

#include <stdlib.h>
#include "gridlabd.h"

EXPORT int do_kill(void*);

EXPORT int major=MAJOR, minor=MINOR;

#ifdef WIN32

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
			break;
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			do_kill(hModule);
			break;
    }
    return TRUE;
}

#else // !WIN32

CDECL int dllinit() __attribute__((constructor));
CDECL int dllkill() __attribute__((destructor));

CDECL int dllinit()
{
	return 0;
}

CDECL int dllkill() {
	do_kill(NULL);
}

#endif // !WIN32

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

