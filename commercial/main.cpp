/** $Id: main.cpp,v 1.5 2008/02/14 23:26:25 d3p988 Exp $
	@file main.cpp
	@defgroup commercial Commercial buildings (commercial)
	@ingroup modules

	This module implements commercial building models.  Currently
	on a single-zone office is supported.  Plans call for multi-zone
	office, and a number of other significant commercial types such
	as schools, stores, and refrigerated warehouses.
 @{
 **/

#define DLMAIN
#define MAJOR 1
#define MINOR 3

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

/**@}*/
