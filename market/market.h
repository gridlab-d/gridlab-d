/* $Id: market.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute */

#ifndef _market_H
#define _market_H

#include <stdarg.h>
#include "gridlabd.h"

typedef enum {
	BS_UNKNOWN=0,
	BS_OFF=1,
	BS_ON=2
} BIDDERSTATE;

typedef enum t_auction_special_mode {
	MD_NONE = 0,
	MD_SELLERS = 1,
	MD_BUYERS = 2,
	MD_FIXED_BUYER = 3,
	MD_FIXED_SELLER = 4,
} SPECIALMODE; // it's special!

typedef enum t_auction_verbose {
	MV_OFF = 0,
	MV_ON = 1,
} VERBOSITY;

typedef enum t_auction_clearing_type {
	CT_NULL = 0,
	CT_SELLER = 1,
	CT_BUYER = 2,
	CT_PRICE = 3,
	CT_EXACT = 4,
	CT_FAILURE = 5,
} CLEARINGTYPE;

typedef enum t_auction_stat_mode {
	ST_CURR = 0,
	ST_NEXT = 1,
	ST_PAST = 2
} STATMODE;

typedef enum t_auction_stat_type {
	SY_NONE = 0,
	SY_MEAN = 1,
	SY_STDEV = 2
} STATTYPE;

typedef int64 KEY;

/*** DO NOT DELETE THE NEXT LINE ***/
//NEWCLASS

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
