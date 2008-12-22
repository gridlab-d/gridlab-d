/** $Id: solar_angles.cpp 1182 2008-12-22 22:08:36Z dchassin $
	Copyright (C) 2008 Battelle Memorial Institute
**/
#include "complex.h"
#include "solar_angles.h"



SolarAngles::SolarAngles()
{
}

SolarAngles::~SolarAngles()
{
}



/******************function eq_time****************************/
// PURPOSE: Compute equation of time (a measure of the earth's
//          "wobble" thoughout the year).
// EXPECTS: Day of year from Jan 1.
// RETURNS: Equation of time in hours.
// SOURCE:  Prof. Bill Murphy, Texas A&M U.
//
double SolarAngles::eq_time(short day_of_yr)
{
    double rad = (2.0 * PI * day_of_yr) / 365.0;
    return(
        ( 0.5501*cos(rad) - 3.0195*cos(2*rad) - 0.0771*cos(3*rad)
        -7.3403*sin(rad) - 9.4583*sin(2*rad) - 0.3284*sin(3*rad) ) / 60.0
    );
}

/******************function solar_time*************************/
// PURPOSE: Compute apparent solar time given local standard (not
//          daylight savings) time and location.
// EXPECTS: Local standard time, day of year, local standard
//          meridian, and local longitude.
// RETURNS: Local solar time in hours (military time)
// SOURCE:  Duffie & Beckman.
//
double SolarAngles::solar_time(
    double std_time,    // Local standard time in decimal hours (e.g., 7:30 is 7.5)
    short day_of_yr,    // Day of year from Jan 1
    double std_meridian,    // Standard meridian (longitude) of local time zone (radians---e.g., Pacific timezone is 120 degrees times pi/180)
    double longitude    // Local longitude (radians west)
)
{
    return std_time + eq_time(day_of_yr) + HR_PER_RADIAN * (std_meridian-longitude);
}


/*****************function local_time*************************/
// PURPOSE: Compute local standard (not daylight savings) time from
//          apparent solar time and location.
// EXPECTS: Apparent solar time, day of year, local standard
//          meridian, and local longitude.
// RETURNS: Local solar time in decimal hours.
// SOURCE:  Duffie & Beckman.
//
double SolarAngles::local_time(
    double sol_time,    // Local solar time (decimal hours--7:30 is 7.5)
    short day_of_yr,    // Day of year from Jan 1
    double std_meridian,    // Standard meridian (longitude) of local time zone (radians---e.g., Pacific timezone is 120 times pi/180)
    double longitude    // Local longitude (radians west)
)
{
    return sol_time - eq_time(day_of_yr) - HR_PER_RADIAN * (std_meridian-longitude);
}


/*****************function declination***********************/
// PURPOSE: Compute the solar declination angle (radians)
// EXPECTS: Day of year from Jan 1.
// RETURNS: Declination angle in radians.
// SOURCE:  Duffie & Beckman.
//
double SolarAngles::declination(short day_of_yr)
{
    return( 0.409280*sin(2.0*PI*(284+day_of_yr)/365) );
}


/******************function cos_incident******************/
// PURPOSE: Compute cosine of angle of incidence of solar beam radiation
//          on a surface
// EXPECTS: latitude, surface slope,
//          surface azimuth angle, solar time, day of year.
// RETURNS: Cosine of angle of incidence (angle between solar beam and
//          surface normal)
//          !!! Note that this function is ignorant of the presence !!!
//          !!! of the horizon.                                     !!!
// SOURCE:  Duffie & Beckman.
//
double SolarAngles::cos_incident(
    double latitude,    // Latitude (radians north)
    double slope,       // Slope of surface relative to horizontal (radians)
    double az,      // Azimuth angle of surface rel. to South (E+, W-) (radians)
    double sol_time,    // Solar time (decimal hours)
    short day_of_yr     // Day of year from Jan 1
)
{
    double sindecl,cosdecl,sinlat,coslat,sinslope,cosslope,sinaz,cosaz,sinhr,coshr; // For efficiency

    double hr_ang = -(15.0 * PI_OVER_180)*(sol_time-12.0);  // morning +, afternoon -

    double decl = declination(day_of_yr);

    // Precalculate
    sindecl  = sin(decl);       cosdecl  = cos(decl);
    sinlat   = sin(latitude);   coslat   = cos(latitude);
    sinslope = sin(slope);      cosslope = cos(slope);
    sinaz    = sin(az);     cosaz    = cos(az);
    sinhr    = sin(hr_ang);     coshr    = cos(hr_ang);

    // The answer...
    double answer = sindecl*sinlat*cosslope
        -sindecl*coslat*sinslope*cosaz
        +cosdecl*coslat*cosslope*coshr
        +cosdecl*sinlat*sinslope*cosaz*coshr
        +cosdecl*sinslope*sinaz*sinhr;

    // Deal with the sun being below the horizon...
    return(answer<0.0 ? (double)0.0 : answer);
}



