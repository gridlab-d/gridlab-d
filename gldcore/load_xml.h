/* $Id: load_xml.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
 * 
 * Authors:
 *	Matthew Hauer <matthew.hauer@pnl.gov>, 6 Nov 07 -
 *
 * Versions:
 *	1.0 - MH - initial version
 *
 * Credits:
 *	adapted from SAX2Print.h
 *
 *	@file load_xml.h
 *	@addtogroup load XML file loader
 *	@ingroup core
 *
 */

#ifndef _XML_LOAD_H_
#define _XML_LOAD_H_

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "globals.h"

#ifdef __cplusplus
extern "C" {
	#include "output.h"

	STATUS loadall_xml(const char *file);
}
#else
STATUS loadall_xml(const char *file);
#endif

#endif

/* EOF */
