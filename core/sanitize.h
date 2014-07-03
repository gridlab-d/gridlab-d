// $Id: sanitize.h 4738 2014-07-03 00:55:39Z dchassin $
// Copyright (C) 2012 Battelle Memorial Institute
// Author: DP Chassin
//
// The sanitize function is used to remove potentially proprietary
// information from GLM models so that they can be distributed without
// concern of revealing potentially sensitive system/customer information
//

#ifndef _SANITIZE_H
#define _SANITIZE_H

typedef enum {
	SO_ERASE		= 0x0001,	///< option to erase/reset to default instead of obfuscate
	SO_NAMES		= 0x0010,	///< option to sanitize names	
	SO_LATITUDE		= 0x0020,	///< option to sanitize latitude
	SO_LONGITUDE	= 0x0040,	///< option to sanitize longitude
	SO_GEOCOORDS	= 0x0060,	///< option to sanitize	lat/lon
	SO_CITY			= 0x0080,	///< option to sanitize city
	SO_TIME			= 0x0100,	///< option to sanitize times (start/stop/in/out)
	SO_DATE			= 0x0200,	///< option to sanitize date (start/stop/in/out)
	SO_TIMEZONE		= 0x0800,	///< option to sanitize timezone
	SO_SPATIAL		= 0x00f0,	///< option to sanitize all spatial info
	SO_TEMPORAL		= 0x0f00,	///< option to sanitize all temporal info
	SO_ALL			= 0x0ff0,	///< option to sanitize all info
} SANITIZEOPTIONS;

#ifdef __cplusplus
extern "C" {
#endif

int sanitize(int argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif
