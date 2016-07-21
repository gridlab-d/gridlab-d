/* automatically created by ./checkpkgs */

#ifndef _GRIDLABD_VERSION_H
#define _GRIDLABD_VERSION_H

#define REV_MAJOR 4
#define REV_MINOR 0
#define REV_PATCH 0

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define _V(X,Y,Z) #X"."#Y"."#Z
#define PACKAGE "gridlabd"
#define PACKAGE_NAME "GridLAB-D"
#define PACKAGE_VERSION _V(REV_MAJOR,REV_MINOR,REV_PATCH)
#define PACKAGE_STRING PACKAGE_NAME" "PACKAGE_VERSION
#endif

const char *version_copyright(void);
unsigned int version_major(void);
unsigned int version_minor(void);
unsigned int version_patch(void);
const unsigned int version_build(void);
const char *version_branch(void);

#endif

// EOF
