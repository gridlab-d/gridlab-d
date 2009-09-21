// main.cpp : Defines the entry point for the DLL application.
//	Copyright (C) 2008 Battelle Memorial Institute
//

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

// code for loading and initializing PLC machines
#include "machine.h"
int load_library(char *name, PLCCODE *code, PLCINIT *init, PLCDATA *data)
{
	void *hLib = LoadLibrary(name);
	if (hLib)
	{
		*data = (PLCDATA)GetProcAddress((HMODULE)hLib,"data");
		*init = (PLCINIT)GetProcAddress((HMODULE)hLib,"init");
		*code= (PLCCODE)GetProcAddress((HMODULE)hLib,"code");
		return (*data!=NULL && *init!=NULL && *code!=NULL) ? 0 : -1;
	}
	return -1;
}

#else // !WIN32
#include "machine.h"
#include <dlfcn.h>
int load_library(char *name, PLCCODE *code, PLCINIT *init, PLCDATA *data)
{
	void *hLib = dlopen(name, RTLD_LAZY);
	if (hLib)
	{
		*data = (PLCDATA)dlsym(hLib,"data");
		*init = (PLCINIT)dlsym(hLib,"init");
		*code = (PLCCODE)dlsym(hLib,"code");
		return (*data!=NULL && *init!=NULL && *code!=NULL) ? 0 : -1;
	}
	return -1;
}
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
