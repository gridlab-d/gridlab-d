/** $Id: solar_angles.h 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include <math.h>

#define HR_PER_RADIAN (12.0 / PI)
#define PI_OVER_180 (PI / 180)

static short days_thru_month[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};  // Ignores leap years...

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
    double cos_incident(double lititude, double slope, double az, double sol_time, short day_of_yr);    // unitless
    double incident(double lititude, double slope, double az, double sol_time, short day_of_yr);        // radians
    double zenith(short day_of_yr, double lititude, double sol_time);                   // radians
    double altitude(short day_of_yr, double lititude, double sol_time);                 // radians
    double hr_sunrise(short day_of_yr, double lititude);                            // decimal hours
    double day_len(short day_of_yr, double lititude);                           // decimal hours
    short  day_of_yr(short month, short day);                               // julian days
};
