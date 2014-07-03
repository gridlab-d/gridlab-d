/** $Id: network.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file network.cpp
	@addtogroup network Transmission flow solver (network)
	@ingroup modules

 @{
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
//#include "network.h"

#define _NETWORK_GLOBALS
#include "network.h"
#undef  _NETWORK_GLOBALS

EXPORT CLASS *init(CALLBACKS *fntable, MODULE *module, int argc, char *argv[])
{
#ifdef _DEBUG
	//cmdargs(argc,argv);
#endif // _DEBUG

	if (set_callback(fntable)==NULL)
	{
		errno = EINVAL;
		return NULL;
	}

#ifdef HAVE_POWERWORLD
#ifndef PWX64
	int status = gl_global_setvar("run_powerworld=true");
	CLASS *first = (new pw_model(module))->oclass;
	new pw_load(module);
	new pw_recorder(module);
	/* always return the first class registered */
#else
	CLASS *first = NULL;
#endif
#else
	CLASS *first = NULL;
#endif
	return first;

}

//Module termination function
//Used to handle COM uninitialization, if it was called
//Better than "finalize" for pw_model since this gets executed on errors as well
EXPORT void term(void)
{
#ifdef HAVE_POWERWORLD
#ifndef PWX64
	pw_model *temp_model;

	if (startedCOM)
	{
		//Get model information
		temp_model = OBJECTDATA(initiatorCOM,pw_model);

		//Call the COM UnInitialize routine
		temp_model->pw_close_COM();

		gl_verbose("network: closed out COM connection");
	}
#endif	//PWX64
#endif	//HAVE_POWERWORLD
}

CDECL int do_kill()
{
	/* if global memory needs to be released, this is a good time to do it */
	return 0;
}

/**@}*/
