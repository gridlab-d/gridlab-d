/** $Id: solar_angles.cpp 4738 2014-07-03 00:55:39Z dchassin $
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
// Corrected for West negative longitudes
//
double SolarAngles::solar_time(
    double std_time,    // Local standard time in decimal hours (e.g., 7:30 is 7.5)
    short day_of_yr,    // Day of year from Jan 1
    double std_meridian,    // Standard meridian (longitude) of local time zone (radians---e.g., Pacific timezone is 120 degrees times pi/180)
    double longitude    // Local longitude (radians east)
)
{
	return std_time + eq_time(day_of_yr) + HR_PER_RADIAN * (longitude-std_meridian);
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
    double longitude    // Local longitude (radians east)
)
{
	//std_meridian and longitude swapped to account for negative west convention (see solar_time above)
	//Unchecked since nothing uses local_time
    return sol_time - eq_time(day_of_yr) - HR_PER_RADIAN * (longitude-std_meridian);
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

//sjin: add solar elevation funcion
double SolarAngles::elevation(
    short day_of_yr,    // day of year from Jan 1
    double latitude,    // latitude (radians)
    double sol_time     // solar time (decimal hours)
)
{
    double hr_ang = -(15.0 * PI_OVER_180)*(sol_time-12.0); // morning +, afternoon -

    double decl = declination(day_of_yr);

    return asin(sin(decl)*sin(latitude) + cos(decl)*cos(latitude)*cos(hr_ang));
}

//sjin: add solar azimuth funcion
double SolarAngles::azimuth(
    short day_of_yr,    // day of year from Jan 1
    double latitude,    // latitude (radians)
    double sol_time     // solar time (decimal hours)
)
{
    double hr_ang = -(15.0 * PI_OVER_180)*(sol_time-12.0); // morning +, afternoon -

    double decl = declination(day_of_yr);

	double alpha = (90.0 * PI_OVER_180) - latitude + decl;

    return acos( (sin(decl)*cos(latitude) - cos(decl)*sin(latitude)*cos(hr_ang))/cos(alpha) );
}

//Most additions below here are from the NREL Solar Position algorithm 2.0
//http://rredc.nrel.gov/solar/codesandalgorithms/solpos/aboutsolpos.html
//Perez model function at the end (perez_tilt) extracted from referenced paper
/*============================================================================
*    Contains:
*        S_solpos     (computes solar position and intensity
*                      from time and place)
*
*            INPUTS:     (via posdata struct) year, daynum, hour,
*                        minute, second, latitude, longitude, timezone,
*                        intervl
*            OPTIONAL:   (via posdata struct) month, day, press, temp, tilt,
*                        aspect, function
*            OUTPUTS:    EVERY variable in the SOLPOS_POSDATA
*                            (defined in solpos.h)
*
*                       NOTE: Certain conditions exist during which some of
*                       the output variables are undefined or cannot be
*                       calculated.  In these cases, the variables are
*                       returned with flag values indicating such.  In other
*                       cases, the variables may return a realistic, though
*                       invalid, value. These variables and the flag values
*                       or invalid conditions are listed below:
*
*                       amass     -1.0 at zenetr angles greater than 93.0
*                                 degrees
*                       ampress   -1.0 at zenetr angles greater than 93.0
*                                 degrees
*                       azim      invalid at zenetr angle 0.0 or latitude
*                                 +/-90.0 or at night
*                       elevetr   limited to -9 degrees at night
*                       etr       0.0 at night
*                       etrn      0.0 at night
*                       etrtilt   0.0 when cosinc is less than 0
*                       prime     invalid at zenetr angles greater than 93.0
*                                 degrees
*                       sretr     +/- 2999.0 during periods of 24 hour sunup or
*                                 sundown
*                       ssetr     +/- 2999.0 during periods of 24 hour sunup or
*                                 sundown
*                       ssha      invalid at the North and South Poles
*                       unprime   invalid at zenetr angles greater than 93.0
*                                 degrees
*                       zenetr    limited to 99.0 degrees at night
*
*        S_init       (optional initialization for all input parameters in
*                      the posdata struct)
*           INPUTS:     SOLPOS_POSDATA*
*           OUTPUTS:    SOLPOS_POSDATA*
*
*                     (Note: initializes the required S_solpos INPUTS above
*                      to out-of-bounds conditions, forcing the user to
*                      supply the parameters; initializes the OPTIONAL
*                      S_solpos inputs above to nominal values.)
*
*       S_decode      (optional utility for decoding the S_solpos return code)
*           INPUTS:     long integer S_solpos return value, SOLPOS_POSDATA*
*           OUTPUTS:    text to stderr
*
*    Usage:
*         In calling program, just after other 'includes', insert:
*
*              #include "solpos00.h"
*
*         Function calls:
*              S_init(SOLPOS_POSDATA*)  [optional]
*              .
*              .
*              [set time and location parameters before S_solpos call]
*              .
*              .
*              int retval = S_solpos(SOLPOS_POSDATA*)
*              S_decode(int retval, SOLPOS_POSDATA*) [optional]
*                  (Note: you should always look at the S_solpos return
*                   value, which contains error codes. S_decode is one option
*                   for examining these codes.  It can also serve as a
*                   template for building your own application-specific
*                   decoder.)
*
*    Martin Rymes
*    National Renewable Energy Laboratory
*    25 March 1998
*
*    27 April 1999 REVISION:  Corrected leap year in S_date.
*    13 January 2000 REVISION:  SMW converted to structure posdata parameter
*                               and subdivided into functions.
*    01 February 2001 REVISION: SMW corrected ecobli calculation
*                               (changed sign). Error is small (max 0.015 deg
*                               in calculation of declination angle)
*----------------------------------------------------------------------------*/

