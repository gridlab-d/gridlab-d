# Metrics collector

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Metrics_collector
# Metrics collector

## Contents

  * 1 Overview
    * 1.1 Attached objects and collected values
    * 1.2 Time interval
  * 2 Synopsis
    * 2.1 See also
# 

Overview

This document describes GridLAB-D implementation of metrics_collector. Metrics_collector class is created as part of the project [Transactive Energy Simulation Platform](http://tesp.readthedocs.io/en/latest/index.html). During the simulation, this class collects the properties of the attached objects, and aggregate the values over predefined time interval (e.g. 5 minutes, 1 hour). Another class [metrics_collector_writer] loops through each metrics_collector at predefined time interval, and writes the intermediate metrics to Javascript Object Notation (JSON) files during the simulation.This saves considerable disk space and processing time over the handling of multiple CSV files. Python, and other languages, have library functions optimized to quickly load JSON files. 

## Attached objects and collected values

![Metrics Collector](../../images/500px-GLDMetricsClasses.png)
  
There are 8 types of objects that metrics_collector can be attached to:  
  
_**Triplex meter and Meter**_  
metrics_collector can be attached to triplex meters and meters to collect properties including real and reactive power, line-to-line voltage (Vll),line-to-neutral voltage (Vln) and voltage unbalance, bill, and voltage violations. At each predefined time interval, metrics_collector processes the recorded properties to obtain totally 28 aggregated values during this time period:  


  * Minimum real power
  * Maximum real power
  * Average real power
  * Minimum reactive power
  * Maximum reactive power
  * Average reactive power
  * Real energy
  * Reactive energy
  * Bill
  * Minimum line-to-line voltage
  * Maximum line-to-line voltage
  * Average line-to-line voltage
  * Minimum line-to-neutral voltage
  * Maximum line-to-neutral voltage
  * Average line-to-neutral voltage
  * Minimum voltage unbalance
  * Maximum voltage unbalance
  * Average voltage unbalance
  * Duration of voltage above ANSI C84.1 Range A upper limit
  * Number of times of voltage above ANSI C84.1 Range A upper limit
  * Duration of voltage below ANSI C84.1 Range A lower limit
  * Number of times of voltage below ANSI C84.1 Range A lower limit
  * Duration of voltage above ANSI C84.1 Range B upper limit
  * Number of times of voltage above ANSI C84.1 Range B upper limit
  * Duration of voltage below ANSI C84.1 Range B lower limit
  * Number of times of voltage below ANSI C84.1 Range B lower limit
  * Duration of voltage below 0.1 of nominal voltage
  * Number of times of voltage below 0.1 of nominal voltage  

_**House**_  
metrics_collector can be attached to houses to collect properties including HVAC loads, total loads, temperature and setpoints. At each predefined time interval, metrics_collector processes the recorded properties to obtain totally 11 aggregated values during this time period:  


  * Minimum HVAC load
  * Maximum HVAC load
  * Average HVAC load
  * Minimum total load
  * Maximum total load
  * Average total load
  * Minimum air temperature
  * Maximum air temperature
  * Average air temperature
  * Average cooling setpoint
  * Average heating setpoint   

_**Water heater**_  
metrics_collector can be attached to water heater to collect power consumption. At each predefined time interval, metrics_collector processes the recorded properties to obtain totally 3 aggregated values during this time period:  


  * Minimum total load
  * Maximum total load
  * Average total load   

_**Inverter**_  
metrics_collector can be attached to inverter to collect real and reactive power output. At each predefined time interval, metrics_collector processes the recorded properties to obtain totally 6 aggregated values during this time period:  


  * Minimum real power output
  * Maximum real power output
  * Average real power output
  * Minimum reactive power output
  * Maximum reactive power output
  * Average reactive power output   

_**Capacitor**_  
metrics_collector can be attached to capacitor to collect capacitor operations of each phase. At each predefined time interval, metrics_collector sums up the operations of the three phases to obtain 1 aggregated value during this time period:  


  * Total capacitor operations of the three phases   

_**Regulator**_  
metrics_collector can be attached to regulator to collect regulator operations of each phase. At each predefined time interval, metrics_collector sums up the operations of the three phases to obtain 1 aggregated value during this time period:  


  * Total regulator operations of the three phases   

_**Swing bus**_  
metrics_collector can be attached to either substation node or swing bus meter to collect real and reactive power and power losses of the whole feeder. At each predefined time interval, metrics_collector processes the recorded properties to obtain totally 18 aggregated values during this time period:  


  * Minimum real power of the whole feeder
  * Maximum real power of the whole feeder
  * Average real power of the whole feeder
  * Median real power of the whole feeder
  * Minimum reactive power of the whole feeder
  * Maximum reactive power of the whole feeder
  * Average reactive power of the whole feeder
  * Median reactive power of the whole feeder
  * Total real energy consumption of the whole feeder
  * Total reactive energy consumption of the whole feeder
  * Minimum real power losses of the whole feeder
  * Maximum real power losses of the whole feeder
  * Average real power losses of the whole feeder
  * Median real power losses of the whole feeder
  * Minimum reactive power losses of the whole feeder
  * Maximum reactive power losses of the whole feeder
  * Average reactive power losses of the whole feeder
  * Median reactive power losses of the whole feeder  

## Time interval

The time interval (in seconds) needs to be given by users in glm files. The time interval defines the period that metrics_collector aggregates the properties of the attached objects.   
An array is created for each recorded property of the attached object. The length of the array is based on the defined time interval in seconds. If the simulation time step is one second, the property values at each second are stored in arrays, and are aggregated at each time interval. If the minimum time step in simulation is larger than 1 second, interpolation method is used to put the values of each skipped second into the array, and the data in the array are aggregated in each time period. 

# 

Synopsis

Below example shows how metrics_collector is attached to an inverter object. User needs to define the time interval in seconds for the metrics_collector. metrics_collector can be also attached to meter, triplex meter, house, water heater, swing bus meter/substation node, capacitor and regulator using the same format.   

    
    
    module [tape];
    object inverter {
      name fourquadinv;
      inverter_type FOUR_QUADRANT;
      four_quadrant_control_mode LOAD_FOLLOWING;
      parent tripmeterval;
      sense_object triptransformer;
      rated_power 3000.0;		
      inverter_efficiency .95;
      charge_on_threshold 5.0 kW;
      charge_off_threshold 7.0 kW;
      discharge_off_threshold 7.5 kW;
      discharge_on_threshold 9.0 kW;
      max_discharge_rate 1.0 kW;
      max_charge_rate 0.80 kW;
      object metrics_collector {
        interval 300; // seconds
      }
    }
    

## See also

  * [Tape (module)]
    * [metrics_collector_writer]
