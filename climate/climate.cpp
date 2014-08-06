/** $Id: climate.cpp 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file climate.cpp
	@author David P. Chassin
 **/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#include "gridlabd.h"
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

//sjin: add solar elevation wrapper funcions
EXPORT int64 calculate_solar_elevation(OBJECT *obj, double latitude, double *value)
{
	static SolarAngles sa; // just for the functions
	double std_time = 0.0;
	double solar_time = 0.0;
	short int doy = 0;
	DATETIME dt;

	climate *cli;
	if (obj == 0 || value == 0){
		//throw "climate/calc_solar: null object pointer in arguement";
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		//throw "climate/calc_solar: input object is not a climate object";
		return 0;
	}
	
	gl_localtime(obj->clock, &dt);
	std_time = (double)(dt.hour) + ((double)dt.minute)/60.0 + (dt.is_dst ? -1:0);
	solar_time = sa.solar_time(std_time, doy, RAD(cli->get_tz_meridian()), RAD(obj->longitude));

	double hr_ang = -(15.0 * PI_OVER_180)*(solar_time-12.0); // morning +, afternoon -

    double decl = 0.409280*sin(2.0*PI*(284+doy)/365);

	*value = asin(sin(decl)*sin(latitude) + cos(decl)*cos(latitude)*cos(hr_ang));

    return 1;
}

//sjin: add solar azimuth wrapper funcions
EXPORT int64 calculate_solar_azimuth(OBJECT *obj, double latitude, double *value)
{
	static SolarAngles sa; // just for the functions
	double std_time = 0.0;
	double solar_time = 0.0;
	short int doy = 0;
	DATETIME dt;

	climate *cli;
	if (obj == 0 || value == 0){
		//throw "climate/calc_solar: null object pointer in arguement";
		return 0;
	}
	cli = OBJECTDATA(obj, climate);
	if(gl_object_isa(obj, "climate", "climate") == 0){
		//throw "climate/calc_solar: input object is not a climate object";
		return 0;
	}

	gl_localtime(obj->clock, &dt);
	std_time = (double)(dt.hour) + ((double)dt.minute)/60.0 + (dt.is_dst ? -1:0);
	solar_time = sa.solar_time(std_time, doy, RAD(cli->get_tz_meridian()), RAD(obj->longitude));

	double hr_ang = -(15.0 * PI_OVER_180)*(solar_time-12.0); // morning +, afternoon -

    double decl = 0.409280*sin(2.0*PI*(284+doy)/365);

	double alpha = (90.0 * PI_OVER_180) - latitude + decl;

	*value = acos( (sin(decl)*cos(latitude) - cos(decl)*sin(latitude)*cos(hr_ang))/cos(alpha) );

    return 1;
}

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
	ghr = cli->get_solar_global();
	dhr = cli->get_solar_diffuse();
	dnr = shading_value * cli->get_solar_direct();

	gl_localtime(obj->clock, &dt);

	std_time = (double)(dt.hour) + ((double)dt.minute)/60.0  + (dt.is_dst ? -1.0:0.0);

	doy=sa.day_of_yr(dt.month,dt.day);

	solar_time = sa.solar_time(std_time, doy, RAD(cli->get_tz_meridian()), RAD(obj->longitude));
	cos_incident = sa.cos_incident(RAD(obj->latitude), tilt, orientation, solar_time, doy);

	*value = dnr * cos_incident + dhr * (1 + cos(tilt)) / 2 + ghr * (1 - cos(tilt)) * cli->get_ground_reflectivity() / 2;

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
	ghr = cli->get_solar_global();
	dhr = cli->get_solar_diffuse();
	dnr = cli->get_solar_direct();

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
	*value = (shading_value*dnr*cos_incident) + dhr*sa.solpos_vals.perez_horz + ghr*((1 - cos(tilt))*cli->get_ground_reflectivity()/2.0);

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

		sscan_rv = sscanf(buf,"%*s %75s %3s %d %c %d %d %c %d %d %d",data_city,data_state,&tz_offset,temp_lat_hem,&lat_degrees,&lat_minutes,temp_long_hem,&long_degrees,&long_minutes,&elevation);

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

