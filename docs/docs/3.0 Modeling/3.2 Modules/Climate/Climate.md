# Climate Module 

**TODO**:  Update for [Hassayampa (Version 3.0)]

The climate module provides an interface that other objects may use to include weather data in their calculations. Objects such as houses and buildings rely on this data to factor outdoor weather into their calculations for internal temperature. The climate data includes temperature, humidity, and solar radiation, which is used to calculate temperature gain that is the result of heat gained from direct exposure of a surface to sunlight. The Climate Module Version 1.0 retrieves climate data from TMY2 files, created and maintained by the National Renewable Energy Laboratory (NREL). 

## Climate Object

The climate object in the climate module is a combined data container and TMY2 file parser. Given a tmy2 file, it will update its contents on an hourly basis from its file. Elsewise, it operates strictly as a constant reference class, with mutable but internally unchanging values. 

### Properties

Property Name  | Unit  | Type  | Default Value  | Description   
---|---|---|---|---  
city  | char32  | N/A  | " "  | The name of the city associated with this climate object.   
humidity  | double  | %  | 0.75  | The current humidity   
record.high  | double  | degF  | 59.0  | The highest air temperature that will be observed in the TMY2 file. Ignored without a TMY2 file.   
record.low  | double  | degF  | 59.0  | The lowest air temperature that will be observed in the TMY2 file. Ignored without a TMY2 file.   
record.solar  | double  | W/sf  | 0.0  | The greatest solar input that will be observed in the TMY2 file. Ignored without a TMY2 file.   
solar_flux  | double  | W/sf  | 0.0  | The current aggregated solar input for a flat surface.   
temperature  | double  | degF  | 59.0  | The current air temperature   
tmyfile  | char1024  | N/A  | " "  | The path to the TMY2 file to use for this object, if any.   
wind_speed  | double  | degrees  | 0.0  | The way the wind blows, 0 deg being northwards   
interpolate  | enumeration  | NONE, LINEAR  | NONE  | Interpolation method to use on the climate data, if any.   
  
### TMY2 Data

TMY is an acronym for typical meteorological year. In a TMY file, weather data for a particular location is aggregated and averaged to provide a typical baseline for the weather of a particular geographical location on a given day at a given hour. TMY data is not suitable for modeling extreme situations and is not necessarily a good indicator for forecasting, but it is an indication of typical weather conditions over an extended period of time. The TMY2 format is an extension of the original TMY format to include information necessary for solar radiation calculations. 

### Solar Radiation

In the climate module, calculations are performed for solar radiation on a surface facing each of the eight major compass points (N, S, E, W, NE, NW, SE, SW) and a horizontal surface. For each surface, the total incident solar radiation is calculated by the following equation: 

$$Q_{solar} = Q_{direct} \cos \left ( \alpha_{incident} \right ) + Q_{diffuse} 
$$

where 

  * $Q_{direct}$ is the direct normal radiation for time and day.
  * $Q_{diffuse}$ is the diffuse horizontal radiation.
  * $A_{incident}$ = the angle of the sun relative to the surface (assuming the surface to be perpendicular to the Earth at that point, excepting the horizontal surface).

The incident angle is calculated by first calculating the solar time, which accounts for the change in the tilt of the earth polar axis with respect to the plane of the orbit around the sun through the year. The solar time is combined with the latitude of the surface, the slope of the surface relative to the horizontal (90° for all surfaces except the horizontal surface), and the azimuth angle relative to south (+ east of south, - west of south), and the day of the year (which is used in a calculation of the solar declination angle) to produce the cosine of the incident angle as follows. 

$$\begin{align} D_{solar} & = 23.45 \deg \frac{2 \pi}{360} \sin \left ( \frac{2 \pi 284 + D_{year}}{365} \right ) \\\ 
    
    
    & = 0.409280 \sin \left ( \frac{2 \pi 284 + D_{year}}{365}  \right )
    

\end{align} 
$$

  


$$A_{hour} = - \frac{15 \pi}{180} \left ( H_{solar} - 12 \right ) 
$$

  


$$\begin{align} \cos(A_{incident}) & = \sin(D_{solar}) \sin(L) cos(S) \\\ 
    
    
    & - \sin(D_{solar}) \cos(L) \sin(S) \cos(Z) \\
    & + \cos(D_{solar}) \cos(L) \cos(S) \cos(A_{hour}) \\
    & + \cos(D_{solar}) \sin(L) \sin(S) \cos(Z) \cos(A_{hour}) \\
    & + \cos(D_{solar}) \sin(S) \sin(Z) \sin(A_{hour})
    

