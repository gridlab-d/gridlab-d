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
#include <vector>

#include "gridlabd.h"
#include "solar_angles.h"
#include "weather_reader.h"
#include "csv_reader.h"


#ifdef _WIN32
#include <io.h>      // Provides _access and related macros
#define R_OK 4       // Define POSIX-like `READ` flag compatibility for Windows
#else
#include <unistd.h>  // For POSIX systems
#endif


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

enum{
	CI_NONE = 0,
	CI_LINEAR,
	CI_QUADRATIC
} CI;

#ifdef CM_NONE
#undef CM_NONE
#endif

enum{
	CM_NONE = 0,
	CM_CUMULUS = 1
} CLOUDMODEL;


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
	double solar_zenith;
	double global_horizontal_extra;
	double wind_dir;
	double tot_sky_cov;
	double opq_sky_cov;
} TMYDATA;

/* published functions */
EXPORT int64 calculate_solar_radiation_degrees(OBJECT *obj, double tilt, double orientation, double *value);
EXPORT int64 calculate_solar_radiation_radians(OBJECT *obj, double tilt, double orientation, double *value);
EXPORT int64 calculate_solar_radiation_shading_degrees(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calculate_solar_radiation_shading_position_radians(OBJECT *obj, double tilt, double orientation, double latitude, double longitude, double shading_value, double *value);
EXPORT int64 calculate_solar_radiation_shading_radians(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calc_solar_solpos_shading_deg(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calc_solar_solpos_shading_position_rad(OBJECT *obj, double tilt, double orientation, double latitude, double longitude, double shading_value, double *value);
EXPORT int64 calc_solar_solpos_shading_rad(OBJECT *obj, double tilt, double orientation, double shading_value, double *value);
EXPORT int64 calc_solar_ideal_shading_position_radians(OBJECT *obj, double tilt, double latitude, double longitude, double shading_value, double *value);

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
	//char buf[500]; // buffer to hold line data.

public:
	int tz_offset;
	char buf[500]; // buffer to hold line data.
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
	int read_data(double *dnr, double *dhr, double *ghr, double *tdb, double *rh, int* month, int* day, int* hour, double *wind=0, double *winddir=0, double *precip=0, double *snowDepth=0, double *pressure = 0, double *extra_terr_dni=0, double *extra_terr_ghi=0, double *tot_sky_cov=0, double *opq_sky_cov=0);

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

enum {
		RT_NONE,
		RT_TMY2,
		RT_CSV,
} RT;

class climate : public gld_object {
	
	// get_/set_ accessors for classes in this module only (non-atomic data need locks on access)
	//GL_STRING(char32,city); ///< the city
	//GL_ATOMIC(double,temperature); ///< the temperature (degF)
	//GL_ATOMIC(double,humidity); ///< the relative humidity (%)
	//GL_ATOMIC(double,wind_speed); ///< wind speed (mph)
	//GL_ATOMIC(double,tz_meridian); ///< timezone meridian
	//GL_ATOMIC(double,solar_global); ///< global solar flux (W/sf)
	//GL_ATOMIC(double,solar_direct); ///< direct solar flux (W/sf)
	//GL_ATOMIC(double,solar_diffuse); ///< diffuse solar flux (W/sf)
	//GL_ATOMIC(double,solar_cloud_global); //< READ ONLY: global solar flux after modification by the cloud model(W/sf)
	//GL_ATOMIC(double,solar_cloud_direct); ///< READ ONLY; direct solar flux after modification by the cloud modelW/sf)
	//GL_ATOMIC(double,solar_cloud_diffuse); ///< READ ONLY: diffuse solar flux after modification by the cloud model(W/sf)
	//GL_ATOMIC(double,cloud_alpha); //Determines the distance between the shading layers of the normalized patterns.
	//										//Smaller values lead to a larger distance between shading layers and greater variation within a cloud, closer to continuous.
	//										//Larger values lead to a smaller distance between shading layers and less variation within a cloud, closer to binary.
	//										//Minimum value is num_cloud_layers.
	//GL_ATOMIC(double,cloud_num_layers); //Higher number of layers makes for the possibility of wispier clouds.
	//GL_ATOMIC(double,cloud_aerosol_transmissivity); //Attenuation factor of no-cloud (clear-sky) radiation due to aerosols
	//GL_ATOMIC(double,ground_reflectivity); // flux reflectivity of ground (W/sf)
	//GL_STRUCT(CLIMATERECORD,record); ///< record values (low,low_day,high,high_day,solar)
	//GL_ATOMIC(double,rainfall); ///< rainfall rate (in/h)
	//GL_ATOMIC(double,snowdepth); ///< snow accumulation (in)
	//GL_STRING(char1024,forecast_spec); ///< forecasting model
	//GL_STRING(char1024,tmyfile); ///< the TMY file name
	//GL_ATOMIC(OBJECT*,reader); ///< the file reader to use when loading data
	//GL_ARRAY(double, solar_flux, CP_LAST); ///< Solar flux array (W/sf) Elements are in order: [S, SE, SW, E, W, NE, NW, N, H]
	/*GL_ATOMIC(double,temperature_raw); ///< the temperature (degC)
	GL_ATOMIC(double,solar_raw);
	GL_ATOMIC(double,wind_dir);
	GL_ATOMIC(double,wind_gust);
	GL_ATOMIC(enumeration, interpolate);
	GL_ATOMIC(double,solar_elevation);
	GL_ATOMIC(double,solar_azimuth);
	GL_ATOMIC(double,solar_zenith);
	GL_ATOMIC(double,direct_normal_extra);
	GL_ATOMIC(double,pressure);
	GL_ATOMIC(double,tz_offset_val);
	GL_ATOMIC(double,global_horizontal_extra);
	GL_ATOMIC(double,tot_sky_cov);
	GL_ATOMIC(double,opq_sky_cov);
	GL_ATOMIC(double,cloud_opacity);
	GL_ATOMIC(double,cloud_reflectivity);
	GL_ATOMIC(double,cloud_speed_factor);
	GL_ATOMIC(enumeration, cloud_model);
    GL_ATOMIC(double,update_time);*/


protected:
	char32 city; ///< the city
	double temperature; ///< the temperature (degF)
	double humidity; ///< the relative humidity (%)
	double wind_speed; ///< wind speed (mph)
	double solar_global; ///< global solar flux (W/sf)
	double solar_direct; ///< direct solar flux (W/sf)
	double tz_meridian; ///< timezone meridian
	double solar_diffuse; ///< diffuse solar flux (W/sf)
	double solar_cloud_global; ///< global solar flux after cloud modification (W/sf)
	double solar_cloud_direct; ///< direct solar flux after cloud modification (W/sf)
	double solar_cloud_diffuse; ///< diffuse solar flux after cloud modification (W/sf)
	double cloud_alpha; ///< cloud layer distance factor
	double cloud_num_layers; ///< number of cloud layers
	double cloud_aerosol_transmissivity; ///< clear-sky radiation attenuation factor due to aerosols
	double ground_reflectivity; ///< ground flux reflectivity (W/sf)
	CLIMATERECORD record; ///< record values (low, low_day, high, high_day, solar)
	double rainfall; ///< rainfall rate (in/h)
	double snowdepth; ///< snow accumulation (in)
	char1024 forecast_spec; ///< forecasting model
	char1024 tmyfile; ///< the TMY file name
	OBJECT* reader; ///< the file reader to use when loading data
	double temperature_raw; ///< the temperature (degC)
	double solar_raw; ///< raw solar flux (W/sf)
	double wind_dir; ///< wind direction
	double wind_gust; ///< wind gust
	enumeration interpolate; ///< interpolation method
	double solar_elevation; ///< solar elevation
	double solar_azimuth; ///< solar azimuth
	double solar_zenith; ///< solar zenith
	double direct_normal_extra; ///< extra direct normal radiation
	double pressure; ///< atmospheric pressure
	double tz_offset_val; ///< timezone offset value
	double global_horizontal_extra; ///< global horizontal radiation
	double tot_sky_cov; ///< total sky coverage
	double opq_sky_cov; ///< opaque sky coverage
	double cloud_opacity; ///< cloud opacity
	double cloud_reflectivity; ///< cloud reflectivity
	double cloud_speed_factor; ///< cloud speed factor
	enumeration cloud_model; ///< cloud model
	double update_time; ///< update time for weather calculations

public:
	climate() {}
	~climate() { if (defaults) delete defaults; }

	static inline climate* get_defaults() {
		if (!defaults) {
			defaults = new climate(); // Initialize lazily
		}
		return defaults;
	}

public:
	/**
	 * Handling `solar_direct` (double).
	 */
	static inline size_t get_solar_direct_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_direct) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_direct(void) {
		return solar_direct;
	}
	inline void set_solar_direct(double p) {
		solar_direct = p;
	}
	inline gld_property get_solar_direct_property(void) {
		return gld_property(my(), std::string("solar_direct").c_str());
	}
	inline std::string get_solar_direct_string(void) {
		return get_solar_direct_property().get_string().get_buffer();
	}
	inline void set_solar_direct(char* str) {
		get_solar_direct_property().from_string(str);
	}

	/**
	 * Handling `tz_meridian` (double).
	 */
	static inline size_t get_tz_meridian_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->tz_meridian) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_tz_meridian(void) {
		return tz_meridian;
	}
	inline void set_tz_meridian(double p) {
		tz_meridian = p;
	}
	inline gld_property get_tz_meridian_property(void) {
		return gld_property(my(), std::string("tz_meridian").c_str());
	}
	inline std::string get_tz_meridian_string(void) {
		return get_tz_meridian_property().get_string().get_buffer();
	}
	inline void set_tz_meridian(char* str) {
		get_tz_meridian_property().from_string(str);
	}

	/**
	 * Handling `solar_diffuse` (double).
	 */
	static inline size_t get_solar_diffuse_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_diffuse) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_diffuse(void) {
		return solar_diffuse;
	}
	inline void set_solar_diffuse(double p) {
		solar_diffuse = p;
	}
	inline gld_property get_solar_diffuse_property(void) {
		return gld_property(my(), std::string("solar_diffuse").c_str());
	}
	inline std::string get_solar_diffuse_string(void) {
		return get_solar_diffuse_property().get_string().get_buffer();
	}
	inline void set_solar_diffuse(char* str) {
		get_solar_diffuse_property().from_string(str);
	}

	/**
	 * Handling `solar_cloud_global` (double).
	 */
	static inline size_t get_solar_cloud_global_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_cloud_global) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_cloud_global(void) {
		return solar_cloud_global;
	}
	inline void set_solar_cloud_global(double p) {
		solar_cloud_global = p;
	}
	inline gld_property get_solar_cloud_global_property(void) {
		return gld_property(my(), std::string("solar_cloud_global").c_str());
	}
	inline std::string get_solar_cloud_global_string(void) {
		return get_solar_cloud_global_property().get_string().get_buffer();
	}
	inline void set_solar_cloud_global(char* str) {
		get_solar_cloud_global_property().from_string(str);
	}

	/**
	 * Handling `solar_cloud_direct` (double).
	 */
	static inline size_t get_solar_cloud_direct_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_cloud_direct) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_cloud_direct(void) {
		return solar_cloud_direct;
	}
	inline void set_solar_cloud_direct(double p) {
		solar_cloud_direct = p;
	}
	inline gld_property get_solar_cloud_direct_property(void) {
		return gld_property(my(), std::string("solar_cloud_direct").c_str());
	}
	inline std::string get_solar_cloud_direct_string(void) {
		return get_solar_cloud_direct_property().get_string().get_buffer();
	}
	inline void set_solar_cloud_direct(char* str) {
		get_solar_cloud_direct_property().from_string(str);
	}

	/**
	 * Handling `solar_cloud_diffuse` (double).
	 */
	static inline size_t get_solar_cloud_diffuse_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_cloud_diffuse) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_cloud_diffuse(void) {
		return solar_cloud_diffuse;
	}
	inline void set_solar_cloud_diffuse(double p) {
		solar_cloud_diffuse = p;
	}
	inline gld_property get_solar_cloud_diffuse_property(void) {
		return gld_property(my(), std::string("solar_cloud_diffuse").c_str());
	}
	inline std::string get_solar_cloud_diffuse_string(void) {
		return get_solar_cloud_diffuse_property().get_string().get_buffer();
	}
	inline void set_solar_cloud_diffuse(char* str) {
		get_solar_cloud_diffuse_property().from_string(str);
	}

	/**
	 * Handling `cloud_alpha` (double).
	 */
	static inline size_t get_cloud_alpha_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_alpha) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_alpha(void) {
		return cloud_alpha;
	}
	inline void set_cloud_alpha(double p) {
		cloud_alpha = p;
	}
	inline gld_property get_cloud_alpha_property(void) {
		return gld_property(my(), std::string("cloud_alpha").c_str());
	}
	inline std::string get_cloud_alpha_string(void) {
		return get_cloud_alpha_property().get_string().get_buffer();
	}
	inline void set_cloud_alpha(char* str) {
		get_cloud_alpha_property().from_string(str);
	}

	/**
	 * Handling `cloud_num_layers` (double).
	 */
	static inline size_t get_cloud_num_layers_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_num_layers) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_num_layers(void) {
		return cloud_num_layers;
	}
	inline void set_cloud_num_layers(double p) {
		cloud_num_layers = p;
	}
	inline gld_property get_cloud_num_layers_property(void) {
		return gld_property(my(), std::string("cloud_num_layers").c_str());
	}
	inline std::string get_cloud_num_layers_string(void) {
		return get_cloud_num_layers_property().get_string().get_buffer();
	}
	inline void set_cloud_num_layers(char* str) {
		get_cloud_num_layers_property().from_string(str);
	}

	/**
	 * Handling `cloud_aerosol_transmissivity` (double).
	 */
	static inline size_t get_cloud_aerosol_transmissivity_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_aerosol_transmissivity) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_aerosol_transmissivity(void) {
		return cloud_aerosol_transmissivity;
	}
	inline void set_cloud_aerosol_transmissivity(double p) {
		cloud_aerosol_transmissivity = p;
	}
	inline gld_property get_cloud_aerosol_transmissivity_property(void) {
		return gld_property(my(), std::string("cloud_aerosol_transmissivity").c_str());
	}
	inline std::string get_cloud_aerosol_transmissivity_string(void) {
		return get_cloud_aerosol_transmissivity_property().get_string().get_buffer();
	}
	inline void set_cloud_aerosol_transmissivity(char* str) {
		get_cloud_aerosol_transmissivity_property().from_string(str);
	}

	/**
	 * Handling `ground_reflectivity` (double).
	 */
	static inline size_t get_ground_reflectivity_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->ground_reflectivity) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_ground_reflectivity(void) {
		return ground_reflectivity;
	}
	inline void set_ground_reflectivity(double p) {
		ground_reflectivity = p;
	}
	inline gld_property get_ground_reflectivity_property(void) {
		return gld_property(my(), std::string("ground_reflectivity").c_str());
	}
	inline std::string get_ground_reflectivity_string(void) {
		return get_ground_reflectivity_property().get_string().get_buffer();
	}
	inline void set_ground_reflectivity(char* str) {
		get_ground_reflectivity_property().from_string(str);
	}

