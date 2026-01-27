# The Clock Directive (TODO: Is there a better word for "directive"?)
This page explains how to manage simulation time in GridLAB-D in more detail. We start with a reminder of the information presented in TODO: Add reference after we break 2.5.2 - GLM Models. A user can use the clock directive to indicate the start and stop time of a simulation, as well as provide timezone information.

## Using date and time
The timezone `clock directive` determines which timezone to use during the simulation. The timezone must be known before any `timestamps` can be interpreted. The timezone rules are used to determine the offset from UTC for all time calculations, as well as determine daylight or summer time shifts. 

If the timezone is not set, the system will assume all `timestamps` are in local time.

Time zones are specified using the [POSIX timezone standard](http://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html). 

## GLM

The timezone is usually set using the `clock directive` as follows: 
    
    
     clock {
       timezone PST8PDT;
       starttime '2000-01-01 00:00:00 PST';
       stoptime '2001-01-01 00:00:00 PST';
     }
 

As mentioned before, date and time specifications in GridLAB-D™ are usually specified using the ISO format, which is `YYYY-MM-DD HH:MM:SS ZZZ`. In the absence of specific time zone information, GridLAB-D uses the current time zone. Time zones are specified in the `tzinfo.txt` file that is installed with GridLAB-D™ under the `share` folder. 
  
    // Examples:1b.glm
    clock {
      starttime '2000-01-01 00:00:00 UTC';
      stoptime '2001-01-01 00:00:00 UTC';
    }
    module residential;
    module tape;
    object house {
      object recorder {
        property air_temperature;
        file temperature.csv;
      };
    }
    

which produces the following output: 
    
    
    2000-01-01 00:00:00 UTC,+69
    2000-01-01 01:00:00 UTC,+69.9947
    2000-01-01 02:00:00 UTC,+70.6269
    2000-01-01 03:00:00 UTC,+71.1764
    2000-01-01 04:00:00 UTC,+71.6754
    2000-01-01 05:00:00 UTC,+72.1189
    2000-01-01 06:00:00 UTC,+72.5266
    2000-01-01 07:00:00 UTC,+72.927
    ...
    

Dates and times are usually specified using the ISO (International Standard Organization) standard format, which is `YYYY-MM-DD HH:MM:SS ZZZ`. (You can change the format of date/time values using the `dateformat global variable`.) 

If you omit the time zone specification, then the time zone indicated by the `TZ` environment variable will be used. If you wish to specify the timezone to use in the simulation, use the timezone directive: 
    
    // Examples:1c.glm
    clock {
      timezone EST+5EDT;
      starttime '2000-01-01 00:00:00 UTC';
      stoptime '2001-01-01 00:00:00 UTC';
    }
    module residential;
    module tape;
    object house {
      object recorder {
        property air_temperature;
        file temperature.csv;
      };
    }
    

which results in the following output: 
    
    
    2000-01-01 00:00:00 EST,+69
    2000-01-01 01:00:00 EST,+69.9947
    2000-01-01 02:00:00 EST,+70.6269
    2000-01-01 03:00:00 EST,+71.1764
    2000-01-01 04:00:00 EST,+71.6754
    2000-01-01 05:00:00 EST,+72.1189
    2000-01-01 06:00:00 EST,+72.5266
    2000-01-01 07:00:00 EST,+72.927
    2000-01-01 08:00:00 EST,+73.3376
    2000-01-01 09:00:00 EST,+73.7618
    

## Timezones
Time zones are specified in the time zone file `tzinfo.txt` file that is installed with the system in the gridlabd folder. GridLAB-D™ does not use the operating system’s time zone specifications by default for several reasons: 

* some operating systems don’t recognize time zone that are historical but no longer used,
* the simulation often needs to run the simulation in a different time zone than that used by the host computer,
* the ability to use alternate time zone rules is essential to understand the energy use implication of altering the time zone rules, something which policymakers have an interest in and sometimes ask.

In GridLAB-D™ timezones follow the [Posix TZ standard](http://www.gnu.org/s/hello/manual/libc/TZ-Variable.html). Each timezone is described with a string using the form 
    
    
     STZ[hh[:mm][DTZ][,M#[#].#.#/hh:mm,M#[#].#.#/hh:mm]]
    

where STZ is the 3-digit standard time zone specification and DTZ is the 3-digit daylight time zone specification, hh:mm describes the offset from GMT with negative for each and positive for west, the first M-spec describes the month, week and weekday on which daylight savings starts, and the second on which it ends. 

Because GridLAB-D™ must run historical simulations, the timezone rules may change from year to year. Consequently, there are different section specify the year in which the timezone specification goes into effect. Each year section is described with the string 
    
    
    [YYYY]
    

The following timezones are currently supported in `tzinfo.txt` for the United States: 
    
    
    UTC0 ; Coordinated Universal Time ~ never uses DST
    GMT0 ; Greenwich Mean Time, no DST
    EST5 ; Eastern no DST
    CST6 ; Central no DST
    MST7 ; Mountain no DST
    PST8 ; Pacific no DST 
    
    [1970] ; Rules as of 1967
    GMT0GMT,M3.5.0/02:00,M10.5.0/2:00 ; GMT, DST last Sun/Mar to last Sun/Oct
    EST+5EDT,M4.5.0/02:00,M10.5.0/02:00 ; Eastern, DST last Sun/Apr to last Sun/Oct
    CST+6CDT,M4.5.0/02:00,M10.5.0/02:00 ; Central, DST last Sun/Apr to last Sun/Oct
    MST+7MDT,M4.5.0/02:00,M10.5.0/02:00 ; Mountain, DST last Sun/Apr to last Sun/Oct
    PST+8PDT,M4.5.0/02:00,M10.5.0/02:00 ; Pacific, DST last Sun/Apr to last Sun/Oct
    
    [1986] ; Rules as of 1986
    EST+5EDT,M4.1.0/02:00,M10.5.0/02:00 ; Eastern, DST first Sun/Apr to last Sun/Oct
    CST+6CDT,M4.1.0/02:00,M10.5.0/02:00 ; Central, DST first Sun/Apr to last Sun/Oct
    MST+7MDT,M4.1.0/02:00,M10.5.0/02:00 ; Mountain, DST first Sun/Apr to last Sun/Oct
    PST+8PDT,M4.1.0/02:00,M10.5.0/02:00 ; Pacific, DST first Sun/Apr to last Sun/Oct
    
    [2007] ; Rules as of 2007
    EST+5EDT,M3.2.0/02:00,M11.1.0/02:00 ; Eastern, DST second Sun/Mar to first Sun/Nov
    CST+6CDT,M3.2.0/02:00,M11.1.0/02:00 ; Central, DST second Sun/Mar to first Sun/Nov
    MST+7MDT,M3.2.0/02:00,M11.1.0/02:00 ; Mountain, DST second Sun/Mar to first Sun/Nov
    PST+8PDT,M3.2.0/02:00,M11.1.0/02:00 ; Pacific, DST second Sun/Mar to first Sun/Nov

# Synopsis
    
    
    clock {
      timezone _tz-spec_ ;
    }
    

# Environmental variables

The timezone can also be set using the environmental variable `TZ`, if it isn't already set in the GLM file. As an example, the user may set the timezone as `TZ=PST8PDT`, which represents Pacific Time with the Daylight Savings Time option added. If the timezone is not set either using a [clock directive] or the `TZ environment variable`, GridLAB-D™ may be unable to interpret `timestamps` and fatal errors may occur. 

## Locale names

You may also use locale names instead of the timezone codes. Locale names are listed in the `tzinfo` file and take the form 
    
    
    Country/Region/City
    

  
For example, instead of coding 
    
    
    timezone PST+8PDT;
    

you can code 
    
    
    timezone US/CA/Los Angeles;
    

For a listing of country and region codes, see [ISO Std 3166-2](http://en.wikipedia.org/wiki/ISO_3166-2). 

Timezones and daylight-savings/summer time rules can be found at [www.worldtimezone.com](http://http://www.worldtimezone.com/). 


# Caveats

* Most Asia and Africa timezones are not yet implemented in the `tzinfo.txt`.

* It is not clear whether 1/2 and 1/4 hours offset timezones always work properly. 

* Many of the historical rules for summer time around the world are not supported. In some cases the current summer time rules may be inappropriately applied to past years. 

## Disabling and enabling objects

It is possible to have an object activate at a pre-determined date and time, and have deactivate to exist at some time later. Each object has a pair of built-in property called in and out that determine when the object enters service and when to go out of service.

In the following example, we modify `house1.glm` to illustrate how the `in` and `out` properties function. 
    
    
    // Examples:1d.glm
    clock {
      timezone EST+5EDT;
      starttime '2000-01-01 00:00:00';
      stoptime '2001-01-01 00:00:00';
    }
    module residential;
    module tape;
    object house {
      object recorder {
        property air_temperature;
        file temperature.csv;
        in '2000-04-01 00:00:00';
        out '2000-04-02 00:00:00';
      };
    }
    

Running `house1.glm` give the following result: 
    
    
    2000-04-01 00:00:00 EST,+76.0806
    2000-04-01 00:05:02 EST,+73.9952
    2000-04-01 01:00:00 EST,+75.6225
    ...
    2000-04-01 22:00:00 EST,+75.5875
    2000-04-01 23:00:00 EST,+75.8156
    2000-04-02 00:00:00 EST,+75.9208
    

The first major difference is that although the simulation started on January 1, 2000 at midnight, the data was collected starting April 1, 2000 as specified by the `in` property of the recorder. The second difference is that the recorder stopped collecting data after midnight of the April 2, 2000 as specified by the `out` property of the recorder. 
