/** $Id: generators.h,v 1.2 2008/02/12 00:28:08 d3g637 Exp $
	Copyright (C) 2008 Battelle Memorial Institute
 **/

#ifndef _generators_H
#define _generators_H

#include <stdarg.h>
#include "gridlabd.h"

/*** DO NOT DELETE THE NEXT LINE ***/
//NEWCLASS
#include "diesel_dg.h"
#include "windturb_dg.h"
#include "battery.h"
#include "dc_dc_converter.h"
#include "inverter.h"
#include "microturbine.h"
#include "power_electronics.h"
#include "rectifier.h"
#include "solar.h"

#define UNKNOWN 0

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
