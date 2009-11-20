/** $Id: weather_reader.h 1182 2009-11-03 22:08:36Z mhauer $
	Copyright (C) 2009 Battelle Memorial Institute
	@file weather_reader.h
	@addtogroup climate
	@ingroup modules
 @{
 **/

#ifndef CLIMATE_WEATHER_READER_
#define CLIMATE_WEATHER_READER_

#include <stdio.h>
#include "gridlabd.h"
#include "weather.h"

/* move this to climate.h */
typedef struct s_records {
		double low;
		double low_day;
		double high;
		double high_day;
		double solar;
} RECORDS;

class weather_reader {
private:
protected:
	FILE *infile;
	weather *data_head;
	weather *data_tail;
public:
	weather_reader();
	~weather_reader();

	RECORDS record;
};

#endif

// EOF