/*============================================================================
*    Long integer function S_solpos, adapted from the VAX solar libraries
*
*    This function calculates the apparent solar position and the
*    intensity of the sun (theoretical maximum solar energy) from
*    time and place on Earth.
*
*    Requires (from the SOLPOS_POSDATA parameter):
*        Date and time:
*            year
*            daynum   (requirement depends on the S_DOY switch)
*            month    (requirement depends on the S_DOY switch)
*            day      (requirement depends on the S_DOY switch)
*            hour
*            minute
*            second
*            interval  DEFAULT 0
*        Location:
*            latitude
*            longitude
*        Location/time adjuster:
*            timezone
*        Atmospheric pressure and temperature:
*            press     DEFAULT 1013.0 mb
*            temp      DEFAULT 10.0 degrees C
*        Tilt of flat surface that receives solar energy:
*            aspect    DEFAULT 180 (South)
*            tilt      DEFAULT 0 (Horizontal)
*        Function Switch (codes defined in solpos.h)
*            function  DEFAULT S_ALL
*
*    Returns (via the SOLPOS_POSDATA parameter):
*        everything defined in the SOLPOS_POSDATA in solpos.h.
*----------------------------------------------------------------------------*/
int SolarAngles::S_solpos(SOLPOS_POSDATA *pdat)
{
	SOLPOS_TRIGDATA trigdat, *tdat;

	tdat = &trigdat;   // point to the structure

	// initialize the trig structure
	trigdat.sd = -999.0; // flag to force calculation of trig data
	trigdat.cd =    1.0;
	trigdat.ch =    1.0; // set the rest of these to something safe
	trigdat.cl =    1.0;
	trigdat.sl =    1.0;

    doy2dom(pdat);                	/* convert input doy to month-day */
    geometry(pdat);               	/* do basic geometry calculations */
	zen_no_ref(pdat,tdat);			/* etr at non-refracted zenith angle */
    ssha(pdat,tdat);				/* Sunset hour calculation */
    sbcf(pdat,tdat);				/* Shadowband correction factor */
	tst(pdat);						/* true solar time */
    srss(pdat);						/* sunrise/sunset calculations */
    sazm(pdat,tdat);				/* solar azimuth calculations */
	refrac(pdat);					/* atmospheric refraction calculations */
	amass(pdat);					/* airmass calculations */
	prime(pdat);					/* kt-prime/unprime calculations */
	etr(pdat);						/* ETR and ETRN (refracted) */
	tilt(pdat);						/* tilt calculations */
	perez_tilt(pdat);				/* Perez tilt calculations */

    return 0;
}

