/** $Id: climate.cpp 4738 2014-07-03 00:55:39Z dchassin $
/** $Id: climate.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file climate.cpp
	@author David P. Chassin
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <string>
#include "gridlabd.h"
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include "climate.h"
#include "timestamp.h"
EXPORT_CREATE(climate)
EXPORT_INIT(climate)
EXPORT_SYNC(climate)
EXPORT_ISA(climate)

#define RAD(x) (x*PI)/180

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
bool is_TMY2 = 0;


//Cloud pattern constants
//The off-screen pattern size is affected by the following two constants.
// These have been set under the assumption of a max wind speed (after altitude adjustment to 1000m)
// of 80 m/s.  At 256 pixles/tile * 20m/pixle  the off-screen pattern size is sufficient to ensure
// that  no undefined areas of the pattern make it on-screen in the 60 s update period.
// Adjusting either of these breaks that assumption and may force other change to be made, such
// as reducing the time between cloud pattern updates.

//EDITME
const int CLOUD_TILE_SIZE = 512;
const int PIXEL_EDGE_SIZE = 20; //Pixel size example: 10m per edge, 100 m^2 area.
const double KM_PER_DEG = 111.32;
const long EMPTY_VALUE = -999;
//const int ALPHA = 300;
//const int NUM_FUZZY_LAYERS = 20;



std::vector<std::vector<double > > cloud_pattern;
std::vector<std::vector<double > > normalized_cloud_pattern;
std::vector<std::vector<int > > binary_cloud_pattern;
std::vector<std::vector<std::vector<double> > > fuzzy_cloud_pattern;
int on_screen_size = 0;
int cloud_pattern_size = 0;
TIMESTAMP last_binary_conversion_time = 0;



EXPORT int64 calculate_solar_radiation_degrees(OBJECT *obj, double tilt, double orientation, double *value)
{
	return calculate_solar_radiation_shading_radians(obj, RAD(tilt), RAD(orientation), 1.0, value);
}

EXPORT int64 calculate_solar_radiation_radians(OBJECT *obj, double tilt, double orientation, double *value){
	return calculate_solar_radiation_shading_radians(obj, tilt, orientation, 1.0, value);
}

EXPORT int64 calculate_solar_radiation_shading_degrees(OBJECT *obj, double tilt, double orientation, double shading_value, double *value) {
	return calculate_solar_radiation_shading_radians(obj, RAD(tilt), RAD(orientation), shading_value, value);
}

EXPORT int64 calculate_solar_radiation_shading_radians(OBJECT *obj, double tilt, double orientation, double shading_value, double *value){
	return calculate_solar_radiation_shading_position_radians(obj, tilt, orientation, NaN, NaN, shading_value, value);
}

EXPORT int64 calculate_solar_radiation_shading_position_radians(OBJECT *obj, double tilt, double orientation, double latitude, double longitude, double shading_value, double *value){
	static SolarAngles sa; // just for the functions
	double ghr, dhr, dnr = 0.0;
	double cos_incident = 0.0;
	double std_time = 0.0;
	double solar_time = 0.0;
	short int doy = 0;
	DATETIME dt;

	climate *cli;
	if(obj == 0 || value == 0){
		//throw "climate/calc_solar: null object pointer in argument";
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		//throw "climate/calc_solar: input object is not a climate object";
		return 0;
	}

	cli->get_solar_for_location(latitude, longitude, &dnr, &ghr, &dhr);

	gl_localtime(obj->clock, &dt);
	std_time = (double)(dt.hour) + ((double)dt.minute)/60.0  + (dt.is_dst ? -1.0:0.0);
	doy=sa.day_of_yr(dt.month,dt.day);
	solar_time = sa.solar_time(std_time, doy, RAD(cli->get_tz_meridian()), RAD(obj->longitude));
	cos_incident = sa.cos_incident(RAD(obj->latitude), tilt, orientation, solar_time, doy);
	*value = (shading_value*dnr*cos_incident) + dhr*(1+cos(tilt))/2. + ghr*(1-cos(tilt))*cli->get_ground_reflectivity()/2.;

	return 1;
}

//Degree version of the new solpos-based solar position/radiation algorithms
EXPORT int64 calc_solar_solpos_shading_deg(OBJECT *obj, double tilt, double orientation, double shading_value, double *value)
{
	return calc_solar_solpos_shading_rad(obj, RAD(tilt), RAD(orientation), shading_value, value);
}

//Solar radiation calcuation based on solpos and Perez tilt models
EXPORT int64 calc_solar_solpos_shading_rad(OBJECT *obj, double tilt, double orientation, double shading_value, double *value)
{
	return calc_solar_solpos_shading_position_rad(obj, tilt, orientation, NaN, NaN, shading_value, value);
}

//Solar radiation calcuation based on solpos and Perez tilt models
EXPORT int64 calc_solar_solpos_shading_position_rad(OBJECT *obj, double tilt, double orientation, double latitude, double longitude, double shading_value, double *value)
{
	static SolarAngles sa; // just for the functions
	double ghr, dhr, dnr;
	double cos_incident;
	double temp_value;
	DATETIME dt;
	TIMESTAMP offsetclock;

	climate *cli;
	if(obj == 0 || value == 0){
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		return 0;
	}

	cli->get_solar_for_location(latitude, longitude, &dnr, &ghr, &dhr);

	if (cli->reader_type==1)//check if reader_type is TMY2.
	{
		//Adjust time by half an hour - adjusts per TMY "reading" intervals - what they really represent
		offsetclock = obj->clock + 1800;
	}
	else	//Just pass it in
	{
		offsetclock = obj->clock;
	}

	gl_localtime(offsetclock, &dt);

	//Convert temperature back to centrigrade - since we seem to like imperial units
	temp_value = ((cli->get_temperature() - 32.0)*5.0/9.0);

	//Initialize solpos algorithm
	sa.S_init(&sa.solpos_vals);

	//Assign in values
	sa.solpos_vals.longitude = obj->longitude;
	sa.solpos_vals.latitude = RAD(obj->latitude);
	if (dt.is_dst == 1)
	{
		sa.solpos_vals.timezone = cli->get_tz_offset_val()-1.0;
	}
	else
	{
		sa.solpos_vals.timezone = cli->get_tz_offset_val();
	}
	sa.solpos_vals.year = dt.year;
	sa.solpos_vals.daynum = (dt.yearday+1);
	sa.solpos_vals.hour = dt.hour+(dt.is_dst?-1:0);
	sa.solpos_vals.minute = dt.minute;
	sa.solpos_vals.second = dt.second;
	sa.solpos_vals.temp = temp_value;
	sa.solpos_vals.press = cli->get_pressure();

	// Solar constant associated with extraterrestrial DNI, 1367 W/sq m - pull from TMY for now
	//sa.solpos_vals.solcon = 126.998456;	//Use constant value for direct normal extraterrestrial irradiance - doesn't seem right to me
	sa.solpos_vals.solcon = cli->get_direct_normal_extra();	//Use weather-read version (TMY)

	sa.solpos_vals.aspect = orientation;
	sa.solpos_vals.tilt = tilt;
	sa.solpos_vals.diff_horz = dhr;
	sa.solpos_vals.dir_norm = dnr;

	//Calculate different solar position values
	sa.S_solpos(&sa.solpos_vals);

	//Pull off new cosine of incidence
	if (sa.solpos_vals.cosinc >= 0.0)
		cos_incident = sa.solpos_vals.cosinc;
	else
		cos_incident = 0.0;

	//Apply the adjustment
	*value = (shading_value*dnr*cos_incident) + dhr*sa.solpos_vals.perez_horz + ghr*((1-cos(tilt))*cli->get_ground_reflectivity()/2.0);

	return 1;
}

EXPORT int64 calc_solar_ideal_shading_position_radians(OBJECT *obj, double tilt, double latitude, double longitude, double shading_value, double *value) {
	double ghr, dhr, dnr;
	double cos_incident;
	double temp_value;
	DATETIME dt;
	TIMESTAMP offsetclock;

	climate *cli;
	if(obj == 0 || value == 0){
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		return 0;
	}

	cli->get_solar_for_location(latitude, longitude, &dnr, &ghr, &dhr);
	// strictly speaking, the diffuse horizontal value should be modified to account for the
	// fraction of the sky dome seen by the panel, but this is copied from solar.c
	*value = shading_value*(dnr) + dhr + ghr*(1-cos(tilt))*(cli->get_ground_reflectivity())/2.0;
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
	char temp_lat_hem[2];
	char temp_long_hem[2];
	int sscan_rv;
  char location_data[10];
	char ld[10],tz[10],tlad[10],tlod[10],el[10];
	float lat_degrees_temp;
	float long_degrees_temp;
	float tz_offset_temp;

	//std::string filenameis;
	//filenameis = file;
  is_TMY2 = 0;
	std::string s = file;
	std::string delimiter = ".";
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
	    token = s.substr(0, pos);
	    s.erase(0, pos + delimiter.length());
	}

	fp = fopen(file, "r");

	if(fp == NULL){
		gl_error("tmy2_reader::open() -- fopen failed on \"%s\"", file);
		return 0;
	}

	// read in the header (use the c code given in the manual)
	if(fgets(buf,500,fp)){

		//Initialize variables - so comparison functions work better
		temp_lat_hem[0] = temp_lat_hem[1] = '\0';
		temp_long_hem[0] = temp_long_hem[1] = '\0';

		if (s == "tmy2") {
			sscan_rv = sscanf(buf,"%*s %75s %3s %d %c %d %d %c %d %d %d",data_city,data_state,&tz_offset,temp_lat_hem,&lat_degrees,&lat_minutes,temp_long_hem,&long_degrees,&long_minutes,&elevation);
      gl_warning("Daylight saving time (DST) is not handled correctly when using TMY2 datasets; please use TMY3 for DST-corrected weather data.");
      is_TMY2 = 1;
		}
		else if (s == "tmy3") {
			sscan_rv = sscanf(buf,"%*[^','],%[^','],%[^','],%[^','],%[^','],%[^','],%s",data_city,data_state,tz,tlad,tlod,el);			
			tz_offset_temp    = atof(tz);
			lat_degrees_temp  = atof(tlad);
			long_degrees_temp = atof(tlod);
			elevation         = atoi(el);

			tz_offset = (int) tz_offset_temp;			
			if (lat_degrees_temp < 0) {
				temp_lat_hem[0] = 'S';
			}
			else {
				temp_lat_hem[0] = 'N';
			}

			if (long_degrees_temp < 0) {
				temp_long_hem[0] ='W';
			}
			else {
				temp_long_hem[0] = 'E';
			}

			double frac_degrees, degrees;
			frac_degrees = modf(lat_degrees_temp, &degrees);
			lat_minutes =  fabs(round(frac_degrees*60));
			lat_degrees = fabs((long)lat_degrees_temp);
			frac_degrees = modf(long_degrees_temp, &degrees);
			long_minutes = fabs(round(frac_degrees*60));
			long_degrees = fabs((long)long_degrees_temp);
		}
		//See if latitude needs negation
		if ((strcmp(temp_lat_hem,"S")==0) || (strcmp(temp_lat_hem,"s")==0))
		{
			lat_degrees=-lat_degrees;
		}

		//Check longitude as well
		if ((strcmp(temp_long_hem,"W")==0) || (strcmp(temp_long_hem,"w")==0))
		{
			long_degrees=-long_degrees;
		}

		return sscan_rv;
	} else {
		gl_error("tmy2_reader::open() -- first fgets read nothing");
		return 0; /* fgets didn't read anything */
	}
	//return 4;
}

/**
	Reads the next line in and stores it in a character buffer.
*/

