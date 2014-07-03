/** $Id: network.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _NETWORK_H
#define _NETWORK_H

#include <stdarg.h>
#include "gridlabd.h"

#ifdef _NETWORK_GLOBALS
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif

GLOBAL bool startedCOM INIT(false);	/*Flag to indicate if COM was initialzed, calls UnInitialization at termination */
GLOBAL OBJECT *initiatorCOM INIT(NULL);	/* Object that initiated the COM interface */

#ifdef __cplusplus

#ifdef HAVE_POWERWORLD
#ifndef PWX64
#include "pw_model.h"
#include "pw_load.h"
#include "pw_recorder.h"
#endif	//PWX64
#endif	//HAVE_POWERWORLD

#endif /* __cplusplus */

#endif