/*============================================================================
*    Void function S_init
*
*    This function initiates all of the input parameters in the struct
*    posdata passed to S_solpos().  Initialization is either to nominal
*    values or to out of range values, which forces the calling program to
*    specify parameters.
*
*    NOTE: This function is optional if you initialize ALL input parameters
*          in your calling code.  Note that the required parameters of date
*          and location are deliberately initialized out of bounds to force
*          the user to enter real-world values.
*
*    Requires: Pointer to a posdata structure, members of which are
*           initialized.
*
*    Returns: Void
*----------------------------------------------------------------------------*/
void SolarAngles::S_init(SOLPOS_POSDATA *pdat)
{
  pdat->day       =    -99;   // Day of month (May 27 = 27, etc.)
  pdat->daynum    =   -999;   // Day number (day of year; Feb 1 = 32 )
  pdat->hour      =    -99;   // Hour of day, 0 - 23
  pdat->minute    =    -99;   // Minute of hour, 0 - 59
  pdat->month     =    -99;   // Month number (Jan = 1, Feb = 2, etc.)
  pdat->second    =    -99;   // Second of minute, 0 - 59
  pdat->year      =    -99;   // 4-digit year
  pdat->interval  =      0;   // instantaneous measurement interval
  pdat->aspect    =     PI;	  // Azimuth of panel surface (direction it
                              //      faces) N=0, E=90, S=180, W=270 - converted to radians
  pdat->latitude  =  -99.0;   // Latitude, degrees north (south negative)
  pdat->longitude = -999.0;   // Longitude, degrees east (west negative)
  pdat->press     = 1013.0;   // Surface pressure, millibars
  pdat->solcon    = 126.998456;   // Solar constant, 1367 W/sq m - 126.998456; W/sq ft
  pdat->temp      =   15.0;   // Ambient dry-bulb temperature, degrees C
  pdat->tilt      =    0.0;   // Radians tilt from horizontal of panel
  pdat->timezone  =  -99.0;   // Time zone, east (west negative).
  pdat->sbwid     =    7.6;   // Eppley shadow band width
  pdat->sbrad     =   31.7;   // Eppley shadow band radius
  pdat->sbsky     =   0.04;   // Drummond factor for partly cloudy skies
  pdat->perez_horz = 1.0;	  // By default, perez_horz is full diffuse
}

/*============================================================================
*    Local void function doy2dom
*
*    This function computes the month/day from the day number.
*
*    Requires (from SOLPOS_POSDATA parameter):
*        Year and day number:
*            year
*            daynum
*
*    Returns (via the SOLPOS_POSDATA parameter):
*            year
*            month
*            day
*----------------------------------------------------------------------------*/
void SolarAngles::doy2dom(SOLPOS_POSDATA *pdat)
{
  int  imon;  // Month (month_days) array counter
  int  leap;  // leap year switch

    // Set the leap year switch
    if ( ((pdat->year % 4) == 0) &&
         ( ((pdat->year % 100) != 0) || ((pdat->year % 400) == 0) ) )
        leap = 1;
    else
        leap = 0;

    // Find the month
    imon = 12;
    while ( pdat->daynum <= month_days [leap][imon] )
        --imon;

    // Set the month and day of month
    pdat->month = imon;
    pdat->day   = pdat->daynum - month_days[leap][imon];
}