public:
	/**
	  * Handling `city` (char32).
	  */
	static inline size_t get_city_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->city) - reinterpret_cast<const char*>(current_defaults);
	}
	inline std::string get_city(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(city);
	}
	inline void set_city(const char* str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(city, str, sizeof(city) - 1);
		city[sizeof(city) - 1] = '\0'; // Ensure null termination.
	}
	inline gld_property get_city_property(void) {
		return gld_property(my(), std::string("city").c_str());
	}
	inline void set_city(char* str) {
		get_city_property().from_string(str);
	}

	/**
	 * Handling `temperature` (double).
	 */
	static inline size_t get_temperature_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->temperature) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_temperature(void) {
		return temperature;
	}
	inline void set_temperature(double p) {
		temperature = p;
	}
	inline gld_property get_temperature_property(void) {
		return gld_property(my(), std::string("temperature").c_str());
	}
	inline std::string get_temperature_string(void) {
		return std::string(get_temperature_property().get_string().get_buffer());
	}
	inline void set_temperature(char* str) {
		get_temperature_property().from_string(str);
	}

	/**
	 * Handling `humidity` (double).
	 */
	static inline size_t get_humidity_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->humidity) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_humidity(void) {
		return humidity;
	}
	inline void set_humidity(double p) {
		humidity = p;
	}
	inline gld_property get_humidity_property(void) {
		return gld_property(my(), std::string("humidity").c_str());
	}
	inline std::string get_humidity_string(void) {
		return get_humidity_property().get_string().get_buffer();
	}
	inline void set_humidity(char* str) {
		get_humidity_property().from_string(str);
	}

	/**
	 * Handling `wind_speed` (double).
	 */
	static inline size_t get_wind_speed_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->wind_speed) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_wind_speed(void) {
		return wind_speed;
	}
	inline void set_wind_speed(double p) {
		wind_speed = p;
	}
	inline gld_property get_wind_speed_property(void) {
		return gld_property(my(), std::string("wind_speed").c_str());
	}
	inline std::string get_wind_speed_string(void) {
		return get_wind_speed_property().get_string().get_buffer();
	}
	inline void set_wind_speed(char* str) {
		get_wind_speed_property().from_string(str);
	}

	/**
	 * Handling `solar_global` (double).
	 */
	static inline size_t get_solar_global_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_global) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_global(void) {
		return solar_global;
	}
	inline void set_solar_global(double p) {
		solar_global = p;
	}
	inline gld_property get_solar_global_property(void) {
		return gld_property(my(), std::string("solar_global").c_str());
	}
	inline std::string get_solar_global_string(void) {
		return get_solar_global_property().get_string().get_buffer();
	}
	inline void set_solar_global(char* str) {
		get_solar_global_property().from_string(str);
	}
