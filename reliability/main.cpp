/** $Id: main.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@defgroup reliability Reliability analysis module
	@ingroup modules

	The reliability analysis modules implements an eventgen
	and metrics objects to permit the study of reliability
	impacts of technologies.

 **/

/* DO NOT EDIT THE NEXT LINE
MODULE:reliability
 */

#define DLMAIN

#include <stdlib.h>
#include "gridlabd.h"

/** @{ */

EXPORT int major=1;
EXPORT int minor=3;

/**@}*/

EXPORT int do_kill(void*);

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