/*============================================================================
*    Local Void function geometry
*
*    Does the underlying geometry for a given time and location
*----------------------------------------------------------------------------*/
void SolarAngles::geometry ( SOLPOS_POSDATA *pdat )
{
  double bottom;      // denominator (bottom) of the fraction
  double c2;          // cosine of d2
  double cd;          // cosine of the day angle or delination
  double d2;          // pdat->dayang times two
  double delta;       // difference between current year and 1949
  double s2;          // sine of d2
  double sd;          // sine of the day angle
  double top;         // numerator (top) of the fraction
  int   leap;        // leap year counter

  /* Day angle */
      /*  Iqbal, M.  1983.  An Introduction to Solar Radiation.
            Academic Press, NY., page 3 */
     pdat->dayang = 2.0 * PI * ( pdat->daynum - 1 ) / 365.0;

    /* Earth radius vector * solar constant = solar energy */
        /*  Spencer, J. W.  1971.  Fourier series representation of the
            position of the sun.  Search 2 (5), page 172 */
    sd     = sin (pdat->dayang);
    cd     = cos (pdat->dayang);
    d2     = 2.0 * pdat->dayang;
    c2     = cos (d2);
    s2     = sin (d2);

    pdat->erv  = 1.000110 + 0.034221 * cd + 0.001280 * sd;
    pdat->erv  += 0.000719 * c2 + 0.000077 * s2;

    /* Universal Coordinated (Greenwich standard) time */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->utime =
        pdat->hour * 3600.0 +
        pdat->minute * 60.0 +
        pdat->second -
        (double)pdat->interval / 2.0;
    pdat->utime = pdat->utime / 3600.0 - pdat->timezone;

    /* Julian Day minus 2,400,000 days (to eliminate roundoff errors) */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */

    /* No adjustment for century non-leap years since this function is
       bounded by 1950 - 2050 */
    delta    = pdat->year - 1949;
    leap     = (int) ( delta / 4.0 );
    pdat->julday =
        32916.5 + delta * 365.0 + leap + pdat->daynum + pdat->utime / 24.0;

    /* Time used in the calculation of ecliptic coordinates */
    /* Noon 1 JAN 2000 = 2,400,000 + 51,545 days Julian Date */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->ectime = pdat->julday - 51545.0;

    /* Mean longitude */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->mnlong  = 280.460 + 0.9856474 * pdat->ectime;

    /* (dump the multiples of 360, so the answer is between 0 and 360) */
    pdat->mnlong -= 360.0 * (int) ( pdat->mnlong / 360.0 );
    if ( pdat->mnlong < 0.0 )
        pdat->mnlong += 360.0;

    /* Mean anomaly */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->mnanom  = 357.528 + 0.9856003 * pdat->ectime;

    /* (dump the multiples of 360, so the answer is between 0 and 360) */
    pdat->mnanom -= 360.0 * (int) ( pdat->mnanom / 360.0 );
    if ( pdat->mnanom < 0.0 )
        pdat->mnanom += 360.0;

    /* Ecliptic longitude */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->eclong  = pdat->mnlong + 1.915 * sin ( pdat->mnanom * raddeg ) +
                    0.020 * sin ( 2.0 * pdat->mnanom * raddeg );

    /* (dump the multiples of 360, so the answer is between 0 and 360) */
    pdat->eclong -= 360.0 * (int) ( pdat->eclong / 360.0 );
    if ( pdat->eclong < 0.0 )
        pdat->eclong += 360.0;

    /* Obliquity of the ecliptic */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */

    /* 02 Feb 2001 SMW corrected sign in the following line */
/*  pdat->ecobli = 23.439 + 4.0e-07 * pdat->ectime;     */
    pdat->ecobli = 23.439 - 4.0e-07 * pdat->ectime;

    /* Declination */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->declin = asin(sin(pdat->ecobli*raddeg)*sin(pdat->eclong*raddeg));

    /* Right ascension */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    top      =  cos ( raddeg * pdat->ecobli ) * sin ( raddeg * pdat->eclong );
    bottom   =  cos ( raddeg * pdat->eclong );

    pdat->rascen =  degrad * atan2 ( top, bottom );

    /* (make it a positive angle) */
    if ( pdat->rascen < 0.0 )
        pdat->rascen += 360.0;

    /* Greenwich mean sidereal time */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->gmst  = 6.697375 + 0.0657098242 * pdat->ectime + pdat->utime;

    /* (dump the multiples of 24, so the answer is between 0 and 24) */
    pdat->gmst -= 24.0 * (int) ( pdat->gmst / 24.0 );
    if ( pdat->gmst < 0.0 )
        pdat->gmst += 24.0;

    /* Local mean sidereal time */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->lmst  = pdat->gmst * 15.0 + pdat->longitude;

    /* (dump the multiples of 360, so the answer is between 0 and 360) */
    pdat->lmst -= 360.0 * (int) ( pdat->lmst / 360.0 );
    if ( pdat->lmst < 0.)
        pdat->lmst += 360.0;

    /* Hour angle */
        /*  Michalsky, J.  1988.  The Astronomical Almanac's algorithm for
            approximate solar position (1950-2050).  Solar Energy 40 (3),
            pp. 227-235. */
    pdat->hrang = pdat->lmst - pdat->rascen;

    /* (force it between -180 and 180 degrees) */
    if ( pdat->hrang < -180.0 )
        pdat->hrang += 360.0;
    else if ( pdat->hrang > 180.0 )
        pdat->hrang -= 360.0;
}