public:
	/**
	 * Handling `temperature_raw` (double).
	 */
	static inline size_t get_temperature_raw_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->temperature_raw) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_temperature_raw(void) {
		return temperature_raw;
	}
	inline void set_temperature_raw(double p) {
		temperature_raw = p;
	}
	inline gld_property get_temperature_raw_property(void) {
		return gld_property(my(), std::string("temperature_raw").c_str());
	}
	inline std::string get_temperature_raw_string(void) {
		return get_temperature_raw_property().get_string().get_buffer();
	}
	inline void set_temperature_raw(char* str) {
		get_temperature_raw_property().from_string(str);
	}

	/**
	 * Handling `solar_raw` (double).
	 */
	static inline size_t get_solar_raw_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_raw) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_raw(void) {
		return solar_raw;
	}
	inline void set_solar_raw(double p) {
		solar_raw = p;
	}
	inline gld_property get_solar_raw_property(void) {
		return gld_property(my(), std::string("solar_raw").c_str());
	}
	inline std::string get_solar_raw_string(void) {
		return get_solar_raw_property().get_string().get_buffer();
	}
	inline void set_solar_raw(char* str) {
		get_solar_raw_property().from_string(str);
	}

	/**
	 * Handling `wind_dir` (double).
	 */
	static inline size_t get_wind_dir_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->wind_dir) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_wind_dir(void) {
		return wind_dir;
	}
	inline void set_wind_dir(double p) {
		wind_dir = p;
	}
	inline gld_property get_wind_dir_property(void) {
		return gld_property(my(), std::string("wind_dir").c_str());
	}
	inline std::string get_wind_dir_string(void) {
		return get_wind_dir_property().get_string().get_buffer();
	}
	inline void set_wind_dir(char* str) {
		get_wind_dir_property().from_string(str);
	}

	/**
	 * Handling `wind_gust` (double).
	 */
	static inline size_t get_wind_gust_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->wind_gust) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_wind_gust(void) {
		return wind_gust;
	}
	inline void set_wind_gust(double p) {
		wind_gust = p;
	}
	inline gld_property get_wind_gust_property(void) {
		return gld_property(my(), std::string("wind_gust").c_str());
	}
	inline std::string get_wind_gust_string(void) {
		return get_wind_gust_property().get_string().get_buffer();
	}
	inline void set_wind_gust(char* str) {
		get_wind_gust_property().from_string(str);
	}

	/**
	 * Handling `interpolate` (enumeration).
	 */
	static inline size_t get_interpolate_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->interpolate) - reinterpret_cast<const char*>(current_defaults);
	}
	inline enumeration get_interpolate(void) {
		return interpolate;
	}
	inline void set_interpolate(enumeration p) {
		interpolate = p;
	}
	inline gld_property get_interpolate_property(void) {
		return gld_property(my(), std::string("interpolate").c_str());
	}
	inline std::string get_interpolate_string(void) {
		return get_interpolate_property().get_string().get_buffer();
	}
	inline void set_interpolate(char* str) {
		get_interpolate_property().from_string(str);
	}

	/**
	 * Handling `solar_elevation` (double).
	 */
	static inline size_t get_solar_elevation_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_elevation) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_elevation(void) {
		return solar_elevation;
	}
	inline void set_solar_elevation(double p) {
		solar_elevation = p;
	}
	inline gld_property get_solar_elevation_property(void) {
		return gld_property(my(), std::string("solar_elevation").c_str());
	}
	inline std::string get_solar_elevation_string(void) {
		return get_solar_elevation_property().get_string().get_buffer();
	}
	inline void set_solar_elevation(char* str) {
		get_solar_elevation_property().from_string(str);
	}

	/**
	 * Handling `solar_azimuth` (double).
	 */
	static inline size_t get_solar_azimuth_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_azimuth) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_azimuth(void) {
		return solar_azimuth;
	}
	inline void set_solar_azimuth(double p) {
		solar_azimuth = p;
	}
	inline gld_property get_solar_azimuth_property(void) {
		return gld_property(my(), std::string("solar_azimuth").c_str());
	}
	inline std::string get_solar_azimuth_string(void) {
		return get_solar_azimuth_property().get_string().get_buffer();
	}
	inline void set_solar_azimuth(char* str) {
		get_solar_azimuth_property().from_string(str);
	}

	/**
	 * Handling `solar_zenith` (double).
	 */
	static inline size_t get_solar_zenith_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->solar_zenith) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_solar_zenith(void) {
		return solar_zenith;
	}
	inline void set_solar_zenith(double p) {
		solar_zenith = p;
	}
	inline gld_property get_solar_zenith_property(void) {
		return gld_property(my(), std::string("solar_zenith").c_str());
	}
	inline std::string get_solar_zenith_string(void) {
		return get_solar_zenith_property().get_string().get_buffer();
	}
	inline void set_solar_zenith(char* str) {
		get_solar_zenith_property().from_string(str);
	}

	/**
	 * Handling `direct_normal_extra` (double).
	 */
	static inline size_t get_direct_normal_extra_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->direct_normal_extra) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_direct_normal_extra(void) {
		return direct_normal_extra;
	}
	inline void set_direct_normal_extra(double p) {
		direct_normal_extra = p;
	}
	inline gld_property get_direct_normal_extra_property(void) {
		return gld_property(my(), std::string("direct_normal_extra").c_str());
	}
	inline std::string get_direct_normal_extra_string(void) {
		return get_direct_normal_extra_property().get_string().get_buffer();
	}
	inline void set_direct_normal_extra(char* str) {
		get_direct_normal_extra_property().from_string(str);
	}

	/**
	 * Handling `pressure` (double).
	 */
	static inline size_t get_pressure_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->pressure) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_pressure(void) {
		return pressure;
	}
	inline void set_pressure(double p) {
		pressure = p;
	}
	inline gld_property get_pressure_property(void) {
		return gld_property(my(), std::string("pressure").c_str());
	}
	inline std::string get_pressure_string(void) {
		return get_pressure_property().get_string().get_buffer();
	}
	inline void set_pressure(char* str) {
		get_pressure_property().from_string(str);
	}

	/**
	 * Handling `tz_offset_val` (double).
	 */
	static inline size_t get_tz_offset_val_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->tz_offset_val) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_tz_offset_val(void) {
		return tz_offset_val;
	}
	inline void set_tz_offset_val(double p) {
		tz_offset_val = p;
	}
	inline gld_property get_tz_offset_val_property(void) {
		return gld_property(my(), std::string("tz_offset_val").c_str());
	}
	inline std::string get_tz_offset_val_string(void) {
		return get_tz_offset_val_property().get_string().get_buffer();
	}
	inline void set_tz_offset_val(char* str) {
		get_tz_offset_val_property().from_string(str);
	}

	/**
	 * Handling `global_horizontal_extra` (double).
	 */
	static inline size_t get_global_horizontal_extra_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->global_horizontal_extra) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_global_horizontal_extra(void) {
		return global_horizontal_extra;
	}
	inline void set_global_horizontal_extra(double p) {
		global_horizontal_extra = p;
	}
	inline gld_property get_global_horizontal_extra_property(void) {
		return gld_property(my(), std::string("global_horizontal_extra").c_str());
	}
	inline std::string get_global_horizontal_extra_string(void) {
		return get_global_horizontal_extra_property().get_string().get_buffer();
	}
	inline void set_global_horizontal_extra(char* str) {
		get_global_horizontal_extra_property().from_string(str);
	}

	/**
	 * Handling `tot_sky_cov` (double).
	 */
	static inline size_t get_tot_sky_cov_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->tot_sky_cov) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_tot_sky_cov(void) {
		return tot_sky_cov;
	}
	inline void set_tot_sky_cov(double p) {
		tot_sky_cov = p;
	}
	inline gld_property get_tot_sky_cov_property(void) {
		return gld_property(my(), std::string("tot_sky_cov").c_str());
	}
	inline std::string get_tot_sky_cov_string(void) {
		return get_tot_sky_cov_property().get_string().get_buffer();
	}
	inline void set_tot_sky_cov(char* str) {
		get_tot_sky_cov_property().from_string(str);
	}

	/**
	 * Handling `opq_sky_cov` (double).
	 */
	static inline size_t get_opq_sky_cov_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->opq_sky_cov) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_opq_sky_cov(void) {
		return opq_sky_cov;
	}
	inline void set_opq_sky_cov(double p) {
		opq_sky_cov = p;
	}
	inline gld_property get_opq_sky_cov_property(void) {
		return gld_property(my(), std::string("opq_sky_cov").c_str());
	}
	inline std::string get_opq_sky_cov_string(void) {
		return get_opq_sky_cov_property().get_string().get_buffer();
	}
	inline void set_opq_sky_cov(char* str) {
		get_opq_sky_cov_property().from_string(str);
	}

	/**
	 * Handling `cloud_opacity` (double).
	 */
	static inline size_t get_cloud_opacity_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_opacity) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_opacity(void) {
		return cloud_opacity;
	}
	inline void set_cloud_opacity(double p) {
		cloud_opacity = p;
	}
	inline gld_property get_cloud_opacity_property(void) {
		return gld_property(my(), std::string("cloud_opacity").c_str());
	}
	inline std::string get_cloud_opacity_string(void) {
		return get_cloud_opacity_property().get_string().get_buffer();
	}
	inline void set_cloud_opacity(char* str) {
		get_cloud_opacity_property().from_string(str);
	}

	/**
	 * Handling `cloud_reflectivity` (double).
	 */
	static inline size_t get_cloud_reflectivity_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_reflectivity) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_reflectivity(void) {
		return cloud_reflectivity;
	}
	inline void set_cloud_reflectivity(double p) {
		cloud_reflectivity = p;
	}
	inline gld_property get_cloud_reflectivity_property(void) {
		return gld_property(my(), std::string("cloud_reflectivity").c_str());
	}
	inline std::string get_cloud_reflectivity_string(void) {
		return get_cloud_reflectivity_property().get_string().get_buffer();
	}
	inline void set_cloud_reflectivity(char* str) {
		get_cloud_reflectivity_property().from_string(str);
	}

	/**
	 * Handling `cloud_speed_factor` (double).
	 */
	static inline size_t get_cloud_speed_factor_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_speed_factor) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_cloud_speed_factor(void) {
		return cloud_speed_factor;
	}
	inline void set_cloud_speed_factor(double p) {
		cloud_speed_factor = p;
	}
	inline gld_property get_cloud_speed_factor_property(void) {
		return gld_property(my(), std::string("cloud_speed_factor").c_str());
	}
	inline std::string get_cloud_speed_factor_string(void) {
		return get_cloud_speed_factor_property().get_string().get_buffer();
	}
	inline void set_cloud_speed_factor(char* str) {
		get_cloud_speed_factor_property().from_string(str);
	}

	/**
	 * Handling `cloud_model` (enumeration).
	 */
	static inline size_t get_cloud_model_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->cloud_model) - reinterpret_cast<const char*>(current_defaults);
	}
	inline enumeration get_cloud_model(void) {
		return cloud_model;
	}
	inline void set_cloud_model(enumeration p) {
		cloud_model = p;
	}
	inline gld_property get_cloud_model_property(void) {
		return gld_property(my(), std::string("cloud_model").c_str());
	}
	inline std::string get_cloud_model_string(void) {
		return get_cloud_model_property().get_string().get_buffer();
	}
	inline void set_cloud_model(char* str) {
		get_cloud_model_property().from_string(str);
	}

	/**
	 * Handling `update_time` (double).
	 */
	static inline size_t get_update_time_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->update_time) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_update_time(void) {
		return update_time;
	}
	inline void set_update_time(double p) {
		update_time = p;
	}
	inline gld_property get_update_time_property(void) {
		return gld_property(my(), std::string("update_time").c_str());
	}
	inline std::string get_update_time_string(void) {
		return get_update_time_property().get_string().get_buffer();
	}
	inline void set_update_time(char* str) {
		get_update_time_property().from_string(str);
	}
