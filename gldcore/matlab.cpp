/** $Id: matlab.c 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file matlab.c
	@addtogroup matlab Matlab environment
	@ingroup core
	
	The \p Matlab environment is selected by the \p --environment \p matlab command
	line argument, or by setting the \p global_environment global variable.

	When the \p Matlab environment is selected, Matlab is started and the \p gridlabd
	\e CMEX module is loaded.
 @{
 **/

#include <stdlib.h>

#include "globals.h"
#include "output.h"
#include "class.h"
#include "object.h"

STATUS matlab_startup(int argc, char *argv)
{
	if (system("matlab -r gl")==0)
		return SUCCESS;
	else
		return FAILED;
}

MODULE *load_java_module(const char *file, /**< module filename, searches \p PATH */
									  int argc, /**< count of arguments in \p argv */
									  char *argv[]) /**< arguments passed from the command line */
{
	output_error("support for Java modules is implemented somewhere else ~ please review the documentation");
	return NULL;
}

MODULE *load_python_module(const char *file, /**< module filename, searches \p PATH */
									  int argc, /**< count of arguments in \p argv */
									  char *argv[]) /**< arguments passed from the command line */
{
	/** @todo add support for Python modules (ticket #120) */
	output_error("support for Python modules is not implemented yet");
	return NULL;
}

/** @} **/