/*============================================================================
*    Local Void function zen_no_ref
*
*    ETR solar zenith angle
*       Iqbal, M.  1983.  An Introduction to Solar Radiation.
*            Academic Press, NY., page 15
*----------------------------------------------------------------------------*/
void SolarAngles::zen_no_ref ( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat )
{
  double cz;          // cosine of the solar zenith angle

	tdat->cd = cos(pdat->declin);
    tdat->ch = cos(raddeg*pdat->hrang);
    tdat->cl = cos(pdat->latitude);
    tdat->sd = sin(pdat->declin);
    tdat->sl = sin(pdat->latitude);

	cz = tdat->sd * tdat->sl + tdat->cd * tdat->cl * tdat->ch;

    // (watch out for the roundoff errors)
    if ( fabs (cz) > 1.0 ) {
        if ( cz >= 0.0 )
            cz =  1.0;
        else
            cz = -1.0;
    }

    pdat->zenetr   = acos ( cz ) * degrad;

    // (limit the degrees below the horizon to 9 [+90 -> 99])
    if ( pdat->zenetr > 99.0 )
        pdat->zenetr = 99.0;

    pdat->elevetr = 90.0 - pdat->zenetr;
}


/*============================================================================
*    Local Void function ssha
*
*    Sunset hour angle, degrees
*       Iqbal, M.  1983.  An Introduction to Solar Radiation.
*            Academic Press, NY., page 16
*----------------------------------------------------------------------------*/
void SolarAngles::ssha( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat )
{
  double cssha;       // cosine of the sunset hour angle
  double cdcl;        // ( cd * cl )

    cdcl    = tdat->cd * tdat->cl;

    if ( fabs ( cdcl ) >= 0.001 ) {
        cssha = -tdat->sl * tdat->sd / cdcl;

        // This keeps the cosine from blowing on roundoff
        if ( cssha < -1.0  )
            pdat->ssha = 180.0;
        else if ( cssha > 1.0 )
            pdat->ssha = 0.0;
        else
            pdat->ssha = degrad * acos ( cssha );
    }
    else if ( ((pdat->declin >= 0.0) && (pdat->latitude > 0.0 )) ||
              ((pdat->declin <  0.0) && (pdat->latitude < 0.0 )) )
        pdat->ssha = 180.0;
    else
        pdat->ssha = 0.0;
}


/*============================================================================
*    Local Void function sbcf
*
*    Shadowband correction factor
*       Drummond, A. J.  1956.  A contribution to absolute pyrheliometry.
*            Q. J. R. Meteorol. Soc. 82, pp. 481-493
*----------------------------------------------------------------------------*/
void SolarAngles::sbcf( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat )
{
  double p, t1, t2;   // used to compute sbcf

    p       = 0.6366198 * pdat->sbwid / pdat->sbrad * pow (tdat->cd,3);
    t1      = tdat->sl * tdat->sd * pdat->ssha * raddeg;
    t2      = tdat->cl * tdat->cd * sin ( pdat->ssha * raddeg );
    pdat->sbcf = pdat->sbsky + 1.0 / ( 1.0 - p * ( t1 + t2 ) );

}