public:
	/**
	 * Handling `record` (CLIMATERECORD).
	 */
	static inline size_t get_record_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->record) - reinterpret_cast<const char*>(current_defaults);
	}
	inline CLIMATERECORD get_record(void) {
		return record;
	}
	inline void set_record(CLIMATERECORD p) {
		record = p;
	}
	inline gld_property get_record_property(void) {
		return gld_property(my(), std::string("record").c_str());
	}
	inline std::string get_record_string(void) {
		return get_record_property().get_string().get_buffer();
	}
	inline void set_record(char* str) {
		get_record_property().from_string(str);
	}

	/**
	 * Handling `rainfall` (double).
	 */
	static inline size_t get_rainfall_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->rainfall) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_rainfall(void) {
		return rainfall;
	}
	inline void set_rainfall(double p) {
		rainfall = p;
	}
	inline gld_property get_rainfall_property(void) {
		return gld_property(my(), std::string("rainfall").c_str());
	}
	inline std::string get_rainfall_string(void) {
		return get_rainfall_property().get_string().get_buffer();
	}
	inline void set_rainfall(char* str) {
		get_rainfall_property().from_string(str);
	}

	/**
	 * Handling `snowdepth` (double).
	 */
	static inline size_t get_snowdepth_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->snowdepth) - reinterpret_cast<const char*>(current_defaults);
	}
	inline double get_snowdepth(void) {
		return snowdepth;
	}
	inline void set_snowdepth(double p) {
		snowdepth = p;
	}
	inline gld_property get_snowdepth_property(void) {
		return gld_property(my(), std::string("snowdepth").c_str());
	}
	inline std::string get_snowdepth_string(void) {
		return get_snowdepth_property().get_string().get_buffer();
	}
	inline void set_snowdepth(char* str) {
		get_snowdepth_property().from_string(str);
	}

	/**
	 * Handling `forecast_spec` (char1024).
	 */
	static inline size_t get_forecast_spec_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->forecast_spec) - reinterpret_cast<const char*>(current_defaults);
	}
	inline std::string get_forecast_spec(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(forecast_spec);
	}
	inline void set_forecast_spec(const char* str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(forecast_spec, str, sizeof(forecast_spec) - 1);
		forecast_spec[sizeof(forecast_spec) - 1] = '\0'; // Ensure null termination.
	}
	inline gld_property get_forecast_spec_property(void) {
		return gld_property(my(), std::string("forecast_spec").c_str());
	}
	inline void set_forecast_spec(char* str) {
		get_forecast_spec_property().from_string(str);
	}

	/**
	 * Handling `tmyfile` (char1024).
	 */
	static inline size_t get_tmyfile_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->tmyfile) - reinterpret_cast<const char*>(current_defaults);
	}
	inline std::string get_tmyfile(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx);
		return std::string(tmyfile);
	}
	inline void set_tmyfile(const char* str) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx);
		strncpy(tmyfile, str, sizeof(tmyfile) - 1);
		tmyfile[sizeof(tmyfile) - 1] = '\0'; // Ensure null termination.
	}
	inline gld_property get_tmyfile_property(void) {
		return gld_property(my(), std::string("tmyfile").c_str());
	}
	inline void set_tmyfile(char* str) {
		get_tmyfile_property().from_string(str);
	}

	/**
	 * Handling `reader` (OBJECT*).
	 */
	static inline size_t get_reader_offset(void) {
		climate* current_defaults = get_defaults();
		return reinterpret_cast<const char*>(&current_defaults->reader) - reinterpret_cast<const char*>(current_defaults);
	}
	inline OBJECT* get_reader(void) {
		return reader;
	}
	inline void set_reader(OBJECT* p) {
		reader = p;
	}
	inline gld_property get_reader_property(void) {
		return gld_property(my(), std::string("reader").c_str());
	}
	inline std::string get_reader_string(void) {
		return get_reader_property().get_string().get_buffer();
	}
	inline void set_reader(char* str) {
		get_reader_property().from_string(str);
	}

	/**
	 * Handling `solar_flux` (GL_ARRAY with indices from CP_LAST).
	 */
