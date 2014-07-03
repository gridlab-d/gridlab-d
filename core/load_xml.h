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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
	#include "output.h"

	int loadall_xml(char *file);
}
#else
int loadall_xml(char *file);
#endif

#endif

/* EOF */