/*============================================================================
*    Local Void function tst
*
*    TST -> True Solar Time = local standard time + TSTfix, time
*      in minutes from midnight.
*        Iqbal, M.  1983.  An Introduction to Solar Radiation.
*            Academic Press, NY., page 13
*----------------------------------------------------------------------------*/
void SolarAngles::tst( SOLPOS_POSDATA *pdat )
{
    pdat->tst    = ( 180.0 + pdat->hrang ) * 4.0;
    pdat->tstfix =
        pdat->tst -
        (double)pdat->hour * 60.0 -
        pdat->minute -
        (double)pdat->second / 60.0 +
        (double)pdat->interval / 120.0; // add back half of the interval

    // bound tstfix to this day
    while ( pdat->tstfix >  720.0 )
        pdat->tstfix -= 1440.0;
    while ( pdat->tstfix < -720.0 )
        pdat->tstfix += 1440.0;

    pdat->eqntim =
        pdat->tstfix + 60.0 * pdat->timezone - 4.0 * pdat->longitude;

}


/*============================================================================
*    Local Void function srss
*
*    Sunrise and sunset times (minutes from midnight)
*----------------------------------------------------------------------------*/
void SolarAngles::srss( SOLPOS_POSDATA *pdat )
{
    if ( pdat->ssha <= 1.0 ) {
        pdat->sretr   =  2999.0;
        pdat->ssetr   = -2999.0;
    }
    else if ( pdat->ssha >= 179.0 ) {
        pdat->sretr   = -2999.0;
        pdat->ssetr   =  2999.0;
    }
    else {
        pdat->sretr   = 720.0 - 4.0 * pdat->ssha - pdat->tstfix;
        pdat->ssetr   = 720.0 + 4.0 * pdat->ssha - pdat->tstfix;
    }
}


/*============================================================================
*    Local Void function sazm
*
*    Solar azimuth angle
*       Iqbal, M.  1983.  An Introduction to Solar Radiation.
*            Academic Press, NY., page 15
*----------------------------------------------------------------------------*/
void SolarAngles::sazm( SOLPOS_POSDATA *pdat, SOLPOS_TRIGDATA *tdat )
{
  double ca;          // cosine of the solar azimuth angle
  double ce;          // cosine of the solar elevation
  double cecl;        // ( ce * cl )
  double se;          // sine of the solar elevation

    ce         = cos ( raddeg * pdat->elevetr );
    se         = sin ( raddeg * pdat->elevetr );

    pdat->azim     = 180.0;
    cecl       = ce * tdat->cl;
    if ( fabs ( cecl ) >= 0.001 ) {
        ca     = ( se * tdat->sl - tdat->sd ) / cecl;
        if ( ca > 1.0 )
            ca = 1.0;
        else if ( ca < -1.0 )
            ca = -1.0;

        pdat->azim = 180.0 - acos ( ca ) * degrad;
        if ( pdat->hrang > 0 )
            pdat->azim  = 360.0 - pdat->azim;
    }
}


