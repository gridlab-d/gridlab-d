/** $Id: climate.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file climate.cpp
	@author David P. Chassin
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "climate.h"
#include "timestamp.h"

#define RAD(x) (x*PI)/180
                         //ET,CT,MT ,PT ,AT, HT
double std_meridians[] = {75,90,105,120,135,150};

double surface_angles[] = {
	360,	// H
	180,	// N
	135,	// NE
	90,		// E
	45,		// SE
	0,		// S
	-45,	// SW
	-90,	// W
	-135,	// NW
};


EXPORT int64 calculate_solar_radiation_degrees(OBJECT *obj, double tilt, double orientation, double *value){
	return calculate_solar_radiation_radians(obj, RAD(tilt), RAD(orientation), value);
}

EXPORT int64 calculate_solar_radiation_radians(OBJECT *obj, double tilt, double orientation, double *value){
	static SolarAngles sa; // just for the functions
	double ghr, dhr, dnr = 0.0;
	double cos_incident = 0.0;
	double std_time = 0.0;
	double solar_time = 0.0;
	short int doy = 0;
	DATETIME dt;


	climate *cli;
	if(obj == 0 || value == 0){
		//throw "climate/calc_solar: null object pointer in arguement";
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		//throw "climate/calc_solar: input object is not a climate object";
		return 0;
	}
	ghr = cli->solar_global;
	dhr = cli->solar_diffuse;
	dnr = cli->solar_direct;

	gl_localtime(obj->clock, &dt);
	std_time = (double)(dt.hour) + ((double)dt.minute)/60.0;
	solar_time = sa.solar_time(std_time, doy, cli->tz_meridian, obj->longitude);
	cos_incident = sa.cos_incident(RAD(obj->latitude), tilt, orientation, solar_time, doy);

	*value = dnr * cos_incident + dhr * (1 + cos(tilt)) / 2 + ghr * (1 - cos(tilt)) * cli->ground_reflectivity / 2;

	return 1;
}

/**
	@addtogroup tmy TMY2 data
	@ingroup climate

	Opens a TMY2 file for reading and reads in the header data.  Some of the data
	in the header is used in solar radiation calculations, so this data needs to
	be held in memory for future use.  This also sets the file pointer used for
	later reads.

	http://rredc.nrel.gov/solar/pubs/tmy2/tab3-2.html
 @{
 **/
int tmy2_reader::open(const char *file){
	fp = fopen(file, "r");

	if(fp == NULL){
		gl_error("tmy2_reader::open() -- fopen failed on \"%s\"", file);
		return 0;
	}

	// read in the header (use the c code given in the manual)
	if(fgets(buf,500,fp)){
		return sscanf(buf,"%*s %75s %3s %d %*s %d %d %*s %d %d",data_city,data_state,&tz_offset,&lat_degrees,&lat_minutes,&long_degrees,&long_minutes);
	} else {
		gl_error("tmy2_reader::open() -- first fgets read nothing");
		return 0; /* fgets didn't read anything */
	}
	//return 4;
}

/**
	Reads the next line in and stores it in a character buffer.
*/

int tmy2_reader::next(){
	// read the next line into the buffer using fgets.
	char *val = fgets(buf,500,fp);

	if(val != NULL)
		return 1;
	else
		return 0;
}

/**
	Passes the header data by reference out to the calling function.

	@param city the city the data represents
	@param state the state the city is located in
	@param degrees latitude degrees
	@param minutes latitude minutes
	@param long_deg longitude degrees
	@param long_min longitude minutes
*/
int tmy2_reader::header_info(char* city, char* state, int* degrees, int* minutes, int* long_deg, int* long_min){
	if(city) strcpy(city,data_city); /* potential buffer overflow */
	if(state) strcpy(state,data_state); /* potential buffer overflow */
	if(degrees) *degrees = lat_degrees;
	if(minutes) *minutes = lat_minutes;
	if(long_deg) *long_deg = long_degrees;
	if(long_min) *long_min = long_minutes;
	return 1;
}

/**
	Parses the data stored in the internal buffer.

	@param dnr Direct Normal Radiation
	@param dhr Diffuse Horizontal Radiation
	@param tdb Bulb temperature
	@param rh  Relative Humidity
	@param month month of year
	@param day day of month
	@param hour hour of day
*/

