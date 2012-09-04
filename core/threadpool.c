/*  $Id: threadpool.c 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 *
 *  Threadpool implementation.
 *
 *  Author: Brandon Carpenter <brandon.carpenter@pnl.gov>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#define _WIN32_WINNT 0x0400
#include <windows.h>
#endif

#include "threadpool.h"

#ifdef HAVE_GET_NPROCS
#include <sys/sysinfo.h>
int processor_count(void) { return get_nprocs(); }
#elif defined(__MACH__)
#include <sys/param.h>
#include <sys/sysctl.h>
int processor_count(void)
{
	int count;
	size_t size = sizeof(count);
	if (sysctlbyname("hw.ncpu", &count, &size, NULL, 0))
		return 1;
	return count;
}
#else
int processor_count(void)
{
#ifdef WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
	char *proc_count = getenv("NUMBER_OF_PROCESSORS");
	int count = proc_count ? atoi(proc_count) : 0;
	return count ? count : 1;
#endif /* WIN32 */
}
#endif /* HAVE_GET_NPROCS */

static MTICODE iterator_proc(MTI *arg)
{
	/* TODO */
	return NULL;
}

MTI *mti_iterator_init(MTIGETFN get, MTICALLFN call, MTIRETFN ret)
{
	/* TODO */
	return NULL;
}

MTIDATA mti_iterator_run(MTI *iterator, MTIDATA data)
{
	/* TODO */
	return NULL;
}
