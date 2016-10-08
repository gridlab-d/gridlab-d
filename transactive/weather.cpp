// $Id: weather.cpp 5149 2015-04-18 22:04:13Z dchassin $

#include "transactive.h"

EXPORT_IMPLEMENT(weather)
EXPORT_PRECOMMIT(weather)
EXPORT_NOTIFY_PROP(weather,wind_heading);
EXPORT_NOTIFY_PROP(weather,wind_speed);
EXPORT_NOTIFY_PROP(weather,wind_gusts);

weather::weather(MODULE *module)
{
	oclass = gld_class::create(module,"weather",sizeof(weather),PC_AUTOLOCK);
	oclass->trl = TRL_CONCEPT;
	defaults = this;
	if ( gl_publish_variable(oclass,
		PT_double,"drybulb[degC]",get_drybulb_offset(),PT_DESCRIPTION,"drybulb air temperature",
		PT_double,"webbulb[degC]",get_wetbulb_offset(),PT_DESCRIPTION,"wetbulb air temperature",
		PT_double,"humidity[pu]",get_humidity_offset(),PT_DESCRIPTION,"relative humidity",
		PT_double,"clouds[pu]",get_clouds_offset(),PT_DESCRIPTION,"cloudiness factor",
		PT_double,"solar_direct[W/m^2]",get_solar_direct_offset(),PT_DESCRIPTION,"direct solar radiation",
		PT_double,"solar_azimuth[deg]",get_solar_azimuth_offset(),PT_DESCRIPTION,"solar azimuth angle",
		PT_double,"solar_altitude[deg]",get_solar_altitude_offset(),PT_DESCRIPTION,"solar altitude angle",
		PT_double,"solar_horizontal[W/m^2]",get_solar_horizontal_offset(),PT_DESCRIPTION,"horizontal solar radiation",
		PT_double,"wind_heading[deg]",get_wind_heading_offset(),PT_DESCRIPTION,"wind heading",
		PT_double,"wind_speed[m/s]",get_wind_speed_offset(),PT_DESCRIPTION,"wind speed",
		PT_double,"wind_gusts[m/s]",get_wind_gusts_offset(),PT_DESCRIPTION,"wind gusts",
		PT_set,"options",get_options_offset(),PT_DESCRIPTION,"options for how values are updated",
			PT_KEYWORD,"NONE",AC_NONE,
			PT_KEYWORD,"SOLARCALCS",AC_SOLARCALCS,
			PT_KEYWORD,"INTERPOLATE",AC_INTERPOLATE,
			PT_KEYWORD,"WINDCALCS",AC_WINDCALCS,
		PT_double,"solar_constant[W/m^2]",get_solar_constant_offset(),PT_DESCRIPTION,"solar constant",
		PT_enumeration,"haze_model",get_haze_model_offset(),PT_DESCRIPTION,"solar haze model",
			PT_KEYWORD,"NONE",SH_NONE,
			PT_KEYWORD,"LOW", SH_HOTTEL23,
			PT_KEYWORD,"HIGH", SH_HOTTEL5,
		PT_double,"elevation[m]",get_elevation_offset(),PT_DESCRIPTION,"elevation of weather station",
		NULL)<1 )
		exception("unable to publish transactive weather properties");
	memset(defaults,0,sizeof(weather));
}

int weather::create(void)
{
	memcpy(this,defaults,sizeof(*this));
	solar_constant = SOLAR_GSC;
	haze_model = SH_NONE;
	return 1;
}

int weather::init(OBJECT *parent)
{
	if ( get_options(AC_SOLARCALCS) )
	{
		if ( !isfinite(my()->latitude) || !isfinite(my()->longitude) )
			exception("cannot use SOLARCALCS option without specifying latitude and longitude");
		if ( my()->latitude<0 )
			exception("southern latitudes not supported yet");
	}
	interpolate(gl_globalclock);
	solarcalcs(gl_globalclock);
	windcalcs(gl_globalclock);
	return 1;
}

int weather::isa(const char *type)
{
	return strcmp(type,"weather")==0;
}

int weather::precommit(TIMESTAMP t1)
{
	interpolate(t1);
	solarcalcs(t1);
	windcalcs(t1);
	return 1;
}

