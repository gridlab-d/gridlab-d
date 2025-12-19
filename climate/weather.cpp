/** $Id: weather.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file weather.cpp
	@author Matthew L Hauer
 **/

#include "weather.h"

CLASS *weather::oclass = 0;

/* mostly doing this to exploit the property system -MH */
//weather::weather(){
//	////memset(this, 0, sizeof(weather));
//}

weather::weather(MODULE *module){
	////memset(this, 0, sizeof(weather));
	if (oclass==nullptr)
	{
		oclass = gl_register_class(module,"weather",sizeof(weather), 0);
		if (oclass==nullptr)
			throw "unable to register class weather";
		else
			oclass->trl = TRL_CONCEPT;

		if (gl_publish_variable(oclass,
			PT_double,"temperature[degF]",PADDR(temperature),
			PT_double,"humidity[%]",PADDR(humidity),
			PT_double,"solar_dir[W/sf]",PADDR(solar_dir),
			PT_double,"solar_direct[W/sf]",PADDR(solar_dir),
			PT_double,"solar_diff[W/sf]",PADDR(solar_diff),
			PT_double,"solar_diffuse[W/sf]",PADDR(solar_diff),
			PT_double,"solar_global[W/sf]",PADDR(solar_global),
			PT_double,"global_horizontal_extra[W/sf]",PADDR(global_horizontal_extra),
			PT_double,"wind_speed[mph]", PADDR(wind_speed),
			PT_double,"wind_dir[deg]", PADDR(wind_dir),
			PT_double,"opq_sky_cov[pu]",PADDR(opq_sky_cov),
			PT_double,"tot_sky_cov[pu]",PADDR(tot_sky_cov),
			PT_double,"rainfall[in/h]",PADDR(rainfall),
			PT_double,"snowdepth[in]",PADDR(snowdepth),
			PT_double,"pressure[mbar]",PADDR(pressure),
			PT_int16,"month",PADDR(month),
			PT_int16,"day",PADDR(day),
			PT_int16,"hour",PADDR(hour),
			PT_int16,"minute",PADDR(minute),
			PT_int16,"second",PADDR(second),
			nullptr)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(weather));
	}
}

EXPORT int create_weather(OBJECT **obj, OBJECT *parent){
	return 1;	// don't want it to get called, but better to have it not be fatal
}

/// Synchronize the cliamte object
EXPORT TIMESTAMP sync_weather(OBJECT *obj, TIMESTAMP t0){
	return TS_NEVER; // really doesn't do anything
}


weather::unique_ptr_type create_weather() {
	return weather::unique_ptr_type(new weather());
}


// Override release method from gld_object
void weather::release()  {
	// Recursively release the linked list
	if (next) {
		next->release();
		next.reset();
	}

	// Reset all values to default
	reset();
}

// Validation method
bool weather::is_valid_weather()  {
	// Add any specific validation logic
	return temperature >= -459.67 && temperature <= 1000.0 && // Fahrenheit range
		humidity >= 0.0 && humidity <= 100.0 &&
		month >= 1 && month <= 12 &&
		day >= 1 && day <= 31 &&
		hour >= 0 && hour <= 23 &&
		minute >= 0 && minute <= 59 &&
		second >= 0 && second <= 59;
}




// Reset method to clear all data
void weather::reset() {
	temperature = 0.0;
	humidity = 0.0;
	solar_dir = 0.0;
	solar_diff = 0.0;
	solar_global = 0.0;
	global_horizontal_extra = 0.0;
	wind_speed = 0.0;
	wind_dir = 0.0;
	opq_sky_cov = 0.0;
	tot_sky_cov = 0.0;
	rainfall = 0.0;
	snowdepth = 0.0;
	pressure = 0.0;

	month = 0;
	day = 0;
	hour = 0;
	minute = 0;
	second = 0;
}




// EOF