\end{align} 
$$

where: 

  * $S$ is the slope of the incident surface (90° is vertical);
  * $Z$ is the surface azimuth angle (angle between the incident surface's origanization (zero is true south, east is positive, west is negative);
  * $L$ is the latitude (north is positive);
  * $A_{hour}$ is hour angle, solar noon is zero, and each hour represents 15° of longitude with mornings positive and afternoons negative;
  * $D_{solar}$ is the declination of the sun (the angular position of the sun at solar noon with respect to the plane of the equator).
Leap years are handled by the fact that an hour of year calculation on February 29 would result in the same hour of year as March 1 on a normal year. March 1 would be used twice in a simulation involving a leap year. 

#### References

John A Duffie and William A Beckman, "Solar Energy Thermal Processes," John Wiley & Sons, 1974 



Author: Nathan Tenney, Pacific Northwest National Laboratory, Richland, Washington (USA), PNNL-17615, May 2008 


# Climate (class) 

**TODO: _This page is imcomplete_**  

The Climate class is the top-level object for weather data storage, retrieval, and parsing. It can be controlled manually, can parse TMY2 or CSV files, and interpolate data points. The climate object reads a specified file at init-time, if one is provided. During the sync steps, the current weather data is posted based on the current data sample and, if it is being interpolated, the next data sample. Solar data is calculated from the location of the city and the time of the year to determine the appropriate azimuth and incident angle of the direct solar radiation, plus the diffuse solar radiation. Solar input is calculated for each of the cardinal and intercardinal directions is calculated, as well as the horizontal direction. 

## Climate actors

### CSV_reader object

If a climate object is reading a CSV file, it will reference a [CSV_reader] object using the "reader" property, and define "tmyfile" with a filename that ends in ".csv". 

## Properties

* **interpolate** -
If the reader is using fixed-period data, such as that from a TMY2 file, the climate object is able to continuously interpolate data points during the period of a sample. 

* **NONE** -
No interpolation will be done. 

* **LINEAR** -
Data values are calculated given a constant rate of change from one data point to the next. 

* **QUADRATIC** -
Data values are calculated using the next two points with classic quadratic interpolation. 

* **city** -
The name of the city the weather data was recorded from. 

* **tmyfile** -
The name of the input file. Not required to be a TMY2 file, given an appropriate reader. 

* **Weather Data** -
All values are either read from the input source or interpolated for this timestep. 

* **temperature** -
The current temperature value, in degrees Fahrenheit. 

* **humidity** -
The current humidity level, as a percentage. 

* **solar_flux** -
The array of solar input values, starting with the horizontal, followed by the north-facing input, then going clockwise around the next seven cardinal and intercardinal directions. 

* **solar_direct** -
The direct solar input, as measured in a 7 degree cone pointed at the sun. Read in watts per square meter. 

* **wind_speed** -

* **rainfall** -

* **snowdepth** -

* **Record Weather Data** -

* **record.low** -

* **record.low_day** -

* **record.high** -

* **record.high_day** -

* **record.solar** -

* **Solar Data** -

* **Functions** -
The climate class has no published functions. 


### Climate Data

Climate data is obtained from the National Renewable Energy Laboratory ([NREL](http://www.nrel.gov/)) [NSRDB: National Solar Radiation Database](https://nsrdb.nrel.gov/) page. 

#### United States

GridLAB-D supports reading [TMY2](https://www.nrel.gov/docs/legosti/old/7668.pdf) or [TMY3](https://www.nrel.gov/docs/fy08osti/43156.pdf) formats. Sample TMY2 and TMY3 data for U.S. locations is available on the GridLAB-D repository at [Climate module TMY repository](https://github.com/gridlab-d/data/). US Data files are named with a two digit state designation followed by the city name, e.g., 
    
    
     OR-Astoria.tmy2
    

For most states and cities, a GLM file is also created to facilitate loading the TMY2. These files can be included in any GLM file: 
    
    
     #include "OR-Astoria.glm"
    

To use modern exports from the NREL NSRDB website, they must be exported in a TMY3-compatible CSV format, or converted to TMY2. 

#### International

There is currently no standard for supporting international weather data. 