void weather::solarcalcs(TIMESTAMP ts)
{
	if ( get_options(AC_SOLARCALCS) )
	{
		gld_clock dt(ts);
		// solar calcs (see Vanek 2008)
		int timezone = dt.get_tzoffset()/3600;
		int dst_offset = dt.get_is_dst() ? 1 : 0;
		double latitude = my()->latitude;
		double longitude = my()->longitude;
		double N = dt.get_yearday();
		double dh = dt.get_hour()+(dt.get_minute()+dt.get_second()/60.0)/60.0;
		double B = (N-81)*2*PI/365.25;
		double ET = 9.87*sin(2*B)-7.53*cos(B)-1.5*sin(B);
		double ST = dh + sign(longitude)*4*(timezone*15-longitude) + ET/60 - dst_offset;
		double h = (ST-12)*PI/12;
		double delta = 2*PI*23.45/360*sin(2*PI*(284+N)/365.25);
		double L = latitude*PI/180;
		double alpha = asin(sin(L)*sin(delta)+cos(L)*cos(delta)*cos(h));
		if ( alpha>0 ) // is daytime
		{
			double phi = PI/2-alpha;
			double z = asin(cos(delta)*sin(h)/cos(alpha));
			if ( cos(h)<=tan(delta)/tan(L) )
			{
				if ( h<0 )
					z = abs(z)-PI;
				else
					z = PI-z;
			}
			double hss = acos(-tan(L)*tan(delta));
			double hsr = -hss;
			// extraterrestrial radiation
			double Gon = SOLAR_GSC*(1+0.033*cos(N*2*PI/365.25));
			double Goh = Gon*cos(phi);
			// atmospheric transmissivity
			double cospsi = cos(alpha*PI/2);
			#define Re 6372
			#define Ha 152.4
			double am = sqrt(1+2*Re/Ha+(Re/Ha*Re/Ha)*cospsi*cospsi) + Re/Ha*cospsi;
			double tau;
			if ( haze_model==SH_NONE )
				tau = 0.5*(exp(-0.65*am)+exp(-0.095*am));
			else
			{
				size_t hm = haze_model-1;
				if ( hm<0 ) exception("negative haze model number");
				if ( hm>1 ) exception("only 2 Hottel haze models supported");
				static const double alt[6] = {0, 500, 1000, 1500, 2000, 2500};
				static const double a0[2][6] = {
					{ 0.1283, 0.1742, 0.2195, 0.2582, 0.2915, 0.3200},
					{ 0.0270, 0.0630, 0.0964, 0.1260, 0.1530, 0.1770}};
				static const double a1[2][6] = {
					{ 0.7559, 0.7214, 0.6848, 0.6532, 0.6265, 0.6020},
					{ 0.8101, 0.8040, 0.7978, 0.7930, 0.7880, 0.7840}};
				static const double k[2][6] = {
					{ 0.3878, 0.3236, 0.3139, 0.2910, 0.2745, 0.2680},
					{ 0.7552, 0.5730, 0.4313, 0.3300, 0.2690, 0.2490}};
				size_t a = (size_t)(elevation/500+0.5);
				if (a<0) a = 0;
				if (a>5) a = 5;
                tau = a0[hm][a] + a1[hm][a]*exp(-k[hm][a]/sin(alpha));
			}
			// compute sky diffuse
			// post values
			solar_direct = Gon*tau;
			solar_horizontal = Gon*tau;
			solar_altitude = alpha*180/2/PI;
			solar_azimuth = z*180/2/PI;
		}
		else
		{
			solar_direct = 0;
			solar_horizontal = 0;
			solar_altitude = 0;
			solar_azimuth = 0;
		}
	}
}

void weather::windcalcs(TIMESTAMP ts)
{
	if ( get_options(AC_WINDCALCS) )
	{
		double v = gl_random_rayleigh(&(my()->rng_state),log(wind_speed));
	}
}
void weather::interpolate(TIMESTAMP ts)
{
	if ( get_options(AC_INTERPOLATE) )
	{
		// TODO
	}
}
int weather::notify_wind_heading(const char *value)
{
	if ( get_options(AC_WINDCALCS) )
	{
		return 0;
	}
	return 1;
}
int weather::notify_wind_speed(const char *value)
{
	if ( get_options(AC_WINDCALCS) )
	{
		return 0;
	}
	return 1;
}
int weather::notify_wind_gusts(const char *value)
{
	if ( get_options(AC_WINDCALCS) )
	{
		return 0;
	}
	return 1;
}

int weather::kmldump(int (*stream)(const char*, ...))
{
  if ( isnan(get_latitude()) || isnan(get_longitude()) ) return 0;
  stream("<Placemark>\n");
  stream("  <name>%s</name>\n", get_name());
  stream("  <description>\n<![CDATA[\n");
  // TODO add popup data here
  stream("    ]]>\n");
  stream("  </description>\n");
  stream("  <Point>\n");
  stream("    <coordinates>%f,%f</coordinates>\n", get_longitude(), get_latitude());
  stream("  </Point>\n");
  stream("</Placemark>");
  return 1;
}
