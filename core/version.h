/* automatically created by ./checkpkgs */

#ifndef _GRIDLABD_VERSION_H
#define _GRIDLABD_VERSION_H

#define COPYRIGHT "Copyright (C) 2004-2013\nBattelle Memorial Institute\nAll Rights Reserved"
#define REV_MAJOR 4
#define REV_MINOR 0
#define REV_PATCH 0
#define BRANCH "trunk" 

#include "build.h"
#ifndef BUILDNUM
#pragma message("core/build.h was not updated properly - try deleting this file and rebuilding again")
#ifdef BUILD
#undef BUILD
#endif
#define BUILDNUM 0
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#else
#define _V(X,Y,Z,W) #X"."#Y"."#Z"."#W
#define PACKAGE "gridlabd"
#define PACKAGE_NAME "GridLAB-D"
#define PACKAGE_VERSION _V(REV_MAJOR,REV_MINOR,REV_PATCH,BUILDNUM)
#define PACKAGE_STRING PACKAGE_NAME" "PACKAGE_VERSION
#endif

#define _B(X) #X
#define BUILD _B(BUILDNUM)

#endif

// EOF
