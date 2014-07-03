/** $Id: weather.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2009 Battelle Memorial Institute
	@file weather.cpp
	@author Matthew L Hauer
 **/

#include "weather.h"

CLASS *weather::oclass = 0;

/* mostly doing this to exploit the property system -MH */
weather::weather(){
	memset(this, 0, sizeof(weather));
}

weather::weather(MODULE *module){
	memset(this, 0, sizeof(weather));
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"weather",sizeof(weather),NULL);
		if (oclass==NULL)
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
			PT_double,"wind_speed[mph]", PADDR(wind_speed),
			PT_double,"rainfall[in/h]",PADDR(rainfall),
			PT_double,"snowdepth[in]",PADDR(snowdepth),
			PT_double,"pressure[mbar]",PADDR(pressure),
			PT_int16,"month",PADDR(month),
			PT_int16,"day",PADDR(day),
			PT_int16,"hour",PADDR(hour),
			PT_int16,"minute",PADDR(minute),
			PT_int16,"second",PADDR(second),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
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

// EOF
