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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef WIN32

/*** WINDOWS ***/
#if defined _M_X64 || defined _M_IA64
#define __WORDSIZE__ 64
#else
#define __WORDSIZE__ 32
#endif

#define int64 __int64 /**< Win32 version of 64-bit integers */
#define FMT_INT64 "I64" /**< Win32 version of 64-bit integer printf format string */
//#define FMT_INT64 "ll" /**< standard version of 64-bit integer printf format string */
#define atoi64 _atoi64 /**< Win32 version of 64-bit atoi */
#define strtoll _strtoi64 /**< Win32 version of POSIX strtoll function */
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
#define X_OK (0) /* windows does not recognize X */
#define F_OK (0)
#endif /* !defined R_OK */
#ifndef isfinite
#define isfinite _finite
#endif
#define strlwr _strlwr
typedef int pid_t;

#else /* !WIN32 */

/*** UNIX/LINUX ***/
#ifdef __APPLE__ /* although not advised, seems reliable enough */
#define MACOSX
#endif

#include <sys/unistd.h>

#if __WORDSIZE__ == 64
#define int64 long /**< standard 64-bit integers on 64-bit machines */
#else
#define int64 long long /**< standard 64-bit integers on 32-bit machines */
#endif
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

static int64 _qnan = 0xffffffffffffffffLL;
#define QNAN (*(double*)&_qnan)

#endif
/**@}**/
