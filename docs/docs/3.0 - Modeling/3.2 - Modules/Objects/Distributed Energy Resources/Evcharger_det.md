# Evcharger det

Deterministic electric or hybrid vehicle charging 

## Synopsis
    
    
    class evcharger_det {
           (class residential_enduse;)
           double actual_charge_rate[W];
           double variation_mean[s];
           double variation_std_dev[s];
           double variation_trip_mean[mile];
           double variation_trip_std_dev[mile];
           double mileage_classification[mile];
           bool work_charging_available;
           char1024 data_file;
           int32 vehicle_index;
           enumeration {UNKNOWN=0, HOME=1, WORK=2, DRIVING_HOME=3, DRIVING_WORK=4} vehicle_location;
           double travel_distance[mile];
           double arrival_at_work;
           double duration_at_work[s];
           double arrival_at_home;
           double duration_at_home[s];
           double battery_capacity[kWh];
           double battery_SOC[%];
           double battery_size[kWh];
           double mileage_efficiency[mile/kWh];
           double maximum_charge_rate[W];
           double charging_efficiency[unit];
    }
    

## Properties

Property name | Type | Unit | Description   
---|---|---|---  
charge_rate | double | W | Current demanded charge rate of the vehicle   
variation_mean | double | s | Mean of normal-distributed variation of schedule variation   
variation_std_dev | double | s | Standard deviation of normal-distributed variation of schedule times   
variation_trip_mean | double | mile | Mean of normal-distributed variation of trip distance variation   
variation_trip_std_dev | double | mile | Standard deviation of normal-distributed variation of trip distance   
mileage_classification | double | mile | Mileage classification of electric vehicle - range to run in all electric   
work_charging_available | bool | none | Charging available when at work   
data_file | char1024 | none | Path to .CSV file with vehicle travel information   
vehicle_index | int32 | none | Index of vehicles in file to this particular vehicle's data   
vehicle_location | enumeration | none | Current vehicle location (UNKNOWN, HOME, WORK, DRIVING_HOME, DRIVING_WORK)   
travel_distance | double | mile | Distance vehicle travels from home to home - round trip distance for the day   
arrival_at_work | double | none | Time vehicle arrives at work - HHMM format   
duration_at_work | double | s | Duration the vehicle remains at work   
arrival_at_home | double | none | Time vehicle arrives at home - HHMM format   
duration_at_home | double | s | Duration the vehicle remains at home   
battery_capacity | double | kWh | Current capacity of the battery   
battery_SOC | double | % | State of charge of battery   
battery_size | double | kWh | Full capacity of battery   
mileage_efficiency | double | mile/kWh | Efficiency of drive train   
maximum_charge_rate | double | W | Maximum output rate of charger   
charging_efficiency | double | unit | Efficiency of charger (ratio) when charging   
  
## Default Evcharger_det

The minimum definition for an evcharger_det object is 
    
    
    object evcharger_det {
    }
    

This will implement a deterministic EV charger equivalent to the following: 
    
    
    object evcharger_det {
           variation_mean 0.0;
           variation_std_dev 0.0;
           variation_trip_mean 0.0;
           variation_trip_std_dev 0.0;
           mileage_classification 33.0;
           work_charging_available false;
           travel_distance 15.0;
           arrival_at_work 500;	//5:00 AM arrival
           duration_at_work 11.5 h;
           arrival_at_home 1700;	//5:00 PM arrival
           duration_at_home 11.5 h;
           mileage_efficiency 3.846;
           maximum_charge_rate 1700;
           charging_efficiency 0.90;
    	}
    

## Evcharger_det Schedule

The charging schedule for the deterministic EV is primarily determined by arrival_at_home, arrival_at_work, duration_at_home, and duration_at_work. Parameters set in variation_mean and variation_std_dev will apply a normal distribution of variation to any arrival and departure times for the vehicle. The vehicle will only influence the local power grid when plugged in at home. If work charging is available, the battery state of charge will be updated, but it is assumed the vehicle is charging elsewhere on the system and not influencing the electrical properties of the current GLM. 

## Data File Example

A data file can be specified for deterministic EV chargers to aid in creating diverse populations. The data file contains information that was available from the Department of Transportation's 2001 National Household Travel Survey. With the specified data file, only the vehicle_index parameter is needed to vary the population behavior. Note that only the ARRHOME, DUR.HOME, HHVEHMILES, ARRWORK, and DUR.WORK columns are used by the evcharger_det object. These fields correspond to the arrival_at_home, duration_at_home, travel_distance, arrival_at_work, and duration_at_work fields of the EV. 

Sample NHTS-data set in CSV format: 
    
    
    HH.VEH.ID,VEHTYPE,URBRUR,ARRHOME,DUR.HOME,DEPHOME,HHVEHMILES,ARRWORK,DUR.WORK,DEPWORK
    10009888.01,3,1,1425,900,525,32,600,480,1400
    10063787.02,4,2,1720,730,530,39,600,635,1635
    4M0000377.03,1,1,527,920,2047,37,2107,480,507
    

## Example
    
    
    module residential;
    module tape;
    
    clock {
    	timezone PST+8PDT;
    	timestamp '2001-07-10 00:00:00';
    	stoptime  '2001-07-18 00:00:00'; 
    };
    
    object house {
    	object evcharger_det {
    		object recorder {
    			file charge.csv;
    			property charge_rate;
    			interval 600;
    		};
    	};				
    }
    