int tmy2_reader::next()
{
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
int tmy2_reader::header_info(char* city, char* state, int* degrees, int* minutes, int* long_deg, int* long_min)
{
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

int tmy2_reader::read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind, double *winddir, double *precip, double *snowDepth, double *pressure, double *extra_terr_dni, double *extra_terr_ghi, double *tot_sky_cov, double *opq_sky_cov){
	int rct = 0;
	int rct_ymd = 0;
	int rct_hm = 0;
	int tmp_dnr, tmp_dhr, tmp_tot_sky_cov, tmp_opq_sky_cov, tmp_tdb, tmp_rh, tmp_wd, tmp_ws, tmp_precip, tmp_sf, tmp_ghr, tmp_extra_ghr, tmp_extra_dni, tmp_press;
	//sscanf(buf, "%*2s%2d%2d%2d%*14s%4d%*2s%4d%*40s%4d%8*s%3d%*s",month,day,hour,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh);
	int tmh, tday, thr, t_mon, t_d,t_hr;
	char t_ymd[11],t_hm[10],t_ehr[10],t_dni[10],t_ghr[10],t_dnr[10],t_dhr[10],t_tkc[10],t_osc[10],t_tdb[10],t_rh[10],t_press[10],t_wd[10],t_ws[10],t_precip[10],t_sf[10],t_month[2],t_day[2],t_year[5],t_hour[2],t_min[2];
	if(month == NULL) month = &tmh;
	if(day == NULL) day = &tday;
	if(hour == NULL) hour = &thr;
	if(buf[2] == '/') {
		//rct = sscanf(buf, "%d/%d/%*s,%d:%*s,%*s,%d,%d,%*s,%*s,%d,%*s,%*s,%d,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%d,%*s,%*s,%*s,%*s,%*s,%d,%*s,%*s,%d,%*s,%*s,%*s,%*s,%*s,%d,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%d,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%*s,%d",month,day,hour,&tmp_extra_dni,&tmp_ghr,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh,&tmp_press,&tmp_ws,&tmp_precip,&tmp_sf);
		rct = sscanf(buf, "%[^','],%[^','],%[^','],%[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%*[^','],%[^','],%*[^','],%*[^','],%*[^','],%*s",t_ymd,t_hm,t_ehr,t_dni,t_ghr,t_dnr,t_dhr,t_tkc,t_osc,t_tdb,t_rh,t_press,t_wd,t_ws,t_precip,t_sf);
		//rct_ymd = sscanf(t_ymd,"%[^'/']/%[^'/']/%*s",t_month,t_day);
		//rct_hm = sscanf(t_hm,"%[^':']:%*s",t_hour);
		//t_mon = atoi(t_month);
		//t_d = atoi(t_day);
		//t_hr = atoi(t_hour);
		//month = &t_mon;
		//day = &t_d;
		//hour = &t_hr;
		rct_ymd = sscanf(t_ymd,"%d/%d/%*d",month,day);
		rct_hm = sscanf(t_hm,"%d:%*d",hour);
		tmp_extra_ghr = atoi(t_ehr);
		tmp_extra_dni = atoi(t_dni);
		tmp_ghr = atoi(t_ghr);
		tmp_dnr = atoi(t_dnr);
		tmp_dhr = atoi(t_dhr);
		tmp_tot_sky_cov = atoi(t_tkc);
		tmp_opq_sky_cov = atoi(t_osc);
		tmp_tdb = (int)(roundf(atof(t_tdb)*10)); //Since for TMY3, measurement is 1/10. So we are multiplying this by 10 to avoid code changes later on. tmp_tdb is now scaled same as TMY2 value
		tmp_rh = atoi(t_rh);
		tmp_press = atoi(t_press);
		tmp_wd = atoi(t_wd);
		tmp_ws = (int)(roundf(atof(t_ws)*10));
		tmp_precip = (int)(roundf(atof(t_precip)*10));//converting from centimeters to millimeters
		tmp_sf = atoi(t_sf);		
		rct = rct+1;
		tmp_sf = 0; //tmy3 doesnt have this field (snow depth). so set to 0

	}
	else {
	//rct = sscanf(buf, "%*2s%2d%2d%2d%*4s%4d%4d%*2s%4d%*2s%4d%*34s%4d%*8s%3d%*2s%4d%*7s%3d%*25s%3d%*7s%3d",month,day,hour,&tmp_extra_dni,&tmp_ghr,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh,&tmp_press,&tmp_ws,&tmp_precip,&tmp_sf);
	rct = sscanf(buf, "%*2s%2d%2d%2d%4d%4d%4d%*2s%4d%*2s%4d%*26s%2d%*2s%2d%*2s%4d%*8s%3d%*2s%4d%*2s%3d%*2s%3d%*25s%3d%*7s%3d",month,day,hour,&tmp_extra_ghr,&tmp_extra_dni,&tmp_ghr,&tmp_dnr,&tmp_dhr,&tmp_tot_sky_cov,&tmp_opq_sky_cov,&tmp_tdb,&tmp_rh,&tmp_press,&tmp_wd,&tmp_ws,&tmp_precip,&tmp_sf);
					/* 3__5__7__9__13__17_20_23_27__29_33___67_71_79__82__85__95_98_ */
	}
	if(rct != 17){
		gl_warning("TMY reader did not get 17 values for line time %d/%d %d00", *month, *day, *hour);
	}
	if(dnr) *dnr = tmp_dnr;
	if(dhr) *dhr = tmp_dhr;
	if(ghr) *ghr = tmp_ghr;

	//Assign extra_dni and pressure - extra DNI converted outside with other solar values
	if (extra_terr_dni) *extra_terr_dni = tmp_extra_dni;
	if (pressure) *pressure = tmp_press;

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

	// extraterrestrial global horizontal
	if (extra_terr_ghi) *extra_terr_ghi = tmp_extra_ghr;

	// total sky cover
	if (tot_sky_cov) *tot_sky_cov = tmp_tot_sky_cov/10.0;

	// opaque sky cover
	if (opq_sky_cov) *opq_sky_cov = tmp_opq_sky_cov/10.0;

	// wind direction
	if (winddir) *winddir = tmp_wd;

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
	delete sa;

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
		oclass = gld_class::create(module,"climate",sizeof(climate),PC_PRETOPDOWN|PC_AUTOLOCK);
		if (gl_publish_variable(oclass,
			PT_double,"solar_elevation",PADDR(solar_elevation), //sjin: publish solar elevation variable
			PT_double,"solar_azimuth",PADDR(solar_azimuth), //sjin: publish solar azimuth variable
            PT_double,"solar_zenith",PADDR(solar_zenith),
			PT_char32, "city", PADDR(city),
			PT_char1024,"tmyfile",PADDR(tmyfile),
			PT_double,"temperature[degF]",PADDR(temperature),
			PT_double,"humidity[pu]",PADDR(humidity),
			PT_double,"solar_flux[W/sf]",PADDR(solar_flux),	PT_SIZE, 9,
			PT_double,"solar_direct[W/sf]",PADDR(solar_direct),
			PT_double,"solar_diffuse[W/sf]",PADDR(solar_diffuse),
			PT_double,"solar_global[W/sf]",PADDR(solar_global),
			PT_double,"extraterrestrial_global_horizontal[W/sf]",PADDR(global_horizontal_extra),
			PT_double,"extraterrestrial_direct_normal[W/sf]",PADDR(direct_normal_extra),
			PT_double,"pressure[mbar]",PADDR(pressure),
			PT_double,"wind_speed[m/s]", PADDR(wind_speed),
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
				PT_KEYWORD,"NONE",(enumeration)CI_NONE,
				PT_KEYWORD,"LINEAR",(enumeration)CI_LINEAR,
				PT_KEYWORD,"QUADRATIC",(enumeration)CI_QUADRATIC,
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
			PT_double,"ground_reflectivity[pu]",PADDR(ground_reflectivity),
			PT_object,"reader",PADDR(reader),
			PT_char1024,"forecast",PADDR(forecast_spec),PT_DESCRIPTION,"forecasting specifications",
			PT_enumeration,"cloud_model",PADDR(cloud_model),PT_DESCRIPTION,"the cloud model to use",
				PT_KEYWORD,"NONE",(enumeration)CM_NONE,
				PT_KEYWORD,"CUMULUS",(enumeration)CM_CUMULUS,
			PT_double,"cloud_opacity[pu]",PADDR(cloud_opacity),
			PT_double,"opq_sky_cov[pu]",PADDR(opq_sky_cov),
			//PT_double,"cloud_reflectivity[pu]",PADDR(cloud_reflectivity), //Unused in the cloud model at this time.
			PT_double,"cloud_speed_factor[pu]",PADDR(cloud_speed_factor),
			PT_double,"solar_cloud_direct[W/sf]",PADDR(solar_cloud_direct),
			PT_double,"solar_cloud_diffuse[W/sf]",PADDR(solar_cloud_diffuse),
			PT_double,"solar_cloud_global[W/sf]",PADDR(solar_cloud_global),
			PT_double,"cloud_alpha[pu]",PADDR(cloud_alpha),
			PT_double,"cloud_num_layers[pu]",PADDR(cloud_num_layers),
			PT_double,"cloud_aerosol_transmissivity[pu]",PADDR(cloud_aerosol_transmissivity),
            PT_double,"update_time",PADDR(update_time),
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(climate));
		sa = new SolarAngles();
		defaults = this;
		gl_publish_function(oclass,	"calculate_solar_radiation_degrees", (FUNCTIONADDR)calculate_solar_radiation_degrees);
		gl_publish_function(oclass,	"calculate_solar_radiation_radians", (FUNCTIONADDR)calculate_solar_radiation_radians);
		gl_publish_function(oclass,	"calculate_solar_radiation_shading_degrees", (FUNCTIONADDR)calculate_solar_radiation_shading_degrees);
		gl_publish_function(oclass,	"calculate_solar_radiation_shading_radians", (FUNCTIONADDR)calculate_solar_radiation_shading_radians);
		//New solar position algorithm stuff
		gl_publish_function(oclass,	"calculate_solar_radiation_shading_position_radians", (FUNCTIONADDR)calculate_solar_radiation_shading_position_radians);
		gl_publish_function(oclass,	"calculate_solpos_radiation_shading_position_radians", (FUNCTIONADDR)calc_solar_solpos_shading_position_rad);
		gl_publish_function(oclass,	"calc_solar_ideal_shading_position_radians", (FUNCTIONADDR)calc_solar_ideal_shading_position_radians);
	}
}

int climate::create(void)
{
	memcpy(this,defaults,sizeof(climate));
	strcpy(city,"");
	strcpy(tmyfile,"");
	temperature = 59.0;
	temperature_raw = 15.0;
	humidity = 0.75;
	rainfall = 0.0;
	snowdepth = 0.0;
	ground_reflectivity = 0.3;
	direct_normal_extra = 126.998456;	//1367 W/m^2 constant in W/ft^2
	pressure = 1000;	//Sea level assumption
	//solar_flux = malloc(8 * sizeof(double));
	solar_flux[0] = solar_flux[1] = solar_flux[2] = solar_flux[3] = solar_flux[4] = solar_flux[5] = solar_flux[6] = solar_flux[7] = solar_flux[8] = 0.0; // W/sf normal
	//solar_flux_S = solar_flux_SE = solar_flux_SW = solar_flux_E = solar_flux_W = solar_flux_NE = solar_flux_NW = solar_flux_N = 0.0; // W/sf normal
	cloud_opacity = 1.0;
	cloud_speed_factor = 1;
	//cloud_reflectivity = 1.0; // very reflective!
	tmy = NULL;
	cloud_model = CM_NONE;
	cloud_num_layers = 40;
	cloud_alpha = 400;
	cloud_aerosol_transmissivity = 0.95;
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
	TIMESTAMP t0 = obj->clock;
	double meter_to_feet = 1.0;
	double tz_num_offset;

	reader_type = RT_NONE;

	// ignore "" files ~ manual climate control is a feature
	if (strcmp(tmyfile,"")==0)
		return 1;

	// open access to the TMY file
	char found_file[1024];
	if (gl_findfile(tmyfile.get_string(),NULL,R_OK,found_file,sizeof(found_file))==NULL) // TODO: get proper values for solar
	{
		gl_error("weather file '%s' access failed", tmyfile.get_string());
		return 0;
	}


	if (cloud_model != CM_NONE) {
		//Cloud model input error checking.
		if (cloud_opacity > 1){
			gl_warning("climate:%s - Cloud opacity must be no greater than 1.0, setting to 1.0",obj->name);
			cloud_opacity = 1.0;
		}
		else if (cloud_opacity < 0){
			gl_warning("climate:%s - Cloud opacity must be no less than 0.0, setting to 0.0",obj->name);
			cloud_opacity = 0.0;
		}
		/*
		if (cloud_reflectivity > 1){
			gl_warning("climate:%s - Cloud reflectivity must be no greater than 1.0, setting to 1.0",obj->name);
			cloud_opacity = 1.0;
		}
		else if (cloud_reflectivity < 0){
			gl_warning("climate:%s - Cloud reflectivity must be no less than 0.0, setting to 0.0",obj->name);
			cloud_opacity = 0.0;
		}
		*/
		if (cloud_speed_factor < 0){
			gl_warning("climate:%s - Cloud speed adjustment cannot be negative, setting to 1.0",obj->name);
			cloud_speed_factor = 1.0;
		}
		if (cloud_alpha < cloud_num_layers){
				gl_warning("climate:%s - Cloud model alpha value must be less than or equal to cloud_num_layers, setting to cloud_num_layers",obj->name);
				cloud_alpha = cloud_num_layers;
		}
		if (cloud_aerosol_transmissivity < 0){
				gl_warning("climate:%s - Cloud model aerosol transmissivity must be greater than or equal to 0, setting to default value of 0.9",obj->name);
				cloud_aerosol_transmissivity = 0.9;
		}
		if (cloud_aerosol_transmissivity > 1){
				gl_warning("climate:%s - Cloud model aerosol transmissivity must be less than or equal to 1, setting to 1",obj->name);
				cloud_aerosol_transmissivity = 1.0;
		}
		gl_warning("This cloud model places a large burden on computational resources. Patience and/or a more capable computer may be required.");
		init_cloud_pattern();
		//write_out_cloud_pattern('C');
		convert_to_binary_cloud();
		//write_out_cloud_pattern('B');
		convert_to_fuzzy_cloud(EMPTY_VALUE, cloud_num_layers, cloud_alpha);
		prev_NTime = t0-60;
	}
	
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
		int rv = 0;

		if(reader == NULL){
			gl_error("climate::init(): no csv_reader specified for tmyfile %s", tmyfile.get_string());
			/* TROUBLESHOOT
				The weather file provided is for the csv_reader object but not csv_reader object was specified in the reader property. 
				Please specify the name of the csv_reader object in the climate object in the glm.
			*/
			return 0;
		} else {
			if((reader->flags & OF_INIT) != OF_INIT){
				char objname[256];
				gl_verbose("climate::init(): deferring initialization on %s", gl_name(reader, objname, 255));
				return 2; // defer
			}
			csv_reader *my = OBJECTDATA(reader,csv_reader);
			reader_hndl = my;
			rv = my->open(my->filename);
//			my->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &wind_speed, &rainfall, &snowdepth);
			//Pull timezone information
			tz_num_offset = my->tz_numval;
			tz_offset_val = tz_num_offset;

			//Copy latitude and longitude information from CSV reader
			obj->latitude = reader->latitude;
			obj->longitude = reader->longitude;


			//CSV Reader validity check
			if (fabs(obj->latitude) > 90)
			{
				gl_error("climate:%s - Latitude is outside +/-90!",obj->name);
				//Defined below
				return 0;
			}

			if (fabs(obj->longitude) > 180)
			{
				gl_error("climate:%s - Longitude is outside +/-180!",obj->name);
				//Defined below
				return 0;
			}

			//Generic warning about southern hemisphere and Duffie-Beckman usage
			if (obj->latitude<0)
			{
				gl_warning("climate:%s - Southern hemisphere solar position model may have issues",obj->name);
				/*  TROUBLESHOOT
				The default solar position model was built around a northern hemisphere assumption.  As such,
				it doesn't always produce completely accurate results for southern hemisphere locations.  Calculated
				insolation values are approximately correct, but may show discrepancies against measured data.  If
				this climate is associated with a solar object, use the SOLAR_TILT_MODEL SOLPOS to ensure proper
				results (this warning will still pop up).
				*/
			}

			//Set the timezone offset - stolen from TMY code below
			tz_meridian =  15 * tz_num_offset;//std_meridians[-file.tz_offset-5];
		}

		return rv;
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
	double dnr,dhr,ghr,wspeed,wdir,precip,snowdepth,pressure,extra_dni,extra_ghi,tot_sky_cov,opq_sky_cov;
	//char cty[50];
	//char st[3];
	int lat_deg,lat_min,long_deg,long_min;
	/* The city/state data isn't used anywhere.  -mhauer */
	//file.header_info(cty,st,&lat_deg,&lat_min,&long_deg,&long_min);
	file.header_info(NULL,NULL,&lat_deg,&lat_min,&long_deg,&long_min);

	//Handle hemispheres
	if (lat_deg<0)
		set_latitude((double)lat_deg - (((double)lat_min) / 60));
	else
		set_latitude((double)lat_deg + (((double)lat_min) / 60));

	if (long_deg<0)
		set_longitude((double)long_deg - (((double)long_min) / 60));
	else
		set_longitude((double)long_deg + (((double)long_min) / 60));

	//Generic check for TMY files
	if (fabs(obj->latitude) > 90)
	{
		gl_error("climate:%s - Latitude is outside +/-90!",obj->name);
		/*  TROUBLESHOOT
		The value read from the weather data indicates a latitude of greater
		than 90 or less than -90 degrees.  This is not a valid value.  Please specify
		the latitude in this range, with positive values representing the northern hemisphere
		and negative values representing the southern hemisphere.
		*/
		return 0;
	}

	if (fabs(obj->longitude) > 180)
	{
		gl_error("climate:%s - Longitude is outside +/-180!",obj->name);
		/*  TROUBLESHOOT
		The value read from the weather data indicates a longitude of greater
		than 180 or less than -180 degrees.  This is not a valid value.  Please specify
		the longitude in this range, with positive values representing the eastern hemisphere
		and negative values representing the western hemisphere.
		*/
		return 0;
	}

	//Generic warning about southern hemisphere and Duffie-Beckman usage
	if (obj->latitude<0)
	{
		gl_warning("climate:%s - Southern hemisphere solar position model may have issues",obj->name);
		//Defined above
	}

	if(0 == gl_convert("m", "ft", &meter_to_feet)){
		gl_error("climate::init unable to gl_convert() 'm' to 'ft'!");
		return 0;
	}
	file.elevation = (int)(file.elevation * meter_to_feet);
	tz_meridian =  15 * file.tz_offset;//std_meridians[-file.tz_offset-5];
	tz_offset_val = file.tz_offset;
	while (line<8760 && file.next())
	{
		while (isdigit(file.buf[1]) == 0) {
			file.next();
		}
		file.read_data(&dnr,&dhr,&ghr,&temperature,&humidity,&month,&day,&hour,&wspeed,&wdir,&precip,&snowdepth,&pressure,&extra_dni,&extra_ghi,&tot_sky_cov,&opq_sky_cov);

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
			if(0 == gl_convert("W/m^2", "W/sf", &(extra_dni))){
				gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
				return 0;
			}
			if(0 == gl_convert("W/m^2", "W/sf", &(extra_ghi))){
				gl_error("climate::init unable to gl_convert() 'W/m^2' to 'W/sf'!");
				return 0;
			}
			if(0 == gl_convert("mps", "mph", &(wspeed))){
				gl_error("climate::init unable to gl_convert() 'm/s' to 'miles/h'!");
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

			tmy[hoy].direct_normal_extra = extra_dni;
			tmy[hoy].pressure = pressure;
			
			tmy[hoy].global_horizontal_extra = extra_ghi;
			tmy[hoy].wind_dir = wdir;
			tmy[hoy].tot_sky_cov = tot_sky_cov;
			tmy[hoy].opq_sky_cov = opq_sky_cov;

			double sol_time = sa->solar_time((double)hour,doy,RAD(tz_meridian),RAD(get_longitude()));
			double sol_rad = 0.0;

			tmy[hoy].solar_elevation = sa->altitude(doy, RAD(obj->latitude), sol_time);
			tmy[hoy].solar_azimuth = sa->azimuth(doy, RAD(obj->latitude), sol_time);
			tmy[hoy].solar_zenith = (90. * PI_OVER_180)-tmy[hoy].solar_elevation;

			for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
				if(c_point == CP_H)
					sol_rad = file.calc_solar(CP_E,doy,RAD(get_latitude()),sol_time,dnr,dhr,ghr,ground_reflectivity,0.0);//(double)dnr * cos_incident + dhr;
				else
					sol_rad = file.calc_solar(c_point,doy,RAD(get_latitude()),sol_time,dnr,dhr,ghr,ground_reflectivity);//(double)dnr * cos_incident + dhr;
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
			gl_error("%s(%d): day %d, hour %d is out of allowed range 0-8759 hours", tmyfile.get_string(),line,day,hour);

		line++;
	}
	file.close();

	/* initialize climate to starttime */
	presync(gl_globalclock);

	/* enable forecasting if specified */
#if 0
	if ( strcmp(forecast_spec,"")!=0 && gl_forecast_create(my(),forecast_spec)==NULL )
	{
		gl_error("%s: forecast '%s' is not valid", get_name(), forecast_spec.get_string());
		return 0;
	}
	else if (get_forecast()!=NULL)
	{	
		/* initialize the forecast data entity */
		FORECAST *fc = get_forecast();
		fc->propref = get_property("temperature");
		gl_forecast_save(fc,get_clock(),3600,0,NULL);
		set_flags(get_flags()|OF_FORECAST);
	}
#endif
	return 1;
}

int climate::get_solar_for_location(double latitude, double longitude, double *direct, double *global, double *diffuse) {
	int retval = 1;
	//int cloud = 0; //binary cloud
	double cloud = 0; //fuzzy cloud
	double f;
	double ETR;
	double ETRN;
	double sol_z;
	OBJECT *obj=OBJECTHDR(this);
	DATETIME dt;
	gl_localtime(obj->clock, &dt);


	switch (get_cloud_model()) {
		case CM_CUMULUS:
			// cloud = 0 -> clear view of sun
			// cloud = 1 -> very dark cloud blocking sun
			retval = get_fuzzy_cloud_value_for_location(latitude, longitude, &cloud); //Fuzzy cloud pattern evaluation
			f = 1 - (cloud * cloud_opacity); //f=1 -> clear view of sun, f=0 -> very dark cloud blocking view of sun.
			//retval = get_binary_cloud_value_for_location(latitude, longitude, &cloud); //Binary cloud pattern evaluation
			//f = cloud ? 1.-cloud_opacity : 1.;
			sol_z = get_solar_zenith();
			ETRN = get_direct_normal_extra();
			ETR = ETRN * cos(sol_z);
			//std::cout << dt.month <<"," << dt.day <<"," << dt.hour <<"," << dt.minute <<"," << sol_z << std::endl;
			if (sol_z > RAD(90)){ //When sun is below the horizon, DNI must be zero.
        *direct = 0;
      }else{
        *direct = f*(global_transmissivity)*ETRN;
      }
			*diffuse = get_solar_diffuse();//(0.07 + 0.45*get_tot_sky_cov())*global_transmissivity*ETR;
				//get_solar_diffuse();
			*global = std::max((*direct)*std::max(cos(get_solar_zenith()),0.0) + *diffuse,0.0);
			solar_cloud_direct = *direct;
			solar_cloud_diffuse = *diffuse;
			solar_cloud_global = *global;
			break;
		default:
			*direct = get_solar_direct();
			*global = get_solar_global();
			*diffuse = get_solar_diffuse();
	}
	return retval;
}

int climate::get_binary_cloud_value_for_location(double latitude, double longitude, int *cloud) {
	int pixel_x = floor(gl_lerp(latitude, MIN_LAT, MIN_LAT_INDEX, MAX_LAT, MAX_LAT_INDEX));
	int pixel_y = floor(gl_lerp(longitude, MIN_LON, MIN_LON_INDEX, MAX_LON, MAX_LON_INDEX));
	*cloud = binary_cloud_pattern[pixel_x][pixel_y];
	//Debugging and validation
	//write_out_cloud_pattern('C');
//	write_out_cloud_pattern('B');
//	gl_output("%i,%f,%f,%i,%i,%i", prev_NTime, latitude, longitude, pixel_x, pixel_y,*cloud);
//	gl_output("%f,%i,%f,%i", latitude, pixel_x, longitude, pixel_y);
//	gl_output(" ");
	return 1;
}

int climate::get_fuzzy_cloud_value_for_location(double latitude, double longitude, double *cloud) {
	//write_out_cloud_pattern('F');
	int pixel_x = floor(gl_lerp(latitude, MIN_LAT, MIN_LAT_INDEX, MAX_LAT, MAX_LAT_INDEX));
	int pixel_y = floor(gl_lerp(longitude, MIN_LON, MIN_LON_INDEX, MAX_LON, MAX_LON_INDEX));
	double value = fuzzy_cloud_pattern[0][pixel_x][pixel_y];
	*cloud = fuzzy_cloud_pattern[0][pixel_x][pixel_y];
	//Debugging and validation
//	write_out_cloud_pattern('F');
//	write_out_cloud_pattern('B');
//	gl_output("%i,%f,%f,%i,%i,%f", prev_NTime, latitude, longitude, pixel_x, pixel_y,*cloud); //fuzzy clouds
//	gl_output("%f,%i,%f,%i", latitude, pixel_x, longitude, pixel_y);
//	gl_output(" ");
	return 1;
}

void climate::init_cloud_pattern() {

	using std::vector;
	int num_points = 0;

	// find the solar objects and count them
	OBJECT *gr_obj = 0;
	FINDLIST *items = gl_find_objects(FL_GROUP, "class=solar");
	for(gr_obj = gl_find_next(items, 0); gr_obj != 0; gr_obj = gl_find_next(items, gr_obj) ){
		num_points++;
	}

	// resize the container to the max number of objects
	vector<vector<double > > coord_list;
	coord_list.resize(num_points);
	for (int i = 0; i < num_points; ++i){
	    coord_list[i].resize(2);
	}

	// now populate the lat and lon, but only for those that have it
	PROPERTY *p_ptr;
	double latitude, longitude;
	num_points = 0;
	for(gr_obj = gl_find_next(items, 0); gr_obj != 0; gr_obj = gl_find_next(items, gr_obj) ){
		p_ptr = gl_get_property(gr_obj, "latitude");
		latitude = *gl_get_double(gr_obj, p_ptr);
		p_ptr = gl_get_property(gr_obj, "longitude");
		longitude = *gl_get_double(gr_obj, p_ptr);
		if (longitude != NaN && latitude != NaN) {
			coord_list[num_points][0] = latitude;
			coord_list[num_points][1] = longitude;
			num_points++;
		}
	}

	// finally resize to the number of solar objects that have lat and lon
	coord_list.resize(num_points);


	int num_tile_edge = calc_cloud_pattern_size(coord_list);
	cloud_pattern_size = num_tile_edge * CLOUD_TILE_SIZE + 1; //pattern must be 2^x + 1 square
	on_screen_size = (num_tile_edge - 2) * CLOUD_TILE_SIZE; //Off-screen area is one tile width around the perimeter of the on-screen area.

	//Build empty cloud pattern array
	cloud_pattern.resize(cloud_pattern_size);
	binary_cloud_pattern.resize(cloud_pattern_size);
	normalized_cloud_pattern.resize(cloud_pattern_size);
	for (int i = 0; i < cloud_pattern_size; ++i){
	    cloud_pattern[i].resize(cloud_pattern_size);
	    binary_cloud_pattern[i].resize(cloud_pattern_size);
	    normalized_cloud_pattern[i].resize(cloud_pattern_size);
	}

	//Initializing array to EMPTY_VALUE
  //TDH: trivially parallelizable
	for (int i = 0; i < cloud_pattern_size; i++ ){
			for (int j = 0; j < cloud_pattern_size; j++){
				cloud_pattern[i][j] = EMPTY_VALUE;
				binary_cloud_pattern[i][j] = EMPTY_VALUE;
				normalized_cloud_pattern[i][j] = EMPTY_VALUE;
			}
	}

	for (int i = 0; i < num_tile_edge; i++ ){
		for (int j = 0; j < num_tile_edge; j++){
			//Building pattern as a series of tiles
			int col_min = i*CLOUD_TILE_SIZE;
			int col_max = ((i+1)*CLOUD_TILE_SIZE);
			int row_min = j*CLOUD_TILE_SIZE;
			int row_max = ((j+1)*CLOUD_TILE_SIZE);
			build_cloud_pattern(col_min, col_max, row_min, row_max); //Min/Max x/y must always define a 2^x + 1 region of cloud_pattern
			//write_out_cloud_pattern('C');
		}
	}
}

void climate::update_cloud_pattern(TIMESTAMP delta_t) {
	int col_shifted_px = 0;
	int row_shifted_px = 0;
	double col_shift_request_px = 0;
	double row_shift_request_px = 0;

	// Calculating pattern shift vector due to wind
	double windspeed_tmy2 = get_wind_speed() * cloud_speed_factor;
	double wind_direct = get_wind_dir();

	//Using European Wind Atlas terrain roughness class and length to translate TMY2 windspeed at 10m to
	// cumulus cloud height, 1000m.
	//float roughness_class = 3; //Assumed. Slightly rural to suburban value.
	//double roughness_length = exp(log(10.0/3.0)*(roughness_class - 3.91249));
	//double windspeed = windspeed_tmy2 * log(1000/roughness_length)/log(10/roughness_length);

	//C. W. Hansen, J. S. Stein, and A. Ellis,
	//“Simulation of One-Minute Power Output from Utility-Scale Photovoltaic Generation Systems,” SAND2011-5529, 2011.
	// Shows distribution of wind speeds at cumulus cloud elevations based on weather balloon launches (pg. 18)
	// A cursory inspection of the TMY3 measured wind speeds shows this is roughly consistent with histogram
	// presented in the paper.
	//
	// As part of validation I found I was having to scale back the windspeed to roughly the same
	// speeds as the original source data. (Thus the "cloud_speed_factor" parameter.)
	// Removing windspeed adjustment and using source data as is.
	double windspeed = windspeed_tmy2;

	//Reformatting wind direction data
	wind_direct = wind_direct -180;
	wind_direct = -1 * (wind_direct -90);
	if (wind_direct <= -360) {
		wind_direct = wind_direct + 360;
	} else if (wind_direct >= 360){
		wind_direct = wind_direct - 360;
	}
	wind_direct = RAD(wind_direct);

	double time_step = delta_t; //Placeholder until we dig into the actual GLD synchronization process.

	double col_shift_px = (windspeed * cos(wind_direct) * time_step)/PIXEL_EDGE_SIZE;
	double row_shift_px = (windspeed * sin(wind_direct) * time_step)/PIXEL_EDGE_SIZE;

	col_shift_request_px = col_shift_request_px + col_shift_px;
	row_shift_request_px = row_shift_request_px + row_shift_px;

	//Accumulating wind shift vector so that small persistent winds still have an effect.
	int col_shift = 0;
	int row_shift = 0;
	if (fabs(col_shift_request_px - col_shifted_px) >= 1) {
		col_shift = floor(col_shift_request_px - col_shifted_px);
		col_shifted_px = col_shifted_px + col_shift;
	}
	if (fabs(row_shift_request_px - row_shifted_px) >= 1) {
		row_shift = floor(row_shift_request_px - row_shifted_px);
		row_shifted_px = row_shifted_px + row_shift;
	}

	int row_boundary = abs(row_shift);
	int col_boundary = abs(col_shift);

	int col = 0;
	int row = 0;

	if (row_shift != 0 || col_shift != 0){
		if (row_shift >= 0 && col_shift >= 0 ){ //Wind blows from SW to NE
			if (col_shift > 0) {
				col = CLOUD_TILE_SIZE - col_boundary;
				//Checking to see if barely off-screen values are empty before shifting the pattern.
				// If check is not done, may result in EMPTY_VALUES getting shifted on_screen.
				for (row = CLOUD_TILE_SIZE; row < CLOUD_TILE_SIZE + on_screen_size; row++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('W');
						break;
					}
				}
			}
			if (row_shift > 0){
				row = CLOUD_TILE_SIZE - row_boundary;
				for (col = CLOUD_TILE_SIZE; col < CLOUD_TILE_SIZE + on_screen_size; col++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('S');
						break;
					}
				}
			}
			//Shifting pattern (after any edges have been rebuilt).
			for (row = cloud_pattern_size - 1; row >= 0; row--){
				for (col = cloud_pattern_size - 1; col >= 0; col--){
					if (row + row_shift > cloud_pattern_size -1 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					}else if (col + col_shift > cloud_pattern_size -1 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					} else {
						cloud_pattern[row + row_shift][col + col_shift] = cloud_pattern[row][col];
						cloud_pattern[row][col] = EMPTY_VALUE;
					}
				}
			}
		} else if (row_shift >= 0 && col_shift <= 0 ){ //Wind blows from SE to NW
			if (col_shift < 0) {
				col = CLOUD_TILE_SIZE + on_screen_size +  col_boundary;
				//Checking to see if barely off-screen values are empty before shifting the pattern.
				// If check is not done, may result in EMPTY_VALUES getting shifted on_screen.
				for (row = CLOUD_TILE_SIZE; row < CLOUD_TILE_SIZE + on_screen_size; row++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('E');
						break;
					}
				}
			}
			if (row_shift > 0){
				row = CLOUD_TILE_SIZE - row_boundary;
				for (col = CLOUD_TILE_SIZE; col < CLOUD_TILE_SIZE + on_screen_size; col++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('S');
						break;
					}
				}
			}
			//Shifting pattern (after any edges have been rebuilt).
			for (row = cloud_pattern_size - 1; row >= 0; row--){
				for (col = 0; col < cloud_pattern_size; col++){
					if (row + row_shift > cloud_pattern_size -1 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					}else if (col + col_shift < 0 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					} else {
						cloud_pattern[row + row_shift][col + col_shift] = cloud_pattern[row][col];
						cloud_pattern[row][col] = EMPTY_VALUE;
					}
				}
			}
		} else if (row_shift <= 0 && col_shift >= 0 ){ //Wind blows from NW to SE
			if (col_shift > 0) {
				col = CLOUD_TILE_SIZE - col_boundary;
				//Checking to see if barely off-screen values are empty before shifting the pattern.
				// If check is not done, may result in EMPTY_VALUES getting shifted on_screen.
				for (row = CLOUD_TILE_SIZE; row <= CLOUD_TILE_SIZE + on_screen_size; row++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('W');
						break;
					}
				}
			}
			if (row_shift < 0){
				row = CLOUD_TILE_SIZE + on_screen_size + row_boundary;
				for (col = CLOUD_TILE_SIZE; col < CLOUD_TILE_SIZE + on_screen_size; col++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {;
						rebuild_cloud_pattern_edge('N');
						break;
					}
				}
			}
			//Shifting pattern (after any edges have been rebuilt).
			for (row = 0; row < cloud_pattern_size ; row++){
				for (col = cloud_pattern_size - 1; col >= 0; col--){
					if (row + row_shift <  0 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					}else if (col + col_shift > cloud_pattern_size -1 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					} else {
						cloud_pattern[row + row_shift][col + col_shift] = cloud_pattern[row][col];
						cloud_pattern[row][col] = EMPTY_VALUE;
					}
				}
			}
		} else if (row_shift <= 0 && col_shift <= 0 ){ //Wind blows from NE to SW
			if (col_shift < 0) {
				col = CLOUD_TILE_SIZE + on_screen_size + col_boundary;
				//Checking to see if barely off-screen values are empty before shifting the pattern.
				// If check is not done, may result in EMPTY_VALUES getting shifted on_screen.
				for (row = CLOUD_TILE_SIZE; row < CLOUD_TILE_SIZE + on_screen_size; row++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('E');
						break;
					}
				}
			}
			if (row_shift < 0){
				row = CLOUD_TILE_SIZE + on_screen_size + row_boundary;
				for (col = CLOUD_TILE_SIZE; col < CLOUD_TILE_SIZE + on_screen_size; col++){
					if (cloud_pattern[row][col] == EMPTY_VALUE) {
						rebuild_cloud_pattern_edge('N');
						break;
					}
				}
			}
			//Shifting pattern (after any edges have been rebuilt).
			for (row = 0; row < cloud_pattern_size ; row++){
				for (col = 0; col < cloud_pattern_size; col++){
					if (row + row_shift <  0 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					}else if (col + col_shift < 0 ){
						cloud_pattern[row][col] = EMPTY_VALUE;
					} else {
						cloud_pattern[row + row_shift][col + col_shift] = cloud_pattern[row][col];
						cloud_pattern[row][col] = EMPTY_VALUE;
					}
				}
			}
		} else {
			//Shouldn't be able to get here.
		}
		solar_zenith = get_solar_zenith();
		if (solar_zenith < (110*PI/180)) { //Only do these things if the sun is above (or slightly below) the horizon).
				//Fractal cloud pattern is preserved and shifted appropriately but since the sun is below the horizon
				//  and the solar radiation on the surface is zero, we don't need to fully define the clouds.
			//Finding cloud outline shape
			double cut_elevation = 0;
			cut_elevation = convert_to_binary_cloud();
			//write_out_cloud_pattern('B');
			convert_to_fuzzy_cloud(cut_elevation, cloud_num_layers, cloud_alpha);
			//write_out_cloud_pattern('F');
		}
	}

}
double climate::convert_to_binary_cloud(){
	//Convert fractal cloud pattern to binary value based on TMY2 opaque sky value.
	//Place-holder location.  Eventually needs to be driven by objects asking for updated
	//  cloud values.  If the time since the last conversion is non-zero, this function needs
	//  to run to generate the new binary value.
	double cloud_value = get_opq_sky_cov(); //get_tot_sky_cov() or get_opq_sky_cov()

	/* TDH:
	 * Trying to counteract the fuzzification process by artificially boosting the coverage.
	 * This is a bit of a hack. The best way to handle this is to include the fuzzification process
	 * in the elevation cut adjustment. This way, the percentage of the sky covered by opaque clouds
	 * is ensured after the fuzzification has taken place. I've tested this and it REALLY slows things
	 * down (in an already slow function). For now, this is what I've chosen to do because it more
	 * or less works.
	*/
	cloud_value = cloud_value * 2.0;
	if (cloud_value >= 1){
		cloud_value = 0.99;
	}



	double search_tolerance = 0.005; //Defines how close is close enough when dialing in the binary cloud pattern.
	//OBJECT *obj=OBJECTHDR(this);
	//TIMESTAMP t1 = obj->clock;


	//TDH: trivially parallelizable
	 double cloud_pattern_max = cloud_pattern[CLOUD_TILE_SIZE][CLOUD_TILE_SIZE];
	 double cloud_pattern_min = cloud_pattern[CLOUD_TILE_SIZE][CLOUD_TILE_SIZE];
	 //Finding max and min value
	 for (int i = 0; i < cloud_pattern_size; i++){
		for (int j = 0; j < cloud_pattern_size; j++){
			if (cloud_pattern[i][j] != EMPTY_VALUE){
				if (cloud_pattern[i][j] > cloud_pattern_max){
					cloud_pattern_max = cloud_pattern[i][j];
				}
				if (cloud_pattern[i][j] < cloud_pattern_min){
					cloud_pattern_min = cloud_pattern[i][j];
				}
			}
		}
	 }

	 double cloud_pattern_range = cloud_pattern_max - cloud_pattern_min;
	 double cut_elevation = cloud_pattern_min;
	 int running_count = 0;
	 double measured_coverage = 0;
	 double step_size = 0;
	 double max_cut_elevation = cloud_pattern_max;
	 double min_cut_elevation = cloud_pattern_min;

	 //Creating normalized cloud pattern
	 for (int i = 0; i < cloud_pattern_size; i++){
		for (int j = 0; j < cloud_pattern_size; j++){
			if (cloud_pattern[i][j] != EMPTY_VALUE){
				normalized_cloud_pattern[i][j] = (cloud_pattern[i][j] - cloud_pattern_min)/cloud_pattern_range;
			}
		}
	 }

	 do{
		 cut_elevation += step_size;
		 running_count = 0;
		 for (int i = CLOUD_TILE_SIZE; i < CLOUD_TILE_SIZE + on_screen_size; i++){
			 for (int j = CLOUD_TILE_SIZE; j < CLOUD_TILE_SIZE + on_screen_size; j++){
				 if (normalized_cloud_pattern[i][j] != EMPTY_VALUE && normalized_cloud_pattern[i][j] <= cut_elevation){//Values less than cut elevation are clouds
					 running_count++;
				 }
			 }
		 }
		 measured_coverage = double(running_count)/(on_screen_size * on_screen_size); //Factor, range [0 1]

		 //Calculating next cut elevation.
		 if (measured_coverage > (cloud_value + search_tolerance)){
			 max_cut_elevation = cut_elevation;
			 step_size = (max_cut_elevation - min_cut_elevation)/-2;
		 }else if (measured_coverage < (cloud_value - search_tolerance)){
			 min_cut_elevation = cut_elevation;
			 step_size = (max_cut_elevation - min_cut_elevation)/2;
		 }
	 } while (measured_coverage < (cloud_value - search_tolerance) || measured_coverage > (cloud_value + search_tolerance));

	 //Converting cloud_pattern to binary_cloud_pattern
 //TDH: trivially parallelizable
	 for (int i = 0; i < cloud_pattern_size; i++){
		 for (int j = 0; j < cloud_pattern_size; j++){
			 if (normalized_cloud_pattern[i][j] == EMPTY_VALUE) {
				 binary_cloud_pattern[i][j] = EMPTY_VALUE;
			 }else if (normalized_cloud_pattern[i][j] <= cut_elevation){
				 binary_cloud_pattern[i][j] = 0; //Cloud
			 }else if (normalized_cloud_pattern[i][j] > cut_elevation){
				 binary_cloud_pattern[i][j] = 1; //Blue sky
			 }
		 }
	 }
	 return cut_elevation;

}

