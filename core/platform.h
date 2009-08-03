/** $Id: platform.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file platform.h
	@author David P. Chassin
	@addtogroup platform
	@ingroup core

	This header file handles platform specific macros
 @{
 **/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifdef WIN32
#if 0 /* not cooperating yet, needed for GLPATH expansion in exec_init() -mh*/
#include <Windows.h>
#endif
#define int64 __int64 /**< Win32 version of 64-bit integers */
#define FMT_INT64 "I64" /**< Win32 version of 64-bit integer printf format string */
#define atoi64 _atoi64 /**< Win32 version of 64-bit atoi */
#include <float.h>
#ifndef MINGW
#include <ymath.h>
#define INFINITY _Inf._Double
#define isnan _isnan
#define getpid _getpid
#else
#define min(A,B) ((A)<(B)?(A):(B))
#endif /* !defined MINGW */
#ifndef R_OK
#define R_OK (4)
#define W_OK (2)
#define X_OK (0)
#define F_OK (0)
#endif /* !defined R_OK */
#ifndef isfinite
#define isfinite _finite
#endif
#define strlwr _strlwr
#else /* !WIN32 */
#define int64 long long /**< standard version of 64-bit integers */
#define FMT_INT64 "ll" /**< standard version of 64-bit integer printf format string */
#define atoi64 atoll	/**< standard version of 64-bit atoi */
#define stricmp strcasecmp	/**< deprecated stricmp */
#define strnicmp strncasecmp /**< deprecated strnicmp */
#define min(A,B) ((A)<(B)?(A):(B)) /**< min macro */
#define max(A,B) ((A)>(B)?(A):(B)) /**< max macro */
#ifndef isfinite
#define isfinite finite
#endif
#endif

#ifdef I64
#define NATIVE int64	/**< native integer size */
#else
#define NATIVE int32	/**< native integer size */
#endif

#endif
/**@}**/