int tmy2_reader::read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind, double *precip, double *snowDepth){
	int rct = 0;
	int tmp_dnr, tmp_dhr, tmp_tdb, tmp_rh, tmp_ws, tmp_precip, tmp_sf, tmp_ghr;
	//sscanf(buf, "%*2s%2d%2d%2d%*14s%4d%*2s%4d%*40s%4d%8*s%3d%*s",month,day,hour,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh);
	int tmh, tday, thr;
	if(month == NULL) month = &tmh;
	if(day == NULL) day = &tday;
	if(hour == NULL) hour = &thr;
	rct = sscanf(buf, "%*2s%2d%2d%2d%*8s%4d%*2s%4d%*2s%4d%*34s%4d%*8s%3d%*13s%3d%*25s%3d%*7s%3d",month,day,hour,&tmp_ghr,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh, &tmp_ws,&tmp_precip,&tmp_sf);
				/* 3__5__7__9___17_20_23_27__29_33___67_71_79__82___95_98_ */
	if(rct != 11){
		gl_warning("TMY reader did not get 11 values for line time %d/%d %d00", *month, *day, *hour);
	}
	if(dnr) *dnr = tmp_dnr;
	if(dhr) *dhr = tmp_dhr;
	if(ghr) *ghr = tmp_ghr;
	if(tdb)
	{
		*tdb = ((double)tmp_tdb)/10.0;
		if (*tdb<low_temp || low_temp==0) low_temp = *tdb;
		else if (*tdb>high_temp || high_temp==0) high_temp = *tdb;
	}
	/* *tdb = ((double)tmp_tdb)/10.0 * 9.0 / 5.0 + 32.0; */
	if(rh) *rh = ((double)tmp_rh)/100.0;
	if(wind) *wind = tmp_ws/10.0;

	// COnvert precip in mm to in/h
	if(precip) *precip = ((double)tmp_precip) * 0.03937;

	//convert snowfall in cm to in
	if(snowDepth) *snowDepth =  ((double)tmp_sf) * 0.3937;

	return 1;
}

/**
	Calculate the solar radation for a surface facing the given compass point.

	@param cpt compass point of the direction the surface is facing
	@param doy day of year
	@param lat latitude of the surface
	@param sol_time the solar time of day
	@param dnr Direct Normal Radiation
	@param dhr Diffuse Horizontal Radiation
	@param ghr Global Horizontal Radiation
	@param gnd_ref Ground Reflectivity
	@param vert_angle the angle of the surface relative to the horizon (Default is 90 degrees)
*/
double tmy2_reader::calc_solar(COMPASS_PTS cpt, short doy, double lat, double sol_time, double dnr, double dhr, double ghr, double gnd_ref, double vert_angle = 90)
{
	SolarAngles *sa = new SolarAngles();
	double surface_angle = surface_angles[cpt];
	double cos_incident = sa->cos_incident(lat,RAD(vert_angle),RAD(surface_angle),sol_time,doy);

	//double solar = (dnr * cos_incident + dhr/2 + ghr * gnd_ref);
	double solar = dnr * cos_incident + dhr;

	if (peak_solar==0 || solar>peak_solar) peak_solar = solar;
	return solar;
}
/**
	Closes the readers internal file pointer
*/
void tmy2_reader::close(){
	fclose(fp);
}

/**@}**********************************************************/

/**	@addtogroup climate Weather (climate)
	@ingroup modules

	Climate implementation
 @{
 **/
CLASS *climate::oclass = NULL;
climate *climate::defaults = NULL;

