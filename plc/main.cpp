// main.cpp : Defines the entry point for the DLL application.
//	Copyright (C) 2008 Battelle Memorial Institute
//

#define DLMAIN

#include <stdlib.h>
#include "gridlabd.h"

#ifdef WIN32
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
#endif // !WIN32
