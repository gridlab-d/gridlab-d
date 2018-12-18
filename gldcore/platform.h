/** $Id: platform.h 4738 2014-07-03 00:55:39Z dchassin $
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
	#include <inttypes.h>
	#ifdef HAVE_CONFIG_H
		#include "config.h"
	#else
/*************************************
 CONTROL WHAT IS PART OF WINDOWS BUILD 
 *************************************/
		#include "config_external.h"
		//#define HAVE_XERCES
	#endif
/*** UNIX/LINUX ***/
	#ifdef __APPLE__ /* although not advised, seems reliable enough */
		#define MACOSX
		#define __WORDSIZE__ 64
		#define X64
	#else
		#define __WORDSIZE__ __WORDSIZE
	#endif
	#include <sys/unistd.h>
	#if __WORDSIZE__ == 64
		#define X64
		#define int64 long long /**< standard 64-bit integers on 64-bit machines */
	#else
		#define int64 long long /**< standard 64-bit integers on 32-bit machines */
	#endif
	#define FMT_INT64 "ll" /**< standard version of 64-bit integer printf format string */
	#define atoi64 atoll	/**< standard version of 64-bit atoi */
	#define stricmp strcasecmp	/**< deprecated stricmp */
	#define strnicmp strncasecmp /**< deprecated strnicmp */
	#define strtok_s strtok_r
	#ifndef isfinite
		#define isfinite finite
	#endif
	#ifdef __cplusplus
		template <class T> inline T min(T a, T b) { return a < b ? a : b; };
		template <class T> inline T max(T a, T b) { return a > b ? a : b; };
	#else __cplusplus
		#define min(A,B) ((A)<(B)?(A):(B)) /**< min macro */
		#define max(A,B) ((A)>(B)?(A):(B))  /**< max macro */
	#endif ! __cplusplus
	#ifdef X64
		#define NATIVE int64	/**< native integer size */
	#else
		#define NATIVE int32	/**< native integer size */
	#endif
	static int64 _qnan = 0xffffffffffffffffLL;
	#define QNAN (*(double*)&_qnan)
#endif
/**@}**/