climate::climate(MODULE *module)
{
	memset(this, 0, sizeof(climate));
	if (oclass==NULL)
	{
		oclass = gl_register_class(module,"climate",sizeof(climate),PC_PRETOPDOWN);
		if (gl_publish_variable(oclass,
			PT_char32, "city", PADDR(city),
			PT_char1024,"tmyfile",PADDR(tmyfile),
			PT_double,"temperature[degF]",PADDR(temperature),
			PT_double,"humidity[%]",PADDR(humidity),
			PT_double,"solar_flux[W/sf]",PADDR(solar_flux),	PT_SIZE, 9,
			PT_double,"solar_direct[W/sf]",PADDR(solar_direct),
			PT_double,"solar_diffuse[W/sf]",PADDR(solar_diffuse),
			PT_double,"solar_global[W/sf]",PADDR(solar_global),
			PT_double,"wind_speed[mph]", PADDR(wind_speed),
			PT_double,"wind_dir[deg]", PADDR(wind_dir),
			PT_double,"wind_gust[mph]", PADDR(wind_gust),
			PT_double,"record.low[degF]", PADDR(record.low),
			PT_int32,"record.low_day",PADDR(record.low_day),
			PT_double,"record.high[degF]", PADDR(record.high),
			PT_int32,"record.high_day",PADDR(record.high_day),
			PT_double,"record.solar[W/sf]", PADDR(record.solar),
			PT_double,"rainfall[in/h]",PADDR(rainfall),
			PT_double,"snowdepth[in]",PADDR(snowdepth),
			PT_enumeration,"interpolate",PADDR(interpolate),PT_DESCRIPTION,"the interpolation mode used on the climate data",
				PT_KEYWORD,"NONE",CI_NONE,
				PT_KEYWORD,"LINEAR",CI_LINEAR,
				PT_KEYWORD,"QUADRATIC",CI_QUADRATIC,
			PT_double,"solar_horiz",PADDR(solar_flux[CP_H]),
			PT_double,"solar_north",PADDR(solar_flux[CP_N]),
			PT_double,"solar_northeast",PADDR(solar_flux[CP_NE]),
			PT_double,"solar_east",PADDR(solar_flux[CP_E]),
			PT_double,"solar_southeast",PADDR(solar_flux[CP_SE]),
			PT_double,"solar_south",PADDR(solar_flux[CP_S]),
			PT_double,"solar_southwest",PADDR(solar_flux[CP_SW]),
			PT_double,"solar_west",PADDR(solar_flux[CP_W]),
			PT_double,"solar_northwest",PADDR(solar_flux[CP_NW]),
			PT_double,"solar_raw[W/sf]",PADDR(solar_raw),
			PT_double,"ground_reflectivity[%]",PADDR(ground_reflectivity),
			PT_object,"reader",PADDR(reader),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(climate));
		strcpy(city,"");
		strcpy(tmyfile,"");
		temperature = 59.0;
		temperature_raw = 15.0;
		humidity = 0.75;
		rainfall = 0.0;
		snowdepth = 0.0;
		ground_reflectivity = 0.3;
		//solar_flux = malloc(8 * sizeof(double));
		solar_flux[0] = solar_flux[1] = solar_flux[2] = solar_flux[3] = solar_flux[4] = solar_flux[5] = solar_flux[6] = solar_flux[7] = solar_flux[8] = 0.0; // W/sf normal
		//solar_flux_S = solar_flux_SE = solar_flux_SW = solar_flux_E = solar_flux_W = solar_flux_NE = solar_flux_NW = solar_flux_N = 0.0; // W/sf normal
		tmy = NULL;
		sa = new SolarAngles();
		defaults = this;
		gl_publish_function(oclass,	"calculate_solar_radiation_degrees", (FUNCTIONADDR)calculate_solar_radiation_degrees);
		gl_publish_function(oclass,	"calculate_solar_radiation_radians", (FUNCTIONADDR)calculate_solar_radiation_radians);
	}
}

int climate::create(void)
{
	memcpy(this,defaults,sizeof(climate));
	return 1;
}

int climate::isa(char *classname)
{
	if(classname != 0)
		return (0 == strcmp(classname,"climate"));
	else
		return 0;
}

int climate::init(OBJECT *parent)
{
	char *dot = 0;
	OBJECT *obj=OBJECTHDR(this);

	reader_type = RT_NONE;

	// ignore "" files ~ manual climate control is a feature
	if (strcmp(tmyfile,"")==0)
		return 1;

	// open access to the TMY file
	char *found_file = gl_findfile(tmyfile,NULL,FF_READ);
	if (found_file==NULL) // TODO: get proper values for solar
	{
		gl_error("weather file '%s' access failed", tmyfile);
		return 0;
	}

	
	//dot = strchr(tmyfile, '.');
	//while(strchr(dot+1, '.')){ /* init time, doesn't have to be fast -MH */
	//	dot = strchr(dot, '.');
	//}
	if(strstr(tmyfile, ".tmy2") || strstr(tmyfile,".tmy")){
		reader_type = RT_TMY2;
	} else if(strstr(tmyfile, ".csv")){
		reader_type = RT_CSV;
	} else {
		gl_warning("climate: unrecognized filetype, assuming TMY2");
	}

	if(reader_type == RT_CSV){
		// may or may not have an object,
		// have not called open()
		if(reader == NULL){
			int rv = 0;
			csv_reader *creader = new csv_reader();
			reader_hndl = creader;
			rv = creader->open(found_file);
//			creader->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &wind_speed, &rainfall, &snowdepth);
			return rv;
		} else {
			int rv = 0;
			csv_reader *my = OBJECTDATA(reader,csv_reader);
			reader_hndl = my;
			rv = my->open(my->filename);
//			my->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &wind_speed, &rainfall, &snowdepth);
			return rv;
		}
	}

	// implicit if(reader_type == RT_TMY2) ~ do the following
	if( file.open(found_file) < 3 ){
		gl_error("climate::init() -- weather file header improperly formed");
		return 0;
	}
	
	// begin parsing the TMY file
	int line=0;
	tmy = (TMYDATA*)malloc(sizeof(TMYDATA)*8760);
	if (tmy==NULL)
	{
		gl_error("TMY buffer allocation failed");
		return 0;
	}

	int month, day, hour;//, year;
	double dnr,dhr,ghr,wspeed,precip,snowdepth;
	//char cty[50];
	//char st[3];
	int lat_deg,lat_min,long_deg,long_min;
	/* The city/state data isn't used anywhere.  -mhauer */
	//file.header_info(cty,st,&lat_deg,&lat_min,&long_deg,&long_min);
	file.header_info(NULL,NULL,&lat_deg,&lat_min,&long_deg,&long_min);
	obj->latitude = (double)lat_deg + ((double)lat_min) / 60;
	obj->longitude = (double)long_deg + ((double)long_min) / 60;
	tz_meridian =  15 * abs(file.tz_offset);//std_meridians[-file.tz_offset-5];
	while (line<8760 && file.next())
	{

		file.read_data(&dnr,&dhr,&ghr,&temperature,&humidity,&month,&day,&hour,&wspeed,&precip,&snowdepth);

		int doy = sa->day_of_yr(month,day);
		int hoy = (doy - 1) * 24 + (hour-1);
		if (hoy>=0 && hoy<8760){
			// pre-conversion of solar data from W/m^2 to W/sf
			if(0 == gl_convert("W/m^2", "W/sf", &(dnr))){
				gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
				return 0;
			}
			if(0 == gl_convert("W/m^2", "W/sf", &(dhr))){
				gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
				return 0;
			}
			if(0 == gl_convert("W/m^2", "W/sf", &(ghr))){
				gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
				return 0;
			}
			tmy[hoy].temp_raw = temperature;
			tmy[hoy].temp = temperature;
			// post-conversion of copy of temperature from C to F
			if(0 == gl_convert("degC", "degF", &(tmy[hoy].temp))){
				gl_error("climate::init unable to gl_convert() 'degC' to 'degF'!");
				return 0;
			}
			tmy[hoy].windspeed=wspeed;
			tmy[hoy].rh = humidity;
			tmy[hoy].dnr = dnr;
			tmy[hoy].dhr = dhr;
			tmy[hoy].ghr = ghr;
			tmy[hoy].rainfall = precip;
			tmy[hoy].snowdepth = snowdepth;
			tmy[hoy].solar_raw = dnr;
			// calculate the solar radiation
			double sol_time = sa->solar_time((double)hour,doy,RAD(tz_meridian),RAD(obj->longitude));
			double sol_rad = 0.0;

			for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
				if(c_point == CP_H)
					sol_rad = file.calc_solar(CP_E,doy,RAD(obj->latitude),sol_time,dnr,dhr,ghr,ground_reflectivity,0.0);//(double)dnr * cos_incident + dhr;
				else
					sol_rad = file.calc_solar(c_point,doy,RAD(obj->latitude),sol_time,dnr,dhr,ghr,ground_reflectivity);//(double)dnr * cos_incident + dhr;
				/* TMY2 solar radiation data is in Watt-hours per square meter. */
				tmy[hoy].solar[c_point] = sol_rad;

				/* track records */
				if (sol_rad>record.solar || record.solar==0) record.solar = sol_rad;
				if (tmy[hoy].temp>record.high || record.high==0)
				{
					record.high = tmy[hoy].temp;
					record.high_day = doy;
				}
				if (tmy[hoy].temp<record.low || record.low==0)
				{
					record.low = tmy[hoy].temp;
					record.low_day = doy;
				}
			}

		}
		else
			gl_error("%s(%d): day %d, hour %d is out of allowed range 0-8759 hours", tmyfile,line,day,hour);

		line++;
	}
	file.close();

	return 1;
}

