/** $Id: weather.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file weather.h
	@addtogroup climate
	@ingroup modules
 @{
 **/

#ifndef CLIMATE_WEATHER_
#define CLIMATE_WEATHER_

#include "gridlabd.h"

class weather {
public:
	static CLASS *oclass;
	static weather *defaults;

	weather();
	weather(MODULE *module);
	int create();
	void set_defaults();
	void get_defaults();
	TIMESTAMP sync(TIMESTAMP t0);

	double temperature; // F
	double humidity;
	double solar_dir;
	double solar_diff;
	double solar_global;
  	double global_horizontal_extra;
	double wind_speed;
	double wind_dir;
	double opq_sky_cov;
	double tot_sky_cov;
	double rainfall;
	double snowdepth;
	double pressure;
	int16 month, day, hour, minute, second;
	weather *next;
};

#endif

// EOF
