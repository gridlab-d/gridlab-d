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
double surface_angles[8] = {180,135,90,45,0,-45,-90,-135}; // Old surface_angles = {0,45,-45,90,-90,135,-135,180};

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

int tmy2_reader::read_data(double *dnr, double *dhr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind, double *precip, double *snowDepth){
	int tmp_dnr, tmp_dhr, tmp_tdb, tmp_rh, tmp_ws, tmp_precip, tmp_sf;
	//sscanf(buf, "%*2s%2d%2d%2d%*14s%4d%*2s%4d%*40s%4d%8*s%3d%*s",month,day,hour,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh);
	int tmh, tday, thr;
	if(month == NULL) month = &tmh;
	if(day == NULL) day = &tday;
	if(hour == NULL) hour = &thr;
	sscanf(buf, "%*2s%2d%2d%2d%*14s%4d%*2s%4d%*34s%4d%8*s%3d%*13s%3d%*25s%3d%*7s%3d",month,day,hour,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh, &tmp_ws,&tmp_precip,&tmp_sf);
				/* 3__5__7__9___23_27__29_33___67_71_79__82___95_98_ */
	if(dnr) *dnr = tmp_dnr;
	if(dhr) *dhr = tmp_dhr;
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
	@param vert_angle the angle of the surface relative to the horizon (Default is 90 degrees)
*/
double tmy2_reader::calc_solar(COMPASS_PTS cpt, short doy, double lat, double sol_time, double dnr, double dhr,double vert_angle = 90)
{
	SolarAngles *sa = new SolarAngles();
	double surface_angle = surface_angles[cpt];
	double cos_incident = sa->cos_incident(lat,RAD(vert_angle),RAD(surface_angle),sol_time,doy);

	double solar = ((double)dnr * cos_incident + dhr);
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
			PT_double,"wind_speed[mph]", PADDR(wind_speed),
			PT_double,"wind_dir[deg]", PADDR(wind_dir),
			PT_double,"wind_gust[mph]", PADDR(wind_gust),
			PT_double,"record.low[degF]", PADDR(record.low),
			PT_double,"record.high[degF]", PADDR(record.high),
			PT_double,"record.solar[W/sf]", PADDR(record.solar),
			PT_double,"rainfall[in/h]",PADDR(rainfall),
			PT_double,"snowdepth[in]",PADDR(snowdepth),
			PT_enumeration,"interpolate",PADDR(interpolate),PT_DESCRIPTION,"the interpolation mode used on the climate data",
				PT_KEYWORD,"NONE",CI_NONE,
				PT_KEYWORD,"LINEAR",CI_LINEAR,
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(climate));
		strcpy(city,"");
		strcpy(tmyfile,"");
		temperature = 59.0;
		temperature_raw = 15.0;
		humidity = 0.75;
		rainfall = 0.0;
		snowdepth = 0.0;
		//solar_flux = malloc(8 * sizeof(double));
		solar_flux[0] = solar_flux[1] = solar_flux[2] = solar_flux[3] = solar_flux[4] = solar_flux[5] = solar_flux[6] = solar_flux[7] = solar_flux[8] = 0.0; // W/sf normal
		//solar_flux_S = solar_flux_SE = solar_flux_SW = solar_flux_E = solar_flux_W = solar_flux_NE = solar_flux_NW = solar_flux_N = 0.0; // W/sf normal
		tmy = NULL;
		sa = new SolarAngles();
		defaults = this;
	}
}

int climate::create(void) 
{
	memcpy(this,defaults,sizeof(climate));
	return 1;
}

int climate::init(OBJECT *parent)
{
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

	int month,day, hour;
	double dnr,dhr, wspeed,precip,snowdepth;
	//char cty[50];
	//char st[3];
	int lat_deg,lat_min,long_deg,long_min;
	/* The city/state data isn't used anywhere.  -mhauer */
	//file.header_info(cty,st,&lat_deg,&lat_min,&long_deg,&long_min);
	file.header_info(NULL,NULL,&lat_deg,&lat_min,&long_deg,&long_min);
	double degrees = (double)lat_deg + ((double)lat_min) / 60;
	double long_degrees = (double)long_deg + ((double)long_min) / 60;
	double tz_meridian =  15 * abs(file.tz_offset);//std_meridians[-file.tz_offset-5];
	while (line<8760 && file.next())
	{
		
		file.read_data(&dnr,&dhr,&temperature,&humidity,&month,&day,&hour,&wspeed,&precip,&snowdepth);
		int doy = sa->day_of_yr(month,day);
		int hoy = (doy - 1) * 24 + (hour-1); 
		if (hoy>=0 && hoy<8760){
			tmy[hoy].temp_raw = temperature;
			tmy[hoy].temp = temperature;
			tmy[hoy].windspeed=wspeed;
			if(0 == gl_convert("degC", "degF", &(tmy[hoy].temp))){
				gl_error("climate::init unable to gl_convert() 'degC' to 'degF'!");
				return 0;
			}
			tmy[hoy].rh = humidity;
			tmy[hoy].dnr = dnr;
			tmy[hoy].dhr = dhr;
			tmy[hoy].rainfall = precip;
			tmy[hoy].snowdepth = snowdepth;
			// calculate the solar radiation
			double sol_time = sa->solar_time((double)hour,doy,RAD(tz_meridian),RAD(long_degrees));
			double sol_rad = 0.0; 
			
			for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
				if(c_point == CP_H)
					sol_rad = file.calc_solar(CP_E,doy,RAD(degrees),sol_time,dnr,dhr,0.0);//(double)dnr * cos_incident + dhr;
				else
					sol_rad = file.calc_solar(c_point,doy,RAD(degrees),sol_time,dnr,dhr);//(double)dnr * cos_incident + dhr;
				/* TMY2 solar radiation data is in Watt-hours per square meter. */
				if(0 == gl_convert("W/m^2", "W/sf", &(sol_rad))){
					gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
					return 0;
				}
				tmy[hoy].solar[c_point] = sol_rad;

				/* track records */
				if (sol_rad>record.solar || record.solar==0) record.solar = sol_rad;
				if (temperature>record.high || record.high==0) record.high = temperature;
				if (temperature<record.low || record.low==0) record.low = temperature;
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
	if (t0>TS_ZERO && tmy!=NULL)
	{
		DATETIME ts;
		int localres = gl_localtime(t0,&ts);
		int hoy;
		double now, hoy0, hoy1;
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
				wind_speed = gl_lerp(now, hoy0, tmy[hoy].windspeed, hoy1, tmy[hoy+1%8760].windspeed);
				rainfall = gl_lerp(now, hoy0, tmy[hoy].rainfall, hoy1, tmy[hoy+1%8760].rainfall);
				snowdepth = gl_lerp(now, hoy0, tmy[hoy].snowdepth, hoy1, tmy[hoy+1%8760].snowdepth);
				//for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
				for(int pt = 0; pt < CP_LAST; ++pt){
					solar_flux[pt] = gl_lerp(now, hoy0, tmy[hoy].solar[pt], hoy1, tmy[hoy+1%8760].solar[pt]);
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
	TIMESTAMP t1 = OBJECTDATA(obj,climate)->sync(t0); // presync really
	obj->clock = t0;
	return t1;
}

/**@}**/
