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
	#define __STDC_FORMAT_MACROS
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
    	#undef int64
		#define int64 long long /**< standard 64-bit integers on 32-bit machines */
	#endif
	#define FMT_INT64 "ll" /**< standard version of 64-bit integer printf format string */
	#define atoi64 atoll	/**< standard version of 64-bit atoi */
	#define stricmp strcasecmp	/**< deprecated stricmp */
	#define strnicmp strncasecmp /**< deprecated strnicmp */
	#define strtok_s strtok_r
	#ifdef X64
		#define NATIVE int64	/**< native integer size */
	#else
		#define NATIVE int32	/**< native integer size */
	#endif
	static int64 _qnan = 0xffffffffffffffffLL;
	#define QNAN (*(double*)&_qnan)
#endif
/**@}**/
