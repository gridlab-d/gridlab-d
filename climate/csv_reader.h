/** $Id: csv_reader.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file csv_reader.h
	@addtogroup climate
	@ingroup modules
 @{
 **/

#ifndef CLIMATE_CSV_READER_
#define CLIMATE_CSV_READER_

#include "climate.h"
#include "weather.h"
#include "weather_reader.h"

/**
	@addtogroup csv CSV weather data
	@ingroup climate

	Opens a CSV files for reading and reads in the weather data.  Packs the data into
	a malloc'ed pointer array for later retrieval.
**/

class csv_reader : public weather_reader {
private:
protected:
	int read_prop(char *);
	int read_header(char *);
	int read_line(char *, int);

	int column_ct;
	PROPERTY **columns;
	TIMESTAMP last_ts; /* time on the last read line */
	weather *weather_root, *weather_last;

public:
	csv_reader();
	csv_reader(MODULE *module);
	~csv_reader();

	static CLASS *oclass;

	long int index;
	TIMESTAMP next_ts;
	weather **samples;
	long int sample_ct;

	char32 city_name;
	char32 state_name;
	char32 timezone;
	double lat_deg;
	double lat_min;
	double long_deg;
	double long_min;
	double tz_numval;
	double low_temp;
	int32 elevation;
	double high_temp;
	double peak_solar;
	char32 timefmt;
	char256 columns_str;
	char256 filename;
	typedef enum {
		CR_INIT,
		CR_OPEN,
		CR_ERROR,
	} CSVSTATUS;
	enumeration status;

	int open(const char *file);
	TIMESTAMP get_data(TIMESTAMP t0, double *temp, double *humid, double *direct, double *diffuse, double *global, double *wind, double *rain, double *snow, double *pressure);
};
#endif

// EOF