void climate::convert_to_fuzzy_cloud( double cut_elevation, int num_fuzzy_layers, double alpha){

	double shade_step_size = 1.0/alpha;

	if (cut_elevation == EMPTY_VALUE){ //Initialization call uses EMPTY_VALUE as the cut elevation.
    //TDH: trivially parallelizable
		//Resizing fuzzy cloud pattern
		fuzzy_cloud_pattern.resize(num_fuzzy_layers);
		for (int i = 0; i < num_fuzzy_layers; ++i){
			fuzzy_cloud_pattern[i].resize(cloud_pattern_size);
			for(int j = 0; j < cloud_pattern_size; j++){
				fuzzy_cloud_pattern[i][j].resize(cloud_pattern_size);
			}
		}

		//Initializing fuzzy cloud pattern
    //TDH: trivially parallelizable
		for (int i = 0; i < num_fuzzy_layers; i++){
			for (int j = 0; j < cloud_pattern_size; j++){
				for (int k = 0; k < cloud_pattern_size; k++){
					fuzzy_cloud_pattern[i][j][k] = 0;
				 }
			 }
		 }
	}

	//Filling in fuzzy pattern with random values
  //TDH: trivially parallelizable
	for (int i = 0; i < num_fuzzy_layers; i++){
		double rand_upper = ((double)(i+1)/(double)num_fuzzy_layers)*cut_elevation;
		double rand_lower = (((double)(i+1)-1)/(double)num_fuzzy_layers)*cut_elevation;
		for (int j = 0; j < cloud_pattern_size; j++){
			for (int kk = 0; kk < cloud_pattern_size; kk++){
				double binary = binary_cloud_pattern[j][kk];
				double normalized = normalized_cloud_pattern[j][kk];
				double fuzzy = fuzzy_cloud_pattern[0][j][kk];
				if (binary_cloud_pattern[j][kk] == 0.0 && normalized_cloud_pattern[j][kk] != EMPTY_VALUE && fuzzy_cloud_pattern[0][j][kk] != EMPTY_VALUE){ //Areas with 0 in the binary pattern are cloudy
					if (normalized_cloud_pattern[j][kk] <= cut_elevation - ((i+1)*shade_step_size)){ //only values below the cut elevation accumulate
						fuzzy_cloud_pattern[0][j][kk] = gl_random_uniform(RNGSTATE,rand_lower, rand_upper)  + fuzzy_cloud_pattern[0][j][kk];
						fuzzy = fuzzy_cloud_pattern[0][j][kk];
					}
				}else { //EMPTY_VALUES get coerced into 0.
					fuzzy_cloud_pattern[0][j][kk] = 0;
				}
			}
		}
	}

	//write_out_cloud_pattern('F');
	//Normalizing fuzzy pattern
	double max_value = fuzzy_cloud_pattern[0][0][0];
	double min_value = fuzzy_cloud_pattern[0][0][0];
	for (int j = 0; j < cloud_pattern_size; j++){
		for (int k = 0; k < cloud_pattern_size; k++){
			double value = fuzzy_cloud_pattern[0][j][k];
			if (fuzzy_cloud_pattern[0][j][k] > max_value){
				max_value = fuzzy_cloud_pattern[0][j][k];
			}
			if (fuzzy_cloud_pattern[0][j][k] < min_value){
				min_value = fuzzy_cloud_pattern[0][j][k];
			}
		}
	}
	double range = max_value - min_value;
	if (range != 0){
		for (int j = 0; j < cloud_pattern_size; j++){
			for (int k = 0; k < cloud_pattern_size; k++){
				double value = (fuzzy_cloud_pattern[0][j][k] - min_value)/range;
				fuzzy_cloud_pattern[0][j][k] = value;
			}
		}
	}else{
		//Do we need to to anything if the pattern is uniform?
	}

	///
	//Put EMPTY_VALUEs back in before calling it good.
  //TDH: trivially parallelizable
	for (int j = 0; j < cloud_pattern_size; j++){
		for (int k = 0; k < cloud_pattern_size; k++){
			if (cloud_pattern[j][k] == EMPTY_VALUE){
				fuzzy_cloud_pattern[0][j][k] = EMPTY_VALUE;
			}

		}
	}
	//write_out_cloud_pattern('F');

}


