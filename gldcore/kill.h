/** $Id: kill.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#ifndef _KILL_H
#define _KILL_H

#if defined(_WIN32)
#include <process.h> // _getpid
// MSVC doesn’t define pid_t; MinGW does in <sys/types.h>.
#ifndef __MINGW32__
typedef int pid_t;
#else
#include <sys/types.h>
#endif
#else
#include <sys/types.h> // pid_t on POSIX
#endif

#ifdef __cplusplus
extern "C"
{
#endif

	void kill_starthandler(void);
	void kill_stophandler(void);
	int kill(pid_t pid, int sig); // Provide a Windows implementation separately

#ifdef __cplusplus
} // extern "C"
#endif

#if defined(_WIN32)
static inline void kill_starthandler(void) { /* no-op on Windows */ }
static inline void kill_stophandler(void) { /* no-op on Windows */ }
static inline int kill(pid_t /*pid*/, int /*sig*/)
{
	/* return -1 to indicate "not supported" on Windows by default */
	return -1;
}
#endif

#endif
