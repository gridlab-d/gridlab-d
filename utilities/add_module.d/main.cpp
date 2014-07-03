/** $Id: main.cpp 4738 2014-07-03 00:55:39Z dchassin $
	@file init.cpp
	@defgroup MODULENAME Module template
	@ingroups modules

	Module templates are available to help programmers
	create new modules.  In a shell, type the following
	command:
	<verbatim>
	linux% add_module NAME
	</verbatim>
	
	You must be in the source folder to this.

	You can then add a class using the add_class command.

 @{ 
 **/

/* TODO: replace the above Doxygen comments with your documentation */

/* DO NOT EDIT THE NEXT LINE 
MODULE:MODULENAME
 */

#define DLMAIN

/* TODO: set the major and minor numbers (0 is ignored) */
#define MAJOR 0
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
/**@}*/
