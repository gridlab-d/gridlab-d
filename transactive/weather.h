// $Id: weather.h 5149 2015-04-18 22:04:13Z dchassin $

#ifndef _WEATHER_H
#define _WEATHER_H

#define AC_NONE 0x0000
#define AC_INTERPOLATE 0x0001 // interpolate subsample data using first-order-hold
#define AC_SOLARCALCS 0x0002 // automatically calculate solar values (requires lat/lon be set)
#define AC_WINDCALCS 0x0004 // synthesize subhourly wind variations based on speed and gusts values (very experimental)

// solar defaults
#define SOLAR_GSC 1367.0
#define SH_NONE     0 // no haze
#define SH_HOTTEL23 1 // 23km Hottel haze model
#define SH_HOTTEL5  2 // 5km Hottel haze model

inline double sign(double x) { return (x<0?-1:(x>0?+1:0)); };

class weather : public gld_object {
public:
	GL_ATOMIC(double,drybulb);
	GL_ATOMIC(double,wetbulb);
	GL_ATOMIC(double,humidity);
	GL_ATOMIC(double,clouds);
	GL_ATOMIC(double,solar_direct);
	GL_ATOMIC(double,solar_azimuth);
	GL_ATOMIC(double,solar_altitude);
	GL_ATOMIC(double,solar_horizontal);
	GL_ATOMIC(double,wind_heading);
	GL_ATOMIC(double,wind_speed);
	GL_ATOMIC(double,wind_gusts);
	GL_BITFLAGS(set,options);
	GL_ATOMIC(double,solar_constant);
	GL_ATOMIC(enumeration,haze_model);
	GL_ATOMIC(double,elevation);
private:
	// TODO internal variables go here
	double avg_wind_heading;
	double avg_wind_speed;
	double avg_wind_gusts;
private:
	void interpolate(TIMESTAMP ts);
	void solarcalcs(TIMESTAMP ts);
	void windcalcs(TIMESTAMP ts);
public:
	DECL_IMPLEMENT(weather);
	DECL_PRECOMMIT;
	DECL_NOTIFY(wind_heading);
	DECL_NOTIFY(wind_speed);
	DECL_NOTIFY(wind_gusts);
	int kmldump(int (*stream)(const char*, ...));
};

#endif // _WEATHER_H