TIMESTAMP climate::sync(TIMESTAMP t0) /* called in presync */
{
	int rv = 0;
	if(t0 > TS_ZERO && reader_type == RT_CSV){
		DATETIME now;
		gl_localtime(t0, &now);
		//OBJECT *obj = OBJECTHDR(this);
		csv_reader *cr = OBJECTDATA(reader,csv_reader);
		rv = cr->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &solar_global, &wind_speed, &rainfall, &snowdepth);
		// calculate the solar radiation
		double sol_time = sa->solar_time((double)now.hour+now.minute/60.0+now.second/3600.0,now.yearday,RAD(tz_meridian),RAD(reader->longitude));
		double sol_rad = 0.0;

		for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
			if(c_point == CP_H)
				sol_rad = file.calc_solar(CP_E,now.yearday,RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity,0.0);//(double)dnr * cos_incident + dhr;
			else
				sol_rad = file.calc_solar(c_point,now.yearday,RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity);//(double)dnr * cos_incident + dhr;
			/* TMY2 solar radiation data is in Watt-hours per square meter. */
			solar_flux[c_point] = sol_rad;
		}
		return rv;
	}
	if (t0>TS_ZERO && tmy!=NULL)
	{
		DATETIME ts;
		int localres = gl_localtime(t0,&ts);
		int hoy;
		double now, hoy0, hoy1, hoy2;
		if(localres == 0){
			GL_THROW("climate::sync -- unable to resolve localtime!");
		}
		int doy = sa->day_of_yr(ts.month,ts.day);
		hoy = (doy - 1) * 24 + (ts.hour);
		switch(interpolate){
			case CI_NONE:
				temperature = tmy[hoy].temp;
				temperature_raw = tmy[hoy].temp_raw;
				humidity = tmy[hoy].rh;
				solar_direct = tmy[hoy].dnr;
				solar_diffuse = tmy[hoy].dhr;
				solar_global = tmy[hoy].ghr;
				solar_raw = tmy[hoy].solar_raw;
				this->wind_speed = tmy[hoy].windspeed;
				this->rainfall = tmy[hoy].rainfall;
				this->snowdepth = tmy[hoy].snowdepth;

				if(memcmp(solar_flux,tmy[hoy].solar,CP_LAST*sizeof(double)))
					memcpy(solar_flux,tmy[hoy].solar,CP_LAST*sizeof(double));
				break;
			case CI_LINEAR:
				now = hoy+ts.minute/60.0;
				hoy0 = hoy;
				hoy1 = hoy+1.0;
				temperature = gl_lerp(now, hoy0, tmy[hoy].temp, hoy1, tmy[hoy+1%8760].temp);
				temperature_raw = gl_lerp(now, hoy0, tmy[hoy].temp_raw, hoy1, tmy[hoy+1%8760].temp_raw);
				humidity = gl_lerp(now, hoy0, tmy[hoy].rh, hoy1, tmy[hoy+1%8760].rh);
				solar_direct = gl_lerp(now, hoy0, tmy[hoy].dnr, hoy1, tmy[hoy+1%8760].dnr);
				solar_diffuse = gl_lerp(now, hoy0, tmy[hoy].dhr, hoy1, tmy[hoy+1%8760].dhr);
				solar_global = gl_lerp(now, hoy0, tmy[hoy].ghr, hoy1, tmy[hoy+1%8760].ghr);
				wind_speed = gl_lerp(now, hoy0, tmy[hoy].windspeed, hoy1, tmy[hoy+1%8760].windspeed);
				rainfall = gl_lerp(now, hoy0, tmy[hoy].rainfall, hoy1, tmy[hoy+1%8760].rainfall);
				snowdepth = gl_lerp(now, hoy0, tmy[hoy].snowdepth, hoy1, tmy[hoy+1%8760].snowdepth);
				solar_raw = gl_lerp(now, hoy0, tmy[hoy].solar_raw, hoy1, tmy[hoy+1%8760].solar_raw);
				for(int pt = 0; pt < CP_LAST; ++pt){
					solar_flux[pt] = gl_lerp(now, hoy0, tmy[hoy].solar[pt], hoy1, tmy[hoy+1%8760].solar[pt]);
				}
				break;
			case CI_QUADRATIC:
				now = hoy+ts.minute/60.0;
				hoy0 = hoy;
				hoy1 = hoy+1.0;
				hoy2 = hoy+2.0;
				temperature = gl_qerp(now, hoy0, tmy[hoy].temp, hoy1, tmy[hoy+1%8760].temp, hoy2, tmy[hoy+2%8760].temp);
				temperature_raw = gl_qerp(now, hoy0, tmy[hoy].temp_raw, hoy1, tmy[hoy+1%8760].temp_raw, hoy2, tmy[hoy+2%8760].temp_raw);
				humidity = gl_qerp(now, hoy0, tmy[hoy].rh, hoy1, tmy[hoy+1%8760].rh, hoy2, tmy[hoy+2%8760].rh);
				solar_direct = gl_qerp(now, hoy0, tmy[hoy].dnr, hoy1, tmy[hoy+1%8760].dnr, hoy2, tmy[hoy+2%8760].dnr);
				solar_diffuse = gl_qerp(now, hoy0, tmy[hoy].dhr, hoy1, tmy[hoy+1%8760].dhr, hoy2, tmy[hoy+2%8760].dhr);
				solar_global = gl_qerp(now, hoy0, tmy[hoy].ghr, hoy1, tmy[hoy+1%8760].ghr, hoy2, tmy[hoy+2%8760].ghr);
				wind_speed = gl_qerp(now, hoy0, tmy[hoy].windspeed, hoy1, tmy[hoy+1%8760].windspeed, hoy2, tmy[hoy+2%8760].windspeed);
				rainfall = gl_qerp(now, hoy0, tmy[hoy].rainfall, hoy1, tmy[hoy+1%8760].rainfall, hoy2, tmy[hoy+2%8760].rainfall);
				snowdepth = gl_qerp(now, hoy0, tmy[hoy].snowdepth, hoy1, tmy[hoy+1%8760].snowdepth, hoy2, tmy[hoy+2%8760].snowdepth);
				solar_raw = gl_qerp(now, hoy0, tmy[hoy].solar_raw, hoy1, tmy[hoy+1%8760].solar_raw, hoy2, tmy[hoy+2%8760].solar_raw);
				for(int pt = 0; pt < CP_LAST; ++pt){
					if(tmy[hoy].solar[pt] == tmy[hoy+1].solar[pt]){
						solar_flux[pt] = tmy[hoy].solar[pt];
					} else {
						solar_flux[pt] = gl_qerp(now, hoy0, tmy[hoy].solar[pt], hoy1, tmy[hoy+1%8760].solar[pt], hoy2, tmy[hoy+2%8760].solar[pt]);
						if(solar_flux[pt] < 0.0)
							solar_flux[pt] = 0.0; /* quadratic isn't always cooperative... */
					}
				}
				break;
			default:
				GL_THROW("climate::sync -- unrecognize interpolation mode!");
		}
		return -(t0+(3600*TS_SECOND-t0%(3600 *TS_SECOND))); /// negative means soft event
	}
	return TS_NEVER;
}