void climate::rebuild_cloud_pattern_edge( char edge_needing_rebuilt){
	int col_min = 0;
	int row_min = 0;
	int i = 0;

	if (edge_needing_rebuilt == 'W'){
		col_min = 0;
		row_min = 0;
		erase_off_screen_pattern('W');
		for (i = 0; i < 1 + on_screen_size/CLOUD_TILE_SIZE + 1; i++ ){
			build_cloud_pattern(col_min, col_min + CLOUD_TILE_SIZE, row_min, row_min + CLOUD_TILE_SIZE); //Min/Max x/y must always define a 2^x + 1 region of cloud_pattern
			row_min = row_min + CLOUD_TILE_SIZE;
		}
		trim_pattern_edge('W');
	} else if (edge_needing_rebuilt == 'E'){
		col_min = CLOUD_TILE_SIZE + on_screen_size - 1;
		row_min = 0;
		erase_off_screen_pattern('E');
		for (i = 0; i < 1 + on_screen_size/CLOUD_TILE_SIZE + 1; i++ ){
			build_cloud_pattern(col_min, col_min + CLOUD_TILE_SIZE, row_min, row_min + CLOUD_TILE_SIZE); //Min/Max x/y must always define a 2^x + 1 region of cloud_pattern
			row_min = row_min + CLOUD_TILE_SIZE;
		}
		trim_pattern_edge('E');
	} else if (edge_needing_rebuilt == 'N'){
		col_min = 0;
		row_min = CLOUD_TILE_SIZE + on_screen_size - 1;
		erase_off_screen_pattern('N');
		for (i = 0; i < 1 + on_screen_size/CLOUD_TILE_SIZE + 1; i++ ){
			build_cloud_pattern(col_min, col_min + CLOUD_TILE_SIZE, row_min, row_min + CLOUD_TILE_SIZE); //Min/Max row/col must always define a 2^x + 1 region of cloud_pattern
			col_min = col_min + CLOUD_TILE_SIZE;
		}
		trim_pattern_edge('N');
	} else if (edge_needing_rebuilt == 'S'){
		col_min = 0;
		row_min = 0;
		erase_off_screen_pattern('S');
		for (i = 0; i < 1 + on_screen_size/CLOUD_TILE_SIZE + 1; i++ ){
			build_cloud_pattern(col_min, col_min + CLOUD_TILE_SIZE, row_min, row_min + CLOUD_TILE_SIZE); //Min/Max row/col must always define a 2^x + 1 region of cloud_pattern
			col_min = col_min + CLOUD_TILE_SIZE;
		}
		trim_pattern_edge('S');
	} else{
		// Shouldn't be able to get here.
	}
}

