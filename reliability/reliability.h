/* $Id: reliability.h 1182 2008-12-22 22:08:36Z dchassin $ 
	Copyright (C) 2008 Battelle Memorial Institute
*/

#ifndef _reliability_H
#define _reliability_H

#include <stdarg.h>
#include "gridlabd.h"

#ifdef _RELIABILITY_CPP
#define GLOBAL
#define INIT(A) = (A)
#else
#define GLOBAL extern
#define INIT(A)
#endif

//Module globals
GLOBAL double event_max_duration INIT(432000.0);	/**< Maximum length of any event on the system - given in seconds - defaults to 5 days*/

#endif
