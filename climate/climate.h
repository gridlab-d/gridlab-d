/** $Id: climate.h 1182 2008-12-22 22:08:36Z dchassin $
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
} CLIMATE_INTERPOLATE;



typedef struct s_tmy {
	double temp; // F
	double temp_raw; // C
	double rh; // %rh
	double dnr;
	double dhr;
	double ghr;
	double solar[CP_LAST]; // W/sf
	double solar_raw;
	double windspeed;
	double rainfall; // in/h
	double snowdepth; // in
} TMYDATA;

/* published functions */
EXPORT int64 calculate_solar_radiation_degrees(OBJECT *obj, double tilt, double orientation, double *value);
EXPORT int64 calculate_solar_radiation_radians(OBJECT *obj, double tilt, double orientation, double *value);

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
	 */
	int read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind=0,double *precip=0, double *snowDepth=0);

	/** obtain records **/

	double calc_solar(COMPASS_PTS cpt, short doy, double lat, double sol_time, double dnr, double dhr, double ghr, double gnd_ref, double vert_angle);

};



class climate {
public:
	char32 city; ///< the city
	char1024 tmyfile; ///< the TMY file name
	OBJECT *reader;
	double temperature; ///< the temperature (degF)
	double temperature_raw; ///< the temperature (degC)
	double humidity; ///< the relative humidity (%)
	double solar_flux[CP_LAST]; ///< Solar flux array (W/sf) Elements are in order: [S, SE, SW, E, W, NE, NW, N, H]
	double solar_direct;
	double solar_diffuse;
	double solar_global;
	double solar_raw;
	double wind_speed; ///< wind speed (m/s)
	double wind_dir; ///< wind direction (0-360)
	double wind_gust; ///< wind gusts (m/s)
	double rainfall; // in/h
	double snowdepth; // in
	double ground_reflectivity; // %
	struct {
		double low;
		double low_day;
		double high;
		double high_day;
		double solar;
	} record;
	CLIMATE_INTERPOLATE interpolate;
	// add some of the init() vars that are useful to capture
	double tz_meridian;
private:
	enum READERTYPE {
		RT_NONE,
		RT_TMY2,
		RT_CSV,
	}  reader_type;
	SolarAngles *sa;
	tmy2_reader file;
	weather_reader *reader_hndl;
	TMYDATA *tmy;
public:
	static CLASS *oclass;
	static climate *defaults;
public:
	climate(MODULE *module);
	int create(void);
	int init(OBJECT *parent);
	int isa(char *classname);
	TIMESTAMP sync(TIMESTAMP t0);
}; ///< climate data 

#endif

/**@}*/