int tmy2_reader::read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind, double *precip, double *snowDepth, double *pressure, double *extra_terr_dni){
	int rct = 0;
	int tmp_dnr, tmp_dhr, tmp_tdb, tmp_rh, tmp_ws, tmp_precip, tmp_sf, tmp_ghr, tmp_extra_dni, tmp_press;
	//sscanf(buf, "%*2s%2d%2d%2d%*14s%4d%*2s%4d%*40s%4d%8*s%3d%*s",month,day,hour,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh);
	int tmh, tday, thr;
	if(month == NULL) month = &tmh;
	if(day == NULL) day = &tday;
	if(hour == NULL) hour = &thr;
	rct = sscanf(buf, "%*2s%2d%2d%2d%*4s%4d%4d%*2s%4d%*2s%4d%*34s%4d%*8s%3d%*2s%4d%*7s%3d%*25s%3d%*7s%3d",month,day,hour,&tmp_extra_dni,&tmp_ghr,&tmp_dnr,&tmp_dhr,&tmp_tdb,&tmp_rh,&tmp_press,&tmp_ws,&tmp_precip,&tmp_sf);
				/* 3__5__7__9__13__17_20_23_27__29_33___67_71_79__82__85__95_98_ */
	if(rct != 13){
		gl_warning("TMY reader did not get 13 values for line time %d/%d %d00", *month, *day, *hour);
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
			PT_char32, "city", PADDR(city),
			PT_char1024,"tmyfile",PADDR(tmyfile),
			PT_double,"temperature[degF]",PADDR(temperature),
			PT_double,"humidity[%]",PADDR(humidity),
			PT_double,"solar_flux[W/sf]",PADDR(solar_flux),	PT_SIZE, 9,
			PT_double,"solar_direct[W/sf]",PADDR(solar_direct),
			PT_double,"solar_diffuse[W/sf]",PADDR(solar_diffuse),
			PT_double,"solar_global[W/sf]",PADDR(solar_global),
			PT_double,"extraterrestrial_direct_normal[W/sf]",PADDR(direct_normal_extra),
			PT_double,"pressure[mbar]",PADDR(pressure),
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
			NULL)<1) GL_THROW("unable to publish properties in %s",__FILE__);
		memset(this,0,sizeof(climate));
		strcpy(city,"");
		strcpy(tmyfile,"");
		temperature = 59.0;
		temperature_raw = 15.0;
		humidity = 75.0;
		rainfall = 0.0;
		snowdepth = 0.0;
		ground_reflectivity = 0.3;
		direct_normal_extra = 126.998456;	//1367 W/m^2 constant in W/ft^2
		pressure = 1000;	//Sea level assumption
		//solar_flux = malloc(8 * sizeof(double));
		solar_flux[0] = solar_flux[1] = solar_flux[2] = solar_flux[3] = solar_flux[4] = solar_flux[5] = solar_flux[6] = solar_flux[7] = solar_flux[8] = 0.0; // W/sf normal
		//solar_flux_S = solar_flux_SE = solar_flux_SW = solar_flux_E = solar_flux_W = solar_flux_NE = solar_flux_NW = solar_flux_N = 0.0; // W/sf normal
		tmy = NULL;
		sa = new SolarAngles();
		defaults = this;
		gl_publish_function(oclass,	"calculate_solar_radiation_degrees", (FUNCTIONADDR)calculate_solar_radiation_degrees);
		gl_publish_function(oclass,	"calculate_solar_radiation_radians", (FUNCTIONADDR)calculate_solar_radiation_radians);
		gl_publish_function(oclass,	"calculate_solar_radiation_shading_degrees", (FUNCTIONADDR)calculate_solar_radiation_shading_degrees);
		gl_publish_function(oclass,	"calculate_solar_radiation_shading_radians", (FUNCTIONADDR)calculate_solar_radiation_shading_radians);
		//sjin: publish solar elevation and azimuth functions
		gl_publish_function(oclass, "calculate_solar_elevation", (FUNCTIONADDR)calculate_solar_elevation);
		gl_publish_function(oclass, "calculate_solar_azimuth", (FUNCTIONADDR)calculate_solar_azimuth);

		//New solar position algorithm stuff
		gl_publish_function(oclass,	"calc_solpos_radiation_shading_degrees", (FUNCTIONADDR)calc_solar_solpos_shading_deg);
		gl_publish_function(oclass,	"calc_solpos_radiation_shading_radians", (FUNCTIONADDR)calc_solar_solpos_shading_rad);
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
		int rv = 0;

		if(reader == NULL){
			csv_reader *creader = new csv_reader();
			reader_hndl = creader;
			rv = creader->open(found_file);
//			creader->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &wind_speed, &rainfall, &snowdepth);
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
	double dnr,dhr,ghr,wspeed,precip,snowdepth,pressure,extra_dni;
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

		file.read_data(&dnr,&dhr,&ghr,&temperature,&humidity,&month,&day,&hour,&wspeed,&precip,&snowdepth,&pressure,&extra_dni);

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
			
			// calculate the solar radiation - hour on here may need a -1 application (hour-1) - unsure how TMYs really code things
			double sol_time = sa->solar_time((double)hour,doy,RAD(tz_meridian),RAD(get_longitude()));
			double sol_rad = 0.0;

			tmy[hoy].solar_elevation = sa->altitude(doy, RAD(obj->latitude), sol_time);
			tmy[hoy].solar_azimuth = sa->azimuth(doy, RAD(obj->latitude), sol_time);

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
	TIMESTAMP rv = 0;

	if(t0 > TS_ZERO && reader_type == RT_CSV){
		gld_clock now(t0);
		csv_reader *cr = OBJECTDATA(reader,csv_reader);
		rv = cr->get_data(t0, &temperature, &humidity, &solar_direct, &solar_diffuse, &solar_global, &wind_speed, &rainfall, &snowdepth, &pressure);
		// calculate the solar radiation
		double sol_time = sa->solar_time((double)now.get_hour()+now.get_minute()/60.0+now.get_second()/3600.0 + (now.get_is_dst() ? -1:0),now.get_yearday(),RAD(tz_meridian),RAD(reader->longitude));
		double sol_rad = 0.0;

		for(COMPASS_PTS c_point = CP_H; c_point < CP_LAST;c_point=COMPASS_PTS(c_point+1)){
			if(c_point == CP_H)
				sol_rad = file.calc_solar(CP_E,now.get_yearday(),RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity,0.0);//(double)dnr * cos_incident + dhr;
			else
				sol_rad = file.calc_solar(c_point,now.get_yearday(),RAD(reader->latitude),sol_time,solar_direct,solar_diffuse,solar_global,ground_reflectivity);//(double)dnr * cos_incident + dhr;
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
				solar_raw = (tmy[hoy].solar_raw);
				pressure = tmy[hoy].pressure;
				direct_normal_extra = tmy[hoy].direct_normal_extra;
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
				temperature_raw = (gl_lerp(now, hoy0, tmy[hoy].temp_raw, hoy1, tmy[hoy+1%8760].temp_raw));
				solar_raw = (gl_lerp(now, hoy0, tmy[hoy].solar_raw, hoy1, tmy[hoy+1%8760].solar_raw));
				pressure = gl_lerp(now, hoy0, tmy[hoy].pressure, hoy1, tmy[hoy+1%8760].pressure);
				direct_normal_extra = gl_lerp(now, hoy0, tmy[hoy].direct_normal_extra, hoy1, tmy[hoy+1%8760].direct_normal_extra);
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
				GL_THROW("climate::sync -- unrecognize interpolation mode!");
		}
		update_forecasts(t0);
		return -(t0+(3600*TS_SECOND-t0%(3600 *TS_SECOND))); /// negative means soft event
	}
	return TS_NEVER;
}

/**@}**/


