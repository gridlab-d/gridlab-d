/* automatically created by ./checkpkgs */

#ifndef _GRIDLABD_VERSION_H
#define _GRIDLABD_VERSION_H

#define COPYRIGHT "Copyright (C) 2004-2012\nBattelle Memorial Institute\nAll Rights Reserved"
#define REV_MAJOR 3
#define REV_MINOR 0
#define REV_PATCH 0
#include "build.h"
#define BRANCH "Hassayampa" 

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
