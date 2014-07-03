/** $Id: matlab.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file matlab.h
	@addtogroup matlab
 @{
 **/

#ifndef _MATLAB_H
#define _MATLAB_H

#include "globals.h"

STATUS matlab_startup(int argc, char *argv[]);

#ifdef __cplusplus
extern "C" {
#endif
	MODULE *load_matlab_module(const char *file, int argc, char *argv[]);
	/** @todo move java and python modules to their own implementation files (ticket #121) */
	MODULE *load_java_module(const char *file, int argc, char *argv[]);
	MODULE *load_python_module(const char *file, int argc, char *argv[]);
#ifdef __cplusplus
}
#endif

#endif

/**@}*/