void climate::trim_pattern_edge( char rebuilt_edge){
	int min_edge = 0;
	int min_edge_1 = 0;
	int min_edge_2 = min_edge_1;
	int min_edge_3 = min_edge_1;
	int max_edge = 0;
	int max_edge_1 = CLOUD_TILE_SIZE + on_screen_size + CLOUD_TILE_SIZE;
	int max_edge_2 = max_edge_1;
	int max_edge_3 = max_edge_1;
	int i = 0;
	int j = 0;
	//write_out_cloud_pattern('C');
	if (rebuilt_edge == 'W' || rebuilt_edge == 'E'){
		//Checking for boundary at southern edge of pattern.
		//Check three regions in areas south of on-screen: W, center, and E
    //TDH: trivially parallelizable - all three of these loops
		for (i = 0; i < CLOUD_TILE_SIZE; i++){
			if (cloud_pattern[i][10] != EMPTY_VALUE){
				min_edge_1 = i;
				break;
			}
		}
		for (i = 0; i < CLOUD_TILE_SIZE; i++){
			if (cloud_pattern[i][CLOUD_TILE_SIZE + 10] != EMPTY_VALUE){
				min_edge_2 = i;
				break;
			}
		}
		for (i = 0; i < CLOUD_TILE_SIZE; i++){
			if (cloud_pattern[i][CLOUD_TILE_SIZE + on_screen_size + 10] != EMPTY_VALUE){
				min_edge_3 = i;
				break;
			}
		}
		min_edge = std::max(min_edge_1,min_edge_2);
		min_edge = std::max(min_edge,min_edge_3);


		//Checking for boundary at northern edge of pattern
		for (i = CLOUD_TILE_SIZE + on_screen_size; i < cloud_pattern_size; i++){
			if (cloud_pattern[i][10] == EMPTY_VALUE){
				max_edge_1 = i;
				break;
			}
		}
		for (i = CLOUD_TILE_SIZE + on_screen_size; i < cloud_pattern_size; i++){
			if (cloud_pattern[i][CLOUD_TILE_SIZE + 10] == EMPTY_VALUE){
				max_edge_2 = i;
				break;
			}
		}
		for (i = CLOUD_TILE_SIZE + on_screen_size; i < cloud_pattern_size; i++){
			if (cloud_pattern[i][CLOUD_TILE_SIZE + on_screen_size + 10] == EMPTY_VALUE){
				max_edge_3 = i;
				break;
			}
		}
		max_edge = std::min(max_edge_1,max_edge_2);
		max_edge = std::min(max_edge,max_edge_3);

		//Trimming pattern
    //TDH: trivially parallelizable
		for(j = 0; j < cloud_pattern_size; j++){
			for(i = 0; i < min_edge; i++){
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
			for(i = max_edge; i < cloud_pattern_size; i++){
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
		//write_out_cloud_pattern('C');
	} else 	if (rebuilt_edge == 'N' || rebuilt_edge == 'S'){
		//Checking for boundary at western edge of pattern.
		//Check three regions in areas south of on-screen: S, center, and N
		//write_out_cloud_pattern('C');
    //TDH: trivially parallelizable - all three of these loops
		for (j = 0; j < CLOUD_TILE_SIZE; j++){
			if (cloud_pattern[10][j] != EMPTY_VALUE){
				min_edge_1 = j;
				break;
			}
		}
		for (j = 0; j < CLOUD_TILE_SIZE; j++){
			if (cloud_pattern[CLOUD_TILE_SIZE + 10][j] != EMPTY_VALUE){
				min_edge_2 = j;
				break;
			}
		}
		for (j = 0; j < CLOUD_TILE_SIZE; j++){
			if (cloud_pattern[CLOUD_TILE_SIZE + on_screen_size + 10][j] != EMPTY_VALUE){
				min_edge_3 = j;
				break;
			}
		}
		min_edge = std::max(min_edge_1,min_edge_2);
		min_edge = std::max(min_edge,min_edge_3);

		//Checking for boundary at eastern edge of pattern
		for (j = CLOUD_TILE_SIZE + on_screen_size; j < cloud_pattern_size; j++){
			if (cloud_pattern[10][j] == EMPTY_VALUE){
				max_edge_1 = j;
				break;
			}
		}
		for (j = CLOUD_TILE_SIZE + on_screen_size; j < cloud_pattern_size; j++){
			if (cloud_pattern[CLOUD_TILE_SIZE + 10][j] == EMPTY_VALUE){
				max_edge_2 = j;
				break;
			}
		}
		for (j = CLOUD_TILE_SIZE + on_screen_size; j < cloud_pattern_size; j++){
			if (cloud_pattern[CLOUD_TILE_SIZE + on_screen_size + 10][j] == EMPTY_VALUE){
				max_edge_3 = j;
				break;
			}
		}
		max_edge = std::min(max_edge_1,max_edge_2);
		max_edge = std::min(max_edge,max_edge_3);

		//Trimming pattern
    //TDH: trivially parallelizable
		for(i = 0; i < cloud_pattern_size; i++){
			for(j = 0; j < min_edge; j++){
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
			for(j = max_edge; j < cloud_pattern_size; j++){
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
	} else {
		//Shouldn't be able to get here.
	}
	//write_out_cloud_pattern('C');
}

void climate::erase_off_screen_pattern( char edge_to_erase){
	int col_min = 0;
	int row_min = 0;
  
  //TDH: trivially parallelizable - all four of these loops
	if (edge_to_erase == 'W'){
		col_min = 0;
		row_min = 0;
		for (int i = 0; i < cloud_pattern_size; i++){ //rows
			for (int j = 0; j < CLOUD_TILE_SIZE - 1; j++){ //cols
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
	} else if (edge_to_erase == 'E'){
		col_min = 0;
		row_min = 0;
		for (int i = 0; i < cloud_pattern_size; i++){ //rows
			for (int j = cloud_pattern_size - CLOUD_TILE_SIZE + 1; j < cloud_pattern_size; j++){ //cols
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
	} else if (edge_to_erase == 'N'){
		col_min = 0;
		row_min = 0;
		for (int i = cloud_pattern_size - CLOUD_TILE_SIZE + 1; i < cloud_pattern_size; i++){ //rows
			for (int j = 0; j < cloud_pattern_size; j++){ //cols
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
	} else if (edge_to_erase == 'S'){
		col_min = 0;
		row_min = 0;
		for (int i = 0; i < CLOUD_TILE_SIZE - 1; i++){ //rows
			for (int j = 0; j < cloud_pattern_size; j++){ //cols
				cloud_pattern[i][j] = EMPTY_VALUE;
			}
		}
	}
	//write_out_cloud_pattern('C');
}

void climate::write_out_pattern_shift(int row_shift, int col_shift){ //Used only for verification.
	using namespace std;
	ofstream out_file;

	out_file.open ("pattern_shift.csv", std::ios_base::app);
	out_file << prev_NTime<< "," << row_shift << "," << col_shift << endl;
	out_file.close();
}


void climate::write_out_cloud_pattern( char pattern){ //Used only for verification.
	using namespace std;
	ofstream out_file;

	char buffer [100];
	sprintf (buffer, "cloud_pattern_%010ld.csv", prev_NTime);
	std::string file_string = buffer;
	out_file.open(file_string.c_str(), ios::out);

	if (pattern == 'C'){
		for (int i = 0; i < cloud_pattern.size(); i++ ){
				for (int j = 0; j < cloud_pattern.size(); j++){
					if (j == (cloud_pattern.size()-1)){
						out_file << cloud_pattern[i][j] << endl;
					} else {
					out_file << cloud_pattern[i][j] << ",";
					}
				}
		}
		out_file.close();
	}else if (pattern == 'B'){
		for (int i = 0; i < binary_cloud_pattern.size(); i++ ){
				for (int j = 0; j < binary_cloud_pattern.size(); j++){
					if (j == (binary_cloud_pattern.size()-1)){
						out_file << binary_cloud_pattern[i][j] << endl;
					} else {
					out_file << binary_cloud_pattern[i][j] << ",";
					}
				}
		}
		out_file.close();
	}else if (pattern == 'F'){
	for (int i = 0; i < cloud_pattern_size; i++ ){
			for (int j = 0; j < cloud_pattern_size; j++){
				if (j == (cloud_pattern_size-1)){
					out_file << fuzzy_cloud_pattern[0][i][j] << endl;
				} else {
				out_file << fuzzy_cloud_pattern[0][i][j] << ",";
				}
			}
	}
	out_file.close();
}
}
void climate::build_cloud_pattern(int col_min, int col_max, int row_min, int row_max){ //Min/Max row/col must always define a 2^x + 1 region of cloud_pattern
	int const SIGMA = 5;
	int step = col_max - col_min;
	int half_step = step / 2;
	int max_num_recursions = log((double)(col_max - col_min))/log(2.0); //Change of base since c++ doesn't have a built-in log base 2.
	int col_start = col_min;
	int row_start = row_min;
	float stdev = SIGMA * SIGMA;

	//Seed corner values that are empty
	if (cloud_pattern[row_start][col_start] < EMPTY_VALUE * 0.98){
		cloud_pattern[row_start][col_start] = gl_random_normal(RNGSTATE,0,stdev);
	}
	if (cloud_pattern[row_start + step][col_start] < EMPTY_VALUE * 0.98){
		cloud_pattern[row_start + step][col_start] = gl_random_normal(RNGSTATE,0,stdev);
	}
	if (cloud_pattern[row_start][col_start + step] < EMPTY_VALUE * 0.98){
		cloud_pattern[row_start][col_start + step] = gl_random_normal(RNGSTATE,0,stdev);
	}
	if (cloud_pattern[row_start + step][col_start + step] < EMPTY_VALUE * 0.98){
		cloud_pattern[row_start + step][col_start + step] = gl_random_normal(RNGSTATE,0,stdev);
	}


	//Filling in rest of pattern
	float D = 0;
	double delta = 0;
	for (int k = 0; k < max_num_recursions; k++){
		col_start = col_min;
		row_start = row_min;
		if (k <= 3){
			D = 1.9;
		} else {
			D = 1.33;
		}

		if (k==0){
			delta = SIGMA * pow(0.5,(0.5 * (2-D)));
		} else {
			delta = delta * pow(0.5,(0.5 * (2-D)));
		}

		stdev = delta*delta;
		int end = pow(2.0,(double)(k));
		for (int i = 0; i < end; i++){
			for (int j = 0; j < end; j++){
				//	- - -> inc col
				//
				//	|	c1		e_b		c2
				//	|
				//	v	e_a		x		e_c
				// inc
				// row	c3		e_d		c4
				double c1 = cloud_pattern[row_start][col_start];
				double c2 = cloud_pattern[row_start][col_start + step];
				double c3 = cloud_pattern[row_start + step][col_start];
				double c4 = cloud_pattern[row_start + step][col_start + step];
				double x = (c1 + c2 + c3 + c4)/4 + gl_random_normal(RNGSTATE,0,stdev);
				double e_a = (x + c1 + c3)/3 + gl_random_normal(RNGSTATE,0, stdev);
				double e_b = (x + c1 + c2)/3 + gl_random_normal(RNGSTATE,0, stdev);
				double e_c = (x + c2 + c4)/3 + gl_random_normal(RNGSTATE,0, stdev);
				double e_d = (x + c3 + c4)/3 + gl_random_normal(RNGSTATE,0, stdev);


				if (cloud_pattern[row_start + half_step][col_start + half_step] < EMPTY_VALUE * 0.98){
					cloud_pattern[row_start + half_step][col_start + half_step] = x;
				}
				if (cloud_pattern[row_start + half_step][col_start] < EMPTY_VALUE * 0.98){
					cloud_pattern[row_start + half_step][col_start] = e_a;
				}
				if (cloud_pattern[row_start][col_start + half_step] < EMPTY_VALUE * 0.98){
					cloud_pattern[row_start][col_start + half_step] = e_b;
				}
				if (cloud_pattern[row_start + half_step][col_start + step] < EMPTY_VALUE * 0.98){
					cloud_pattern[row_start + half_step][col_start + step] = e_c;
				}
				if (cloud_pattern[row_start + step][col_start + half_step] < EMPTY_VALUE * 0.98){
					cloud_pattern[row_start + step][col_start + half_step] = e_d;
				}
				row_start = row_start + step;
			}
			row_start = row_min;
			col_start = col_start + step;
		}
		step = half_step;
		half_step = half_step/2;
	}
}


int climate::calc_cloud_pattern_size(std::vector< std::vector<double> > &location_list){
	double lat_max = location_list[0][0];
	double lat_min = location_list[0][0];
	double long_max = location_list[0][1];
	double long_min = location_list[0][1];
	double lat_delta = 0;
	double long_delta = 0;
	double degree_range_lat = 0;
	double degree_range_lon = 0;
	int num_tile_edge = 1; //Minimum size (1 on-screen tile with 1 off-screen on all sides; 9 tiles total).
	if (location_list.size() > 1) {
		for (int i=1; i<location_list.size(); i++) {
			lat_max = std::max(lat_max,location_list[i][0]);
			lat_min = std::min(lat_min,location_list[i][0]);
			long_max = std::max(long_max,location_list[i][1]);
			long_min = std::min(long_min,location_list[i][1]);
		}
		lat_delta = fabs(lat_max - lat_min);
		long_delta = fabs(long_max - long_min);
		double x_size_km = lat_delta * KM_PER_DEG; //111.32 km/degree latitude
		double y_size_km = long_delta * cos(RAD(lat_min)) * KM_PER_DEG; //Assumes the lat_delta is small enough that  the polar
															//convergence of the longitudinal lines is insignificant.
		double max_dim_km = std::max(x_size_km, y_size_km);
		num_tile_edge = ceil(((max_dim_km * 1000. / (double)PIXEL_EDGE_SIZE) / (double)CLOUD_TILE_SIZE));
	}
	MIN_LAT_INDEX = CLOUD_TILE_SIZE; // because there is always a buffer
	MAX_LAT_INDEX = CLOUD_TILE_SIZE + num_tile_edge * CLOUD_TILE_SIZE; // buffer plus the number of pixels
	MIN_LON_INDEX = MIN_LAT_INDEX; // square
	MAX_LON_INDEX = MAX_LAT_INDEX; // still square
//	gl_output("degree_range lat: %f", degree_range_lat);
//	gl_output("degree_range lon: %f", degree_range_lon);
//	gl_output("lat_delta: %f", lat_delta);
//	gl_output("long_delta: %f", long_delta);
	degree_range_lat = ((double)(num_tile_edge * CLOUD_TILE_SIZE * PIXEL_EDGE_SIZE))/1000./KM_PER_DEG;
	degree_range_lon = ((double)(num_tile_edge * CLOUD_TILE_SIZE * PIXEL_EDGE_SIZE))/1000./KM_PER_DEG/cos(RAD(lat_min));
	MIN_LAT = lat_min - (degree_range_lat-lat_delta)/2.;
	MAX_LAT = MIN_LAT + degree_range_lat;
	MIN_LON = long_min - (degree_range_lon-long_delta)/2.;
	MAX_LON = MIN_LON + degree_range_lon;
//	gl_output("lat min: %i=%f\tlat max: %i=%f", MIN_LAT_INDEX, MIN_LAT, MAX_LAT_INDEX, MAX_LAT);
//	gl_output("lon min: %i=%f\tlon max: %i=%f", MIN_LON_INDEX, MIN_LON, MAX_LON_INDEX, MAX_LON);
//	gl_output(" ");
	return num_tile_edge + 2; //Adding extra tiles for off-screen buffer around perimeter;
}

void climate::update_forecasts(TIMESTAMP t0)
{
	static const int Nh = 72; /* number of hours in forecast */
	static const int dt = 3600; /* number of seconds in forecast interval */

#if 0
	FORECAST *fc;
	
	for ( fc=get_forecast() ; fc!=NULL ; fc=fc->next )
	{
		/* don't update forecasts that are already current */
		if ( t0/dt==(fc->starttime/dt) ) continue;

		int h, hoy;
		double t[Nh];
		for ( h=0 ; h<Nh ; h++ )
		{
			if (h==0)
			{
				/* actual values */
				DATETIME ts;
				if ( !gl_localtime(t0+(h*dt),&ts) )
				{
					GL_THROW("climate::sync -- unable to resolve localtime!");
				}
				int doy = sa->day_of_yr(ts.month,ts.day);
				hoy = (doy - 1) * 24 + (ts.hour);
			}
			else if ( hoy==8760 )
				hoy = 0;
			else
				hoy++;

			/* this is an extremely naive model of forecast error */
			t[h] = tmy[hoy].temp + gl_random_normal(RNGSTATE,0,h/10.0);
		}
		gl_forecast_save(fc,t0,dt,Nh,t);
#ifdef NEVER
		char buffer[1024];
		int len = sprintf(buffer,"%d",fc->starttime);
		for ( h=3; h<72; h+=3 )
			len += sprintf(buffer+len,",%.1f",fc->values[h]);
		printf("%s\n",buffer);
#endif
	}
#endif
}


TIMESTAMP climate::presync(TIMESTAMP t0) /* called in presync */
{
	TIMESTAMP csv_rv = 0;
	TIMESTAMP tmy_rv = 0;
	TIMESTAMP cloud_rv = 0;
	DATETIME dt;
    // establish the current time
    update_time = t0;
    // %%%%% 20170224 MJB Stub in solar computation
    if(t0 > TS_ZERO && tmy==NULL && reader_type != RT_CSV) {
        gld_clock now(t0);
        //calculate the solar radiation
        OBJECT *obj=OBJECTHDR(this);
        double longitude = obj->longitude;
        double sol_time =
            sa->solar_time((double)now.get_hour()+(now.get_minute()/60.0)+(now.get_second()/3600.0)+(now.get_is_dst()
                        ? -1:0),now.get_yearday(),RAD(tz_meridian),longitude);
        gl_localtime(t0, &dt);
        short day_of_yr = sa->day_of_yr(dt.month,dt.day);
        solar_zenith = sa->zenith(day_of_yr, RAD(obj->latitude), sol_time);
        double sol_rad = 0.0;
        for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;
                c_point=COMPASS_PTS(c_point+1)) {
            if(c_point == CP_H) {
                sol_rad =
                    file.calc_solar(CP_E,now.get_yearday(),RAD(obj->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity,0.0);
            } else {
                sol_rad =
                    file.calc_solar(c_point,now.get_yearday(),RAD(obj->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity);
            }
            solar_flux[c_point] = sol_rad;
        }
    } 
	// TODO: need to read the cloud stuff from the csv file
	// changes appear to be limited to weather.h, weather.cpp, csv_reader.h, csv_reader.cpp
	if(t0 > TS_ZERO && reader_type == RT_CSV){
		gld_clock now(t0);
		csv_reader *cr = OBJECTDATA(reader,csv_reader);
		csv_rv = cr->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &solar_global, &global_horizontal_extra, &wind_speed,&wind_dir, &opq_sky_cov, &tot_sky_cov, &rainfall, &snowdepth, &pressure);
		// calculate the solar radiation
		double sol_time = sa->solar_time((double)now.get_hour()+now.get_minute()/60.0+now.get_second()/3600.0 + (now.get_is_dst() ? -1:0),now.get_yearday(),RAD(tz_meridian),RAD(reader->longitude));
		//std::cout << now.get_hour() << "," << now.get_minute() << "," << now.get_second() <<"," << now.get_is_dst() << "," << now.get_yearday() << "," << tz_meridian << "," << reader->longitude << std::endl;
		//std::cout << sol_time << std::endl;
		gl_localtime(t0, &dt);
		short day_of_yr = sa->day_of_yr(dt.month,dt.day);
		solar_zenith = sa->zenith(day_of_yr, RAD(reader->latitude), sol_time);
		double sol_rad = 0.0;


		for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
			if(c_point == CP_H)
				sol_rad = file.calc_solar(CP_E,now.get_yearday(),RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity,0.0);//(double)dnr * cos_incident + dhr;
			else
				sol_rad = file.calc_solar(c_point,now.get_yearday(),RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity);//(double)dnr * cos_incident + dhr;
			/* TMY2 solar radiation data is in Watt-hours per square meter. */
			solar_flux[c_point] = sol_rad;
		}
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
		//Shifts TMY3 data
		//One hour shift accounts for DST (if applicable)
		//One hour shift as TMY are summarized values for the preceding hour. To accurately calculate the solar time
		//    we need to start from the beginning of the hour and advance through it.
		gld_clock present(t0);
		if (!is_TMY2){
			if (present.get_is_dst())
				hoy = hoy - 2;
			else
				hoy = hoy - 1;
		}
		if (hoy < 0){ //Taking care of the wrap-around at the year boundary.
			hoy = hoy + 8760;
		}
		switch(interpolate){
			case CI_NONE:
				temperature = (tmy[hoy].temp);
				humidity = (tmy[hoy].rh);
				solar_direct = (tmy[hoy].dnr);
				solar_diffuse = (tmy[hoy].dhr);
				solar_global = (tmy[hoy].ghr);
				wind_speed = (tmy[hoy].windspeed);
				rainfall = (tmy[hoy].rainfall);
				snowdepth = (tmy[hoy].snowdepth);
				temperature_raw = (tmy[hoy].temp_raw);
				solar_azimuth = (tmy[hoy].solar_azimuth);
				solar_elevation = (tmy[hoy].solar_elevation);
				solar_zenith = (tmy[hoy].solar_zenith);
				solar_raw = (tmy[hoy].solar_raw);
				pressure = tmy[hoy].pressure;
				direct_normal_extra = tmy[hoy].direct_normal_extra;
				global_horizontal_extra = tmy[hoy].global_horizontal_extra;
				wind_dir = tmy[hoy].wind_dir;
				tot_sky_cov = tmy[hoy].tot_sky_cov;
				opq_sky_cov = tmy[hoy].opq_sky_cov;
				if ( memcmp(solar_flux,tmy[hoy].solar,CP_LAST*sizeof(double))!=0 )
					memcpy(solar_flux,tmy[hoy].solar,sizeof(solar_flux));
				break;
			case CI_LINEAR:
				now = hoy+ts.minute/60.0;
				hoy0 = hoy;
				hoy1 = hoy+1.0;
				temperature = (gl_lerp(now, hoy0, tmy[hoy].temp, hoy1, tmy[hoy+1%8760].temp));
				humidity = (gl_lerp(now, hoy0, tmy[hoy].rh, hoy1, tmy[hoy+1%8760].rh));
				solar_direct = (gl_lerp(now, hoy0, tmy[hoy].dnr, hoy1, tmy[hoy+1%8760].dnr));
				solar_diffuse = (gl_lerp(now, hoy0, tmy[hoy].dhr, hoy1, tmy[hoy+1%8760].dhr));
				solar_global = (gl_lerp(now, hoy0, tmy[hoy].ghr, hoy1, tmy[hoy+1%8760].ghr));
				wind_speed = (gl_lerp(now, hoy0, tmy[hoy].windspeed, hoy1, tmy[hoy+1%8760].windspeed));
				rainfall = (gl_lerp(now, hoy0, tmy[hoy].rainfall, hoy1, tmy[hoy+1%8760].rainfall));
				snowdepth = (gl_lerp(now, hoy0, tmy[hoy].snowdepth, hoy1, tmy[hoy+1%8760].snowdepth));
				solar_azimuth = (gl_lerp(now, hoy0, tmy[hoy].solar_azimuth, hoy1, tmy[hoy+1%8760].solar_azimuth));
				solar_elevation = (gl_lerp(now, hoy0, tmy[hoy].solar_elevation, hoy1, tmy[hoy+1%8760].solar_elevation));
				solar_zenith = (gl_lerp(now, hoy0, tmy[hoy].solar_zenith, hoy1, tmy[hoy+1%8760].solar_zenith));
				temperature_raw = (gl_lerp(now, hoy0, tmy[hoy].temp_raw, hoy1, tmy[hoy+1%8760].temp_raw));
				solar_raw = (gl_lerp(now, hoy0, tmy[hoy].solar_raw, hoy1, tmy[hoy+1%8760].solar_raw));
				pressure = gl_lerp(now, hoy0, tmy[hoy].pressure, hoy1, tmy[hoy+1%8760].pressure);
				direct_normal_extra = gl_lerp(now, hoy0, tmy[hoy].direct_normal_extra, hoy1, tmy[hoy+1%8760].direct_normal_extra);
				global_horizontal_extra = gl_lerp(now, hoy0, tmy[hoy].global_horizontal_extra, hoy1, tmy[hoy+1%8760].global_horizontal_extra);
				wind_dir = gl_lerp(now, hoy0, tmy[hoy].wind_dir, hoy1, tmy[hoy+1%8760].wind_dir);
				tot_sky_cov = gl_lerp(now, hoy0, tmy[hoy].tot_sky_cov, hoy1, tmy[hoy+1%8760].tot_sky_cov);
				opq_sky_cov = gl_lerp(now, hoy0, tmy[hoy].opq_sky_cov, hoy1, tmy[hoy+1%8760].opq_sky_cov);
				for ( int pt = 0 ; pt < CP_LAST ; ++pt )
				{
					solar_flux[pt] = gl_lerp(now, hoy0, tmy[hoy].solar[pt], hoy1, tmy[hoy+1%8760].solar[pt]);
				}
				break;
			case CI_QUADRATIC:
				now = hoy+ts.minute/60.0;
				hoy0 = hoy;
				hoy1 = hoy+1.0;
				hoy2 = hoy+2.0;
				temperature = (gl_qerp(now, hoy0, tmy[hoy].temp, hoy1, tmy[hoy+1%8760].temp, hoy2, tmy[hoy+2%8760].temp));
				humidity = (gl_qerp(now, hoy0, tmy[hoy].rh, hoy1, tmy[hoy+1%8760].rh, hoy2, tmy[hoy+2%8760].rh));
				if(humidity < 0.0){
					humidity = 0.0;
					gl_verbose("Setting humidity to zero. Quadratic interpolation caused the humidity to drop below zero.");
				}
				solar_direct = (gl_qerp(now, hoy0, tmy[hoy].dnr, hoy1, tmy[hoy+1%8760].dnr, hoy2, tmy[hoy+2%8760].dnr));
				if(solar_direct < 0.0){
					solar_direct = 0.0;
					gl_verbose("Setting solar_direct to zero. Quadratic interpolation caused the solar_direct to drop below zero.");
				}
				solar_diffuse = (gl_qerp(now, hoy0, tmy[hoy].dhr, hoy1, tmy[hoy+1%8760].dhr, hoy2, tmy[hoy+2%8760].dhr));
				if(solar_diffuse < 0.0){
					solar_diffuse = 0.0;
					gl_verbose("Setting solar_diffuse to zero. Quadratic interpolation caused the solar_diffuse to drop below zero.");
				}
				solar_global = (gl_qerp(now, hoy0, tmy[hoy].ghr, hoy1, tmy[hoy+1%8760].ghr, hoy2, tmy[hoy+2%8760].ghr));
				if(solar_global < 0.0){
					solar_global = 0.0;
					gl_verbose("Setting solar_global to zero. Quadratic interpolation caused the solar_global to drop below zero.");
				}
				wind_speed = (gl_qerp(now, hoy0, tmy[hoy].windspeed, hoy1, tmy[hoy+1%8760].windspeed, hoy2, tmy[hoy+2%8760].windspeed));
				if(wind_speed < 0.0){
					wind_speed = 0.0;
					gl_verbose("Setting wind_speed to zero. Quadratic interpolation caused the wind_speed to drop below zero.");
				}
				rainfall = (gl_qerp(now, hoy0, tmy[hoy].rainfall, hoy1, tmy[hoy+1%8760].rainfall, hoy2, tmy[hoy+2%8760].rainfall));
				if(rainfall < 0.0){
					rainfall = 0.0;
					gl_verbose("Setting rainfall to zero. Quadratic interpolation caused the rainfall to drop below zero.");
				}
				snowdepth = (gl_qerp(now, hoy0, tmy[hoy].snowdepth, hoy1, tmy[hoy+1%8760].snowdepth, hoy2, tmy[hoy+2%8760].snowdepth));
				if(snowdepth < 0.0){
					snowdepth = 0.0;
					gl_verbose("Setting snowdepth to zero. Quadratic interpolation caused the snowdepth to drop below zero.");
				}
				solar_azimuth = (gl_qerp(now, hoy0, tmy[hoy].solar_azimuth, hoy1, tmy[hoy+1%8760].solar_azimuth, hoy2, tmy[hoy+2%8760].solar_azimuth));
				solar_elevation = (gl_qerp(now, hoy0, tmy[hoy].solar_elevation, hoy1, tmy[hoy+1%8760].solar_elevation, hoy2, tmy[hoy+2%8760].solar_elevation));
				solar_zenith = (gl_qerp(now, hoy0, tmy[hoy].solar_zenith, hoy1, tmy[hoy+1%8760].solar_zenith, hoy2, tmy[hoy+2%8760].solar_zenith));
				temperature_raw = (gl_qerp(now, hoy0, tmy[hoy].temp_raw, hoy1, tmy[hoy+1%8760].temp_raw, hoy2, tmy[hoy+2%8760].temp_raw));
				solar_raw = (gl_qerp(now, hoy0, tmy[hoy].solar_raw, hoy1, tmy[hoy+1%8760].solar_raw, hoy2, tmy[hoy+2%8760].solar_raw));
				if(solar_raw < 0.0){
					solar_raw = 0.0;
					gl_verbose("Setting solar_raw to zero. Quadratic interpolation caused the solar_raw to drop below zero.");
				}
				pressure = gl_qerp(now, hoy0, tmy[hoy].pressure, hoy1, tmy[hoy+1%8760].pressure, hoy2, tmy[hoy+2%8760].pressure);
				if(pressure < 0.0){
					pressure = 0.0;
					gl_verbose("Setting pressure to zero. Quadratic interpolation caused the pressure to drop below zero.");
				}
				direct_normal_extra = gl_qerp(now, hoy0, tmy[hoy].direct_normal_extra, hoy1, tmy[hoy+1%8760].direct_normal_extra, hoy2, tmy[hoy+2%8760].direct_normal_extra);
				if(direct_normal_extra < 0.0){
					direct_normal_extra = 0.0;
					gl_verbose("Setting extraterrestrial_direct_normal to zero. Quadratic interpolation caused the extraterrestrial_direct_normal to drop below zero.");
				}
				global_horizontal_extra = gl_qerp(now, hoy0, tmy[hoy].global_horizontal_extra, hoy1, tmy[hoy+1%8760].global_horizontal_extra, hoy2, tmy[hoy+2%8760].global_horizontal_extra);
				if(global_horizontal_extra < 0.0){
					global_horizontal_extra = 0.0;
					gl_verbose("Setting global_horizontal_extra to zero. Quadratic interpolation caused the global_horizontal_extra to drop below zero.");
				}
				wind_dir = gl_qerp(now, hoy0, tmy[hoy].wind_dir, hoy1, tmy[hoy+1%8760].wind_dir, hoy2, tmy[hoy+2%8760].wind_dir);
				if(wind_dir < 0.0){
					wind_dir = 360.0+wind_dir;
					gl_verbose("Setting wind_dir to 360+wind_dir. Quadratic interpolation caused the wind_dir to drop below zero.");
				}
				if(wind_dir > 360.0){
					wind_dir = wind_dir-360.0;
					gl_verbose("Setting wind_dir to wind_dir-360. Quadratic interpolation caused the wind_dir to rise above 360.");
				}
				tot_sky_cov = gl_qerp(now, hoy0, tmy[hoy].tot_sky_cov, hoy1, tmy[hoy+1%8760].tot_sky_cov, hoy2, tmy[hoy+2%8760].tot_sky_cov);
				if(tot_sky_cov < 0.0){
					tot_sky_cov = 0.0;
					gl_verbose("Setting tot_sky_cov to zero. Quadratic interpolation caused the tot_sky_cov to drop below zero.");
				}
				opq_sky_cov = gl_qerp(now, hoy0, tmy[hoy].opq_sky_cov, hoy1, tmy[hoy+1%8760].opq_sky_cov, hoy2, tmy[hoy+2%8760].opq_sky_cov);
				if(opq_sky_cov < 0.0){
					opq_sky_cov = 0.0;
					gl_verbose("Setting opq_sky_cov to zero. Quadratic interpolation caused the opq_sky_cov to drop below zero.");
				}

				for ( int pt = 0; pt < CP_LAST; ++pt )
				{
					if ( tmy[hoy].solar[pt] == tmy[hoy+1].solar[pt])
					{
						solar_flux[pt] = tmy[hoy].solar[pt];
					} else {
						solar_flux[pt] = gl_qerp(now, hoy0, tmy[hoy].solar[pt], hoy1, tmy[hoy+1%8760].solar[pt], hoy2, tmy[hoy+2%8760].solar[pt]);
						if(solar_flux[pt] < 0.0)
							solar_flux[pt] = 0.0; /* quadratic isn't always cooperative... */
					}
				}
				break;
			default:
				GL_THROW("climate::sync -- unrecognized interpolation mode!");
		}
		update_forecasts(t0);
		tmy_rv = -(t0+(3600*TS_SECOND-t0%(3600 *TS_SECOND))); /// negative means soft event
	}
	if (cloud_model == CM_CUMULUS) {
		if (prev_NTime != t0 ){
			double p = pressure*0.1; // in millibars, convert to kPa
			double Z = solar_zenith;
			double M = 0.02857*sqrt((1224. * cos(Z) * cos(Z)) + 1.);
			double u = 4.5; // generally in range of 3 to 4.5
			double aw = 0.077*pow(u/M,0.3);
			double Pa = cloud_aerosol_transmissivity; // e^-(alpha*M)
			double PRPA = 1.041-(0.15*sqrt(((p*0.00949)+0.051)/M));
			global_transmissivity = std::max(0.0,(PRPA-aw)*Pa);
			update_cloud_pattern(t0 - prev_NTime);
			//write_out_cloud_pattern('F');
			prev_NTime = t0;
		}

		cloud_rv = t0 + 60;
	}

	//Extra logic to return the correct timestamp based on the weather data source and the use of the cloud model.
	if (t0 <= TS_ZERO)
		return TS_NEVER;
	else if (cloud_model == CM_NONE)
		if (reader_type == RT_CSV)
			return csv_rv;
		else if (tmy!=NULL)
			return tmy_rv;
		else
			return TS_NEVER;
	else
		if (reader_type == RT_CSV)
			if (cloud_rv <= fabs(csv_rv))
				return cloud_rv;
			else
				return 	csv_rv;
		else
			if (cloud_rv <= fabs(tmy_rv))
				return cloud_rv;
			else
				return tmy_rv;


}

/**@}**/


