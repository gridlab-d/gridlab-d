/* $Id
 * Version information
 */

#define _VERSION_C // flag special consideration for this file

#include "version.h"
#include <time.h>

#include "build.h" // BRANCH will be defined automatically from the ticket

//#ifndef BRANCH
#define BRANCH "Keeler" // update this from legal.h each time trunk is branched
//#endif

#ifndef BUILDNUM
#error("gldcore/build.h was not updated properly (BUILDNUM is missing) - try deleting it and rebuilding again")
#ifdef BUILD
#undef BUILD
#endif
#define BUILDNUM 0
#endif

#ifndef REV_YEAR
#error("gldcore/build.h was not updated properly (REV_YEAR is missing) - try deleting it and rebuilding again")
#endif

const char *version_copyright(void)
{
	static char buffer[1024];
	sprintf(buffer,"Copyright (C) 2004-%d\nBattelle Memorial Institute\nAll Rights Reserved", REV_YEAR);
	return buffer;
}

unsigned int version_major(void)
{
	return REV_MAJOR;
}
unsigned int version_minor(void)
{
	return REV_MINOR;
}
unsigned int version_patch(void)
{
	return REV_PATCH;
}
const unsigned int version_build(void)
{
	return BUILDNUM;
}

const char *version_branch(void)
{
	return BRANCH;
}

