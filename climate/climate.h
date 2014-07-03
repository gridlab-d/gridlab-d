/** $Id: climate.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
	@file climate.h
	@addtogroup climate
	@ingroup modules
 @{
 **/

#ifndef _CLIMATE_H
#define _CLIMATE_H

#include <stdarg.h>
#include "gridlabd.h"
#include "solar_angles.h"
#include "weather_reader.h"
#include "csv_reader.h"

typedef enum{
	CP_H    = 0,
    CP_N    = 1,
	CP_NE   = 2,
	CP_E    = 3,
	CP_SE   = 4,
	CP_S    = 5,
	CP_SW   = 6,
	CP_W    = 7,
	CP_NW   = 8,
	CP_LAST = 9
} COMPASS_PTS;

typedef enum{
	CI_NONE = 0,
	CI_LINEAR,
	CI_QUADRATIC
};



typedef struct s_tmy {
	double temp; // F
	double temp_raw; // C
	double rh; // %rh
	double dnr;
	double dhr;
	double ghr;
	double solar[CP_LAST]; // W/sf
	double solar_raw;
	double direct_normal_extra;
	double pressure;
	double windspeed;
	double rainfall; // in/h
	double snowdepth; // in
	double solar_azimuth;
	double solar_elevation;
} TMYDATA;

/* published functions */
EXPORT int64 calculate_solar_radiation_degrees(OBJECT *obj, double tilt, double orientation, double *value);
EXPORT int64 calculate_solar_radiation_radians(OBJECT *obj, double tilt, double orientation, double *value);
EXPORT int64 calculate_solar_radiation_shading_degrees(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calculate_solar_radiation_shading_radians(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calc_solar_solpos_shading_deg(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calc_solar_solpos_shading_rad(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
//sjin: add solar elevation and azimuth published funcions
EXPORT int64 calculate_solar_elevation(OBJECT *obj, double latitude, double *value);
EXPORT int64 calculate_solar_azimuth(OBJECT *obj, double latitude, double *value);

/**
 * This implements a Gridlab-D specific TMY2 data reader.  It was implemented
 * to pull specific information from the TMY2 raw format, including latitude
 * information contained in the TMY2 header.  Header information will be 
 * maintained for the lifetime of the reader, as it may be needed to populate
 * columns of the TMY2 structure for later use.  Leap years are handled by 
 * treating February 29th and March 1 as numerically equivalent.  IE on a
 * leap year, March 1 data is repeated for February 29th.
 */
class tmy2_reader{
private:
	// Header information
	char data_city[75];
	char data_state[3];
	int lat_degrees;
	int lat_minutes;
	int long_degrees;
	int long_minutes;

	double low_temp;
	double high_temp;
	double peak_solar;	

	FILE *fp;
	char buf[500]; // buffer to hold line data.

public:
	int tz_offset;
	tmy2_reader(){}

	int elevation;

	void close();

	/**
	 * Open the file for reading.  This will read in the header information
	 * and position the file reader at the first data line in the file.
	 *
	 * This call will throw an exception if the file fails to open
	 *
	 * @param file the name of the TMY2 file to open
	 */
	int open(const char *file);

	/**
	 * Store the current line in a buffer for later reading by read_data
	 */
	int next();

	/**
	 * Populate the given arguments with data from the tmy2 file header
	 *
	 * @param city
	 * @param state
	 * @param degrees latitude degrees
	 * @param minutes latitude minutes
	 * @param long_deg longitude degrees
	 * @param long_min longitude minutes
	 */
	int header_info(char* city, char* state, int* degrees, int* minutes, int* long_deg, int* long_min);

	/**
	 * Populate the given arguments with data from the buffer.
	 *
	 * @param dnr - Direct Normal Radiation
	 * @param dhr - Diffuse Horizontal Radiation
	 * @param rh Relative Humidity
	 * @param tdb - Dry Bulb Temperature
	 * @param month - Month of the observation
	 * @param day - Day of the observation
	 * @param hour - hour of the observation
	 * @param wind Wind speed (optional)
	 *
	 * @param pressure - atmospheric pressure
	 * @param extra_terr_dni - Extra terrestrial direct normal irradiance (top of atmosphere)
	 */
	int read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind=0,double *precip=0, double *snowDepth=0, double *pressure = 0, double *extra_terr_dni = 0);

	/** obtain records **/

	double calc_solar(COMPASS_PTS cpt, short doy, double lat, double sol_time, double dnr, double dhr, double ghr, double gnd_ref, double vert_angle);

};

typedef struct {
	double low;
	double low_day;
	double high;
	double high_day;
	double solar;
} CLIMATERECORD;

typedef	enum {
		RT_NONE,
		RT_TMY2,
		RT_CSV,
};

class climate : public gld_object {
	
	// get_/set_ accessors for classes in this module only (non-atomic data need locks on access)
	GL_STRING(char32,city); ///< the city
	GL_ATOMIC(double,temperature); ///< the temperature (degF)
	GL_ATOMIC(double,humidity); ///< the relative humidity (%)
	GL_ATOMIC(double,wind_speed); ///< wind speed (m/s)
	GL_ATOMIC(double,tz_meridian); ///< timezone meridian
	GL_ATOMIC(double,solar_global); ///< global solar flux (W/sf)
	GL_ATOMIC(double,solar_direct); ///< direct solar flux (W/sf)
	GL_ATOMIC(double,solar_diffuse); ///< diffuse solar flux (W/sf)
	GL_ATOMIC(double,ground_reflectivity); // flux reflectivity of ground (W/sf)
	GL_STRUCT(CLIMATERECORD,record); ///< record values (low,low_day,high,high_day,solar)
	GL_ATOMIC(double,rainfall); ///< rainfall rate (in/h)
	GL_ATOMIC(double,snowdepth); ///< snow accumulation (in)
	GL_STRING(char1024,forecast_spec); ///< forecasting model
	GL_STRING(char1024,tmyfile); ///< the TMY file name
	GL_ATOMIC(OBJECT*,reader); ///< the file reader to use when loading data
	GL_ATOMIC(double,temperature_raw); ///< the temperature (degC)
	GL_ARRAY(double,solar_flux,CP_LAST); ///< Solar flux array (W/sf) Elements are in order: [S, SE, SW, E, W, NE, NW, N, H]
	GL_ATOMIC(double,solar_raw);
	GL_ATOMIC(double,wind_dir);
	GL_ATOMIC(double,wind_gust);
	GL_ATOMIC(enumeration,interpolate);
	GL_ATOMIC(double,solar_elevation);
	GL_ATOMIC(double,solar_azimuth);
	GL_ATOMIC(double,direct_normal_extra);
	GL_ATOMIC(double,pressure);
	GL_ATOMIC(double,tz_offset_val);

	// data not shared with classes in this module (no locks needed)
private:
	SolarAngles *sa;
	tmy2_reader file;
	weather_reader *reader_hndl;
	TMYDATA *tmy;
public:
	enumeration reader_type;
	static CLASS *oclass;
	static climate *defaults;
public:
	void update_forecasts(TIMESTAMP t0);
public:
	climate(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP presync(TIMESTAMP t0);
	inline TIMESTAMP sync(TIMESTAMP t0) { return TS_NEVER; };
	inline TIMESTAMP postsync(TIMESTAMP t0) { return TS_NEVER; };
}; ///< climate data 

#endif

/**@}*/
