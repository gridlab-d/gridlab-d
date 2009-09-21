/** $Id: main.cpp,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
	@file init.cpp
	@defgroup generators Generators
	@ingroup modules

 @{ 
 **/

/* TODO: replace the above Doxygen comments with your documentation */

/* DO NOT EDIT THE NEXT LINE 
MODULE:generators
 */

#define DLMAIN

/* TODO: set the major and minor numbers (0 is ignored) */
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