/*============================================================================
*    Local Int function refrac
*
*    Refraction correction, degrees
*        Zimmerman, John C.  1981.  Sun-pointing programs and their
*            accuracy.
*            SAND81-0761, Experimental Systems Operation Division 4721,
*            Sandia National Laboratories, Albuquerque, NM.
*----------------------------------------------------------------------------*/
void SolarAngles::refrac( SOLPOS_POSDATA *pdat )
{
  double prestemp;    // temporary pressure/temperature correction
  double refcor;      // temporary refraction correction
  double tanelev;     // tangent of the solar elevation angle

    // If the sun is near zenith, the algorithm bombs; refraction near 0
    if ( pdat->elevetr > 85.0 )
        refcor = 0.0;

    // Otherwise, we have refraction
    else {
        tanelev = tan ( raddeg * pdat->elevetr );
        if ( pdat->elevetr >= 5.0 )
            refcor  = 58.1 / tanelev -
                      0.07 / ( pow (tanelev,3) ) +
                      0.000086 / ( pow (tanelev,5) );
        else if ( pdat->elevetr >= -0.575 )
            refcor  = 1735.0 +
                      pdat->elevetr * ( -518.2 + pdat->elevetr * ( 103.4 +
                      pdat->elevetr * ( -12.79 + pdat->elevetr * 0.711 ) ) );
        else
            refcor  = -20.774 / tanelev;

        prestemp    =
            ( pdat->press * 283.0 ) / ( 1013.0 * ( 273.0 + pdat->temp ) );
        refcor     *= prestemp / 3600.0;
    }

    // Refracted solar elevation angle
    pdat->elevref = pdat->elevetr + refcor;

    // (limit the degrees below the horizon to 9)
    if ( pdat->elevref < -9.0 )
        pdat->elevref = -9.0;

    // Refracted solar zenith angle
    pdat->zenref  = 90.0 - pdat->elevref;
    pdat->coszen  = cos( raddeg * pdat->zenref );
}


/*============================================================================
*    Local Void function  amass
*
*    Airmass
*       Kasten, F. and Young, A.  1989.  Revised optical air mass
*            tables and approximation formula.  Applied Optics 28 (22),
*            pp. 4735-4738
*----------------------------------------------------------------------------*/
void SolarAngles::amass( SOLPOS_POSDATA *pdat )
{
    if ( pdat->zenref > 93.0 )
    {
        pdat->amass   = -1.0;
        pdat->ampress = -1.0;
    }
    else
    {
        pdat->amass =
            1.0 / ( cos (raddeg * pdat->zenref) + 0.50572 *
            pow ((96.07995 - pdat->zenref),-1.6364) );

        pdat->ampress   = pdat->amass * pdat->press / 1013.0;
    }
}

/*============================================================================
*    Local Void function prime
*
*    Prime and Unprime
*    Prime  converts Kt to normalized Kt', etc.
*       Unprime deconverts Kt' to Kt, etc.
*            Perez, R., P. Ineichen, Seals, R., & Zelenka, A.  1990.  Making
*            full use of the clearness index for parameterizing hourly
*            insolation conditions. Solar Energy 45 (2), pp. 111-114
*----------------------------------------------------------------------------*/
void SolarAngles::prime( SOLPOS_POSDATA *pdat )
{
    pdat->unprime = 1.031 * exp ( -1.4 / ( 0.9 + 9.4 / pdat->amass ) ) + 0.1;
    pdat->prime   = 1.0 / pdat->unprime;
}

/*============================================================================
*    Local Void function etr
*
*    Extraterrestrial (top-of-atmosphere) solar irradiance
*----------------------------------------------------------------------------*/
void SolarAngles::etr( SOLPOS_POSDATA *pdat )
{
    if ( pdat->coszen > 0.0 ) {
        pdat->etrn = pdat->solcon * pdat->erv;
        pdat->etr  = pdat->etrn * pdat->coszen;
    }
    else {
        pdat->etrn = 0.0;
        pdat->etr  = 0.0;
    }
}

/*============================================================================
*    Local Void function tilt
*
*    ETR on a tilted surface
*----------------------------------------------------------------------------*/
void SolarAngles::tilt( SOLPOS_POSDATA *pdat )
{
  double ca;          // cosine of the solar azimuth angle
  double cp;          // cosine of the panel aspect
  double ct;          // cosine of the panel tilt
  double sa;          // sine of the solar azimuth angle
  double sp;          // sine of the panel aspect
  double st;          // sine of the panel tilt
  double sz;          // sine of the refraction corrected solar zenith angle


    // Cosine of the angle between the sun and a tipped flat surface,
    //  useful for calculating solar energy on tilted surfaces
    ca      = cos ( raddeg * pdat->azim );
    cp      = cos (pdat->aspect);
    ct      = cos (pdat->tilt);
    sa      = sin ( raddeg * pdat->azim );
    sp      = sin (pdat->aspect);
    st      = sin (pdat->tilt);
    sz      = sin ( raddeg * pdat->zenref );
    pdat->cosinc  = pdat->coszen * ct + sz * st * ( ca * cp + sa * sp );

    if ( pdat->cosinc > 0.0 )
        pdat->etrtilt = pdat->etrn * pdat->cosinc;
    else
        pdat->etrtilt = 0.0;

}

