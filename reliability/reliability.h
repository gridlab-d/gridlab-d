/* $Id: reliability.h 1182 2008-12-22 22:08:36Z dchassin $ 
	Copyright (C) 2008 Battelle Memorial Institute
*/

#ifndef _reliability_H
#define _reliability_H

#include <stdarg.h>
#include "gridlabd.h"

/*** DO NOT DELETE THE NEXT LINE ***/
//NEWCLASS
#include "metrics.h"
#include "eventgen.h"

/* optional exports */
#ifdef OPTIONAL

/* TODO: define this function to enable checks routine */
EXPORT int check(void);

/* TODO: define this function to allow direct import of models */
EXPORT int import_file(char *filename);

/* TODO: define this function to allow direct export of models */
EXPORT int export_file(char *filename);

/* TODO: define this function to allow export of KML data for a single object */
EXPORT int kmldump(FILE *fp, OBJECT *obj);
#endif

#endif
