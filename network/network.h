/** $Id: network.h 1197 2009-01-07 01:31:53Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _NETWORK_H
#define _NETWORK_H

#include "gridlabd.h"

#ifdef _GLOBALS_C
#ifdef __cplusplus
#error "_GLOBALS_C must be defined in a C file"
#else
#define GLOBAL_EXPORT EXPORT
#define GLOBAL
#define INIT(A) =(A);
#endif
#else
#ifdef __cplusplus
#define GLOBAL_EXPORT EXPORT
#define GLOBAL CDECL
#else
#define GLOBAL_EXPORT EXPORT extern
#define GLOBAL CDECL extern
#endif
#define INIT(A)
#endif

GLOBAL_EXPORT int major INIT(MAJOR);
GLOBAL_EXPORT int minor INIT(MINOR);

/* power flow properties */
GLOBAL double nominal_frequency_Hz INIT(60.0);

/* Gauss-Seidel solver default controls */
GLOBAL double acceleration_factor INIT(1.4);
GLOBAL double convergence_limit INIT(0.001);
GLOBAL double mvabase INIT(1.0);
GLOBAL double kvbase INIT(12.5);
GLOBAL int16 model_year INIT(2000);
GLOBAL char8 model_case INIT("S");
GLOBAL char32 model_name INIT("(unnamed)");

#ifdef _DEBUG
GLOBAL int16 debug_node INIT(0);
GLOBAL int16 debug_link INIT(0);
#endif

#ifdef __cplusplus

/* needed for initialization of CPP globals */
#ifdef DLMAIN
#undef INIT
#define INIT(A) =(A);
#endif

#include "capbank.h"
#include "fuse.h"
#include "relay.h"
#include "regulator.h"
#include "transformer.h"
#include "meter.h"
#include "generator.h"

#endif /* __cplusplus */

#endif