//Perez diffuse radiation tilt model
//Extracted from Perez et al., "Modeling Dayling Availability and Irradiance Components from Direct and
//Global Irradiance," Solar Energy, vol. 44, no. 7, pp. 271-289, 1990.
void SolarAngles::perez_tilt( SOLPOS_POSDATA *pdat )
{
	double zen_rad, zen_rad_cubed, a_val, b_val;
	short index;

	//See if there's even any diffuse radiation to care about - i.e., is it not night
	if (pdat->diff_horz != 0.0)
	{
		//Put the zenith angle into radians so it works with the Perez model
		zen_rad = pdat->zenref * PI_OVER_180;

		//Make a cube of it and scale it since we'll need it in a couple places
		//Uses constant of 1.041 for zenith in radians (per Perez et al. 1990)
		zen_rad_cubed = zen_rad * zen_rad * zen_rad * 1.041;

		//Compute sky clearness - if there's no direct on the surface, it's 1.0 (save some calculations)
		if (pdat->dir_norm != 0.0)
			pdat->perez_skyclear = ((pdat->diff_horz + pdat->dir_norm)/pdat->diff_horz + zen_rad_cubed)/(1 + zen_rad_cubed);
		else
			pdat->perez_skyclear = 1.0;

		//Flag the index
		pdat->perez_skyclear_idx = -1;

		//Index out the sky clearness
		for (index=0; index<7; index++)
		{
			if ((pdat->perez_skyclear>=perez_clearness_limits[index]) && (pdat->perez_skyclear<perez_clearness_limits[index+1]))
			{
				pdat->perez_skyclear_idx = index;	//Indices are offset by one since they'll be used as indices later
				break;	//Get us out once we're assigned
			}
		}

		//If exited and is still -1, means it must be "clear" (index region 8)
		if (pdat->perez_skyclear_idx == -1)
			pdat->perez_skyclear_idx = 7;	//0 referened - 8th bin

		//Calculate the sky brightness
		if (pdat->etrn == 0.0)
			pdat->perez_brightness = 0.0;
		else
			pdat->perez_brightness = (pdat->diff_horz*pdat->amass/pdat->etrn);

		//Now calculate the F1 and F2 coefficients
		pdat->perez_F1 = perez_tilt_coeff_F1[0][pdat->perez_skyclear_idx] + perez_tilt_coeff_F1[1][pdat->perez_skyclear_idx]*pdat->perez_brightness + perez_tilt_coeff_F1[2][pdat->perez_skyclear_idx]*zen_rad;
		pdat->perez_F2 = perez_tilt_coeff_F2[0][pdat->perez_skyclear_idx] + perez_tilt_coeff_F2[1][pdat->perez_skyclear_idx]*pdat->perez_brightness + perez_tilt_coeff_F2[2][pdat->perez_skyclear_idx]*zen_rad;

		//Maximum check
		if (pdat->perez_F1 < 0)
			pdat->perez_F1 = 0.0;

		//Now compute the Perez horizontal scalar (to be applied to diffuse) - get a & b values first
		if (pdat->cosinc > 0.0)
			a_val = pdat->cosinc;
		else
			a_val = 0.0;

		if (pdat->coszen > COS85DEG)
			b_val = pdat->coszen;
		else
			b_val = COS85DEG;

		//Compute the scalar
		pdat->perez_horz = ((1 - pdat->perez_F1)*(1 + cos(pdat->tilt))/2.0 + pdat->perez_F1*a_val/b_val + pdat->perez_F2*sin(pdat->tilt));
	}
	else	//Must be night, just assign a zero constant
		pdat->perez_horz = 0.0;

}