protected:
	double solar_flux[CP_LAST]; ///< Solar flux array (W/sf). Elements are in order: [S, SE, SW, E, W, NE, NW, N, H].

public:
	/**
	 * Get the byte offset of the `solar_flux` array within the object.
	 */
	static inline size_t get_solar_flux_offset(void) {
		return reinterpret_cast<const char*>(&defaults->solar_flux) - reinterpret_cast<const char*>(defaults);
	}

	/**
	 * Get the `gld_property` object for `solar_flux`.
	 */
	inline gld_property get_solar_flux_property(void) {
		return gld_property(my(), "solar_flux");
	}

	/**
	 * Get the pointer to the `solar_flux` array (thread-safe read lock included).
	 * @return Pointer to the `solar_flux` array.
	 */
	inline double* get_solar_flux(void) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Read lock ensures thread-safe access.
		return solar_flux;
	}

	/**
	 * Get an individual element of the `solar_flux` array by index (thread-safe read lock included).
	 * @param n Index of the array element to retrieve.
	 * @return The value of the array element at index `n`.
	 */
	inline double get_solar_flux(size_t n) {
		if (n >= CP_LAST) {
			throw std::out_of_range("Index exceeds solar_flux array bounds.");
		}
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::shared_lock<std::shared_mutex> lock(mtx); // Read lock ensures thread-safe access.
		return solar_flux[n];
	}

	/**
	 * Set the entire `solar_flux` array (thread-safe write lock included).
	 * @param p Pointer to an array of doubles to copy into `solar_flux`.
	 */
	inline void set_solar_flux(double* p) {
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Write lock ensures thread-safe modification.
		memcpy(solar_flux, p, sizeof(solar_flux));
	}

	/**
	 * Set an individual element of the `solar_flux` array by index (thread-safe write lock included).
	 * @param n Index of the array element to modify.
	 * @param m Value to set at index `n`.
	 */
	inline void set_solar_flux(size_t n, double m) {
		if (n >= CP_LAST) {
			throw std::out_of_range("Index exceeds solar_flux array bounds.");
		}
		auto& mtx = SharedMutexManager::get_mutex(my());
		std::unique_lock<std::shared_mutex> lock(mtx); // Write lock ensures thread-safe modification.
		solar_flux[n] = m;
	}


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
	void init_cloud_pattern(void);
	void update_cloud_pattern(TIMESTAMP dt);
	int get_solar_for_location(double latitude, double longitude, double *direct, double *global, double *diffuse);
private:
	int calc_cloud_pattern_size(std::vector<std::vector<double> > &location_list);
	void build_cloud_pattern(int col_min, int col_max, int row_min, int row_max);
	void write_out_cloud_pattern(char pattern);
	void write_out_pattern_shift(int row_shift, int col_shift);
	void rebuild_cloud_pattern_edge( char edge_needing_rebuilt);
	void trim_pattern_edge( char rebuilt_edge);
	void erase_off_screen_pattern( char edge_to_erase);
	int get_fuzzy_cloud_value_for_location(double latitude, double longitude, double *cloud);
	int get_binary_cloud_value_for_location(double latitude, double longitude, int *cloud);
	double convert_to_binary_cloud();
	void convert_to_fuzzy_cloud( double cut_elevation, int num_fuzzy_layers, double alpha);
	TIMESTAMP prev_NTime;
	int MIN_LAT_INDEX;
	int MAX_LAT_INDEX;
	double MIN_LAT;
	double MAX_LAT;
	int MIN_LON_INDEX;
	int MAX_LON_INDEX;
	double MIN_LON;
	double MAX_LON;
	double global_transmissivity;
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
