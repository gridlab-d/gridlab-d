/** $Id: solar_angles.h 4738 2014-07-03 00:55:39Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <math.h>

#define HR_PER_RADIAN (12.0 / PI)
#define PI_OVER_180 (PI / 180)
#define raddeg (PI / 180)
#define degrad (180 / PI)
#define COS85DEG 0.08715574274765817355806427083747

static short days_thru_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};  // Ignores leap years...

//Solpos
// cumulative number of days prior to beginning of month - SOLPOS constants
static int  month_days[2][13] = { { 0,   0,  31,  59,  90, 120, 151,
                                   181, 212, 243, 273, 304, 334 },
                                { 0,   0,  31,  60,  91, 121, 152,
                                   182, 213, 244, 274, 305, 335 } };

//Perez tilt model coefficients - extracted from Perez et al., 1990.
static double perez_tilt_coeff_F1[3][8] = {{-0.008, 0.130, 0.330, 0.568, 0.873, 1.132, 1.060, 0.678},
											{0.588, 0.683, 0.487, 0.187, -0.392, -1.237, -1.600, -0.327},
											{-0.062, -0.151, -0.221, -0.295, -0.362, -0.412, -0.359, -0.250}};

static double perez_tilt_coeff_F2[3][8] = {{-0.060, -0.019, 0.055, 0.109, 0.226, 0.288, 0.264, 0.156},
											{0.072, 0.066, -0.064, -0.152, -0.462, -0.823, -1.127, -1.377},
											{-0.022, -0.029, -0.026, -0.014, 0.001, 0.056, 0.131, 0.251}};

//Boundaries for Perez model "discrete sky clearness" categories - from Perez et al., 1990
static double perez_clearness_limits[8] = {1.0, 1.065, 1.230, 1.500, 1.950, 2.800, 4.500, 6.200};

class SolarAngles {
private:
    // No instance data (for now).  Someday it may be useful to
    // keep latitude, longitude, and standard meridian as member
    // data, but that would complicate using the class for multiple
    // locations sequentially...

public:

    // The order of the arguments across methods is not always
    // consistent.  They're that way to match the S-PLUS and C
    // implementations, which are that way for no particular reason...

    SolarAngles(void);
    ~SolarAngles(void);

    double eq_time(short day_of_yr);                                    // hours
    double solar_time(double std_time, short day_of_yr, double std_meridian, double longitude);     // decimal hours
    double local_time(double std_time, short day_of_yr, double std_meridian, double longitude);     // decimal hours
    double declination(short day_of_yr);                                    // radians
    double cos_incident(double latitude, double slope, double az, double sol_time, short day_of_yr);    // unitless
    double incident(double latitude, double slope, double az, double sol_time, short day_of_yr);        // radians
    double zenith(short day_of_yr, double latitude, double sol_time);                   // radians
    double altitude(short day_of_yr, double latitude, double sol_time);                 // radians
    double hr_sunrise(short day_of_yr, double latitude);                            // decimal hours
    double day_len(short day_of_yr, double latitude);                           // decimal hours
    short  day_of_yr(short month, short day);                               // julian days
	//sjin: add solar elevation and azimuth funcions
	double elevation(short day_of_yr, double latitude, double sol_time);
	double azimuth(short day_of_yr, double latitude, double sol_time);

	//Most additions below here are from the NREL Solar Position algorithm 2.0
	//http://rredc.nrel.gov/solar/codesandalgorithms/solpos/aboutsolpos.html
	//Perez model function at the end (perez_tilt) extracted from referenced paper
	typedef struct SOLPOS_TRIGDATA //used to pass calculated values locally
		{
			double cd;       // cosine of the declination
			double ch;       // cosine of the hour angle
			double cl;       // cosine of the latitude
			double sd;       // sine of the declination
			double sl;       // sine of the latitude
		};

	typedef struct SOLPOS_POSDATA
	{
		/***** ALPHABETICAL LIST OF COMMON VARIABLES *****/
			   /* Each comment begins with a 1-column letter code:
				  I:  INPUT variable
				  O:  OUTPUT variable
				  T:  TRANSITIONAL variable used in the algorithm,
					  of interest only to the solar radiation
					  modelers, and available to you because you
					  may be one of them.

				  The FUNCTION column indicates which sub-function
				  within solpos must be switched on using the
				  "function" parameter to calculate the desired
				  output variable.  All function codes are
				  defined in the solpos.h file.  The default
				  S_ALL switch calculates all output variables.
				  Multiple functions may be or'd to create a
				  composite function switch.  For example,
				  (S_TST | S_SBCF). Specifying only the functions
				  for required output variables may allow solpos
				  to execute more quickly.

				  The S_DOY mask works as a toggle between the
				  input date represented as a day number (daynum)
				  or as month and day.  To set the switch (to
				  use daynum input), the function is or'd; to
				  clear the switch (to use month and day input),
				  the function is inverted and and'd.

				  For example:
					  pdat->function |= S_DOY (sets daynum input)
					  pdat->function &= ~S_DOY (sets month and day input)

				  Whichever date form is used, S_solpos will
				  calculate and return the variables(s) of the
				  other form.  See the soltest.c program for
				  other examples. */

		/* VARIABLE        I/O  Function    Description */
		/* -------------  ----  ----------  ---------------------------------------*/

		int   day;       /* I/O: S_DOY      Day of month (May 27 = 27, etc.)
											solpos will CALCULATE this by default,
											or will optionally require it as input
											depending on the setting of the S_DOY
											function switch. */
		int   daynum;    /* I/O: S_DOY      Day number (day of year; Feb 1 = 32 )
											solpos REQUIRES this by default, but
											will optionally calculate it from
											month and day depending on the setting
											of the S_DOY function switch. */
		int   function;  /* I:              Switch to choose functions for desired
											output. */
		int   hour;      /* I:              Hour of day, 0 - 23, DEFAULT = 12 */
		int   interval;  /* I:              Interval of a measurement period in
											seconds.  Forces solpos to use the
											time and date from the interval
											midpoint. The INPUT time (hour,
											minute, and second) is assumed to
											be the END of the measurement
											interval. */
		int   minute;    /* I:              Minute of hour, 0 - 59, DEFAULT = 0 */
		int   month;     /* I/O: S_DOY      Month number (Jan = 1, Feb = 2, etc.)
											solpos will CALCULATE this by default,
											or will optionally require it as input
											depending on the setting of the S_DOY
											function switch. */
		int   second;    /* I:              Second of minute, 0 - 59, DEFAULT = 0 */
		int   year;      /* I:              4-digit year (2-digit year is NOT
										   allowed */

		/***** doubleS *****/

		double amass;      /* O:  S_AMASS    Relative optical airmass */
		double ampress;    /* O:  S_AMASS    Pressure-corrected airmass */
		double aspect;     /* I:             Azimuth of panel surface (direction it
											faces) N=0, E=90, S=180, W=270,
											DEFAULT = 180 */
		double azim;       /* O:  S_SOLAZM   Solar azimuth angle:  N=0, E=90, S=180,
											W=270 */
		double cosinc;     /* O:  S_TILT     Cosine of solar incidence angle on
											panel */
		double coszen;     /* O:  S_REFRAC   Cosine of refraction corrected solar
											zenith angle */
		double dayang;     /* T:  S_GEOM     Day angle (daynum*360/year-length)
											degrees */
		double declin;     /* T:  S_GEOM     Declination--zenith angle of solar noon
											at equator, degrees NORTH */
		double eclong;     /* T:  S_GEOM     Ecliptic longitude, degrees */
		double ecobli;     /* T:  S_GEOM     Obliquity of ecliptic */
		double ectime;     /* T:  S_GEOM     Time of ecliptic calculations */
		double elevetr;    /* O:  S_ZENETR   Solar elevation, no atmospheric
											correction (= ETR) */
		double elevref;    /* O:  S_REFRAC   Solar elevation angle,
											deg. from horizon, refracted */
		double eqntim;     /* T:  S_TST      Equation of time (TST - LMT), minutes */
		double erv;        /* T:  S_GEOM     Earth radius vector
											(multiplied to solar constant) */
		double etr;        /* O:  S_ETR      Extraterrestrial (top-of-atmosphere)
											W/sq m global horizontal solar
											irradiance */
		double etrn;       /* O:  S_ETR      Extraterrestrial (top-of-atmosphere)
											W/sq m direct normal solar
											irradiance */
		double etrtilt;    /* O:  S_TILT     Extraterrestrial (top-of-atmosphere)
											W/sq m global irradiance on a tilted
											surface */
		double gmst;       /* T:  S_GEOM     Greenwich mean sidereal time, hours */
		double hrang;      /* T:  S_GEOM     Hour angle--hour of sun from solar noon,
											degrees WEST */
		double julday;     /* T:  S_GEOM     Julian Day of 1 JAN 2000 minus
											2,400,000 days (in order to regain
											single precision) */
		double latitude;   /* I:             Latitude, degrees north (south negative) */
		double longitude;  /* I:             Longitude, degrees east (west negative) */
		double lmst;       /* T:  S_GEOM     Local mean sidereal time, degrees */
		double mnanom;     /* T:  S_GEOM     Mean anomaly, degrees */
		double mnlong;     /* T:  S_GEOM     Mean longitude, degrees */
		double rascen;     /* T:  S_GEOM     Right ascension, degrees */
		double press;      /* I:             Surface pressure, millibars, used for
											refraction correction and ampress */
		double prime;      /* O:  S_PRIME    Factor that normalizes Kt, Kn, etc. */
		double sbcf;       /* O:  S_SBCF     Shadow-band correction factor */
		double sbwid;      /* I:             Shadow-band width (cm) */
		double sbrad;      /* I:             Shadow-band radius (cm) */
		double sbsky;      /* I:             Shadow-band sky factor */
		double solcon;     /* I:             Solar constant (NREL uses 1367 W/sq m) */
		double ssha;       /* T:  S_SRHA     Sunset(/rise) hour angle, degrees */
		double sretr;      /* O:  S_SRSS     Sunrise time, minutes from midnight,
											local, WITHOUT refraction */
		double ssetr;      /* O:  S_SRSS     Sunset time, minutes from midnight,
											local, WITHOUT refraction */
		double temp;       /* I:             Ambient dry-bulb temperature, degrees C,
											used for refraction correction */
		double tilt;       /* I:             Degrees tilt from horizontal of panel */
		double timezone;   /* I:             Time zone, east (west negative).
										  USA:  Mountain = -7, Central = -6, etc. */
		double tst;        /* T:  S_TST      True solar time, minutes from midnight */
		double tstfix;     /* T:  S_TST      True solar time - local standard time */
		double unprime;    /* O:  S_PRIME    Factor that denormalizes Kt', Kn', etc. */
		double utime;      /* T:  S_GEOM     Universal (Greenwich) standard time */
		double zenetr;     /* T:  S_ZENETR   Solar zenith angle, no atmospheric
											correction (= ETR) */
		double zenref;     /* O:  S_REFRAC   Solar zenith angle, deg. from zenith,
											refracted */
		//Added variables - Perez model
		double diff_horz;					//Diffuse horizontal radiation for Perez tilt model calculations
		double dir_norm;					//Direct normal radiation for Perez tilt model calculations
		double extra_irrad;					//Extraterrestrial direct normal irradiance
		double perez_horz;					//Horizontal scalar from Perez diffuse model
		int perez_skyclear_idx;				//Sky clearness index from Perez tilt model
		double perez_skyclear;				//Sky clearness epsilon value from Perez tilt model
		double perez_brightness;			//Sky brightness value from Perez tilt model
		double perez_F1;					//F1 coefficient calculation from Perez tilt model
		double perez_F2;					//F2 coefficient calculation from Perez tilt model
	};

	SOLPOS_POSDATA solpos_vals;				//Working variable for all solpos calculations

	int S_solpos(SOLPOS_POSDATA *pdat);
	void S_init(SOLPOS_POSDATA *pdat);

	void doy2dom( SOLPOS_POSDATA *pdat );
	void geometry( SOLPOS_POSDATA *pdat );
	void zen_no_ref ( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat );
	void ssha( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat );
	void sbcf( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat );
	void tst( SOLPOS_POSDATA *pdat );
	void srss( SOLPOS_POSDATA *pdat );
	void sazm( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat );
	void refrac( SOLPOS_POSDATA *pdat );
	void amass( SOLPOS_POSDATA *pdat );
	void prime( SOLPOS_POSDATA *pdat );
	void etr( SOLPOS_POSDATA *pdat );
	void tilt( SOLPOS_POSDATA *pdat );
	void perez_tilt( SOLPOS_POSDATA *pdat );

};