//////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF CORE LINKAGE: climate
//////////////////////////////////////////////////////////////////////////

/// Create a climate object
EXPORT int create_climate(OBJECT **obj, ///< a pointer to the OBJECT*
						  OBJECT *parent) ///< a pointer to the parent OBJECT
{
	*obj = gl_create_object(climate::oclass);
	if (*obj!=NULL)
	{
		climate *my = OBJECTDATA(*obj,climate);
		gl_set_parent(*obj,parent);
		my->create();
		return 1;
	}
	return 0;
}

EXPORT int isa_climate(OBJECT *obj, char *classname)
{
	if(obj != 0 && classname != 0)
		return OBJECTDATA(obj,climate)->isa(classname);
	else
		return 0;
}

/// Initialize the climate object
EXPORT int init_climate(OBJECT *obj, /// a pointer to the OBJECT
						OBJECT *parent) /// a poitner to the parent OBJECT
{
	if (obj!=NULL)
	{
		climate *my = OBJECTDATA(obj,climate);
		return my->init(parent);
	}
	return 0;
}

/// Synchronize the cliamte object
EXPORT TIMESTAMP sync_climate(OBJECT *obj, /// a pointer to the OBJECT
							  TIMESTAMP t0) /// the time to which the OBJECT's clock should advance
{
	TIMESTAMP t1 = TS_NEVER;
	try
	{
		t1 = OBJECTDATA(obj,climate)->sync(t0); // presync really
	}
	catch(char *msg)
	{
		gl_error("climate::sync exception caught: %s", msg);
		t1 = TS_INVALID;
	}
	catch (...)
	{
		gl_error("climate::sync exception caught: no info");
		t1 = TS_INVALID;
	}
	obj->clock = t0;
	return t1;
}

/**@}**/


