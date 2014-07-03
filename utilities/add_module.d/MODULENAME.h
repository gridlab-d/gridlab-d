/* $Id: MODULENAME.h 4738 2014-07-03 00:55:39Z dchassin $ */

#ifndef _MODULENAME_H
#define _MODULENAME_H

#include <stdarg.h>
#include "gridlabd.h"

/*** DO NOT DELETE THE NEXT LINE ***/
//NEWCLASSINC

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