/******************function incident******************/
// PURPOSE: Compute the angle of incidence of solar
//          beam radiation on a surface.
// EXPECTS: latitude, surface slope,
//          surface azimuth angle, solar time, day of year.
// RETURNS: Angle of incidence (angle between solar beam and
//          surface normal) in radians.
// SOURCE:  Duffie & Beckman.
//
double SolarAngles::incident(
    double latitude,    // Latitude (radians)
    double slope,       // Slope of surface relative to horizontal (radians)
    double az,      // Azimuth angle of surface rel. to South (radians, E+, W-)
    double sol_time,    // Solar time (decimal hours)
    short day_of_yr     // Day of year from Jan 1
)
{
    return acos(cos_incident(latitude, slope, az, sol_time, day_of_yr));
}


/*****************function zenith*****************************/
// PURPOSE: Compute the solar zenith angle (radians, the angle between solar beam
//          and the veritical.  This could be calculated as the incident
//          angle on a horizontal surface, but is more efficient with
//          this dedicated function.
// EXPECTS: Day of year, latitude, solar time.
// RETURNS: Zenith angle in radians.
// SOURCE:  Duffy & Beckman.
//
double SolarAngles::zenith(
    short day_of_yr,    // day of year from Jan 1
    double latitude,    // latitude (radians)
    double sol_time     // solar time (decimal hours)
)
{
    double hr_ang = -(15.0 * PI_OVER_180)*(sol_time-12.0); // morning +, afternoon -

    double decl = declination(day_of_yr);

    return acos(sin(decl)*sin(latitude) + cos(decl)*cos(latitude)*cos(hr_ang));
}



/*****************function altitude*****************************/
// PURPOSE: Compute the solar altitude angle (radians, angle between solar beam
//          and the veritical.  This could be calculated as the incident
//          angle on a vertical surface, but is more efficient with
//          this dedicated function.
// EXPECTS: Day of year, latitude, solar time.
// RETURNS: Altitude angle (angle above horizon) in radians.
// SOURCE:  Duffy & Beckman.
//
double SolarAngles::altitude(
    short day_of_yr,    // day of year from Jan 1
    double latitude,    // latitude (radians)
    double sol_time     // solar time (hours)
)
{
    return (90.0 * PI_OVER_180) - zenith(day_of_yr,latitude,sol_time);
}



/*****************function hr_sunrise*****************************/
// PURPOSE: Compute the hour of sunrise.
// EXPECTS: Declination angle, latitude.
// RETURNS: Solar hour of sunrise.
// SOURCE:  Duffy & Beckman.
//
double SolarAngles::hr_sunrise(
    short day_of_yr,    // solar declination angle (radians)
    double latitude     // latitude (radians)
)
{
    // Calculate hour angle of sunrise (converted to degrees)
    double hr_ang = acos( -tan(latitude)*tan(declination(day_of_yr)) ) / PI_OVER_180;

    // Convert hour angle to solar time
    return( (-hr_ang/15.0) + 12.0 );
}



/*****************function day_len********************************/
// PURPOSE: Compute the length of a given day.
// EXPECTS: Day of year, latitude.
// RETURNS: Day length (hours).
// SOURCE:  Duffy & Beckman.
//
double SolarAngles::day_len(
    short day_of_yr,    // Day of year from Jan 1
    double latitude     // latitude (radians)
)
{
    return 2.0 * acos( -tan(latitude)*tan(declination(day_of_yr)) ) / (15.0 * PI_OVER_180);
}



/*****************function day_of_yr********************************/
// PURPOSE: Compute julian day of year.
// EXPECTS: Month, day, year, all as integers.
// RETURNS: Day length (hours).
//
// This ignores leap years but that's okay...the other functions always
// assume a year has only 365 days.
//
short SolarAngles::day_of_yr(
    short month,        // Month of year (1-12)
    short day       // Day of month (1-31)
)
{

    // TODO:  Error checking on the 1-12 and 1-31...

    return days_thru_month[month-1] + day;
}
