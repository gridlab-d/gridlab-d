# Evcharger
TODO: FT: Document in a manner consistent with other residential devices.  Old/deprecated model that has low trust


**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Evcharger
# Evcharger

**evcharger** \- Electric or hybrid vehicles 

## Synopsis
    
    
    class evcharger {
           (class residential_enduse;)
           enumeration {HIGH=2, MEDIUM=1, LOW=0} charger_type;
           enumeration {HYBRID=1, ELECTRIC=0} vehicle_type;       
           enumeration {WORK=1, HOME=0, UNKNOWN=4294967295} state;       
           double p_go_home[unit/h];
           double p_go_work[unit/h];
           double work_dist[mile];
           double capacity[kWh];
           double charge[unit];
           bool charge_at_work;
           double charge_throttle[unit];
           char1024 demand_profile;
    }
    

## Properties

Property name | Type | Unit | Description   
---|---|---|---  
charger_type | enumeration | none | Charge rate (HIGH, MEDIUM, or LOW)   
vehicle_tpe | enumeration | none | Specify if vehicle is hybrid or all electric.   
state | enumeration | none | Initial location of the vehicle.   
p_go_home | double | unit/h | Probability of vehicle returning home.   
p_go_work | double | unit/h | Probability of vehicle leaving to work.   
work_dist | double | mile | One way distance traveled between home and work.   
capacity | double | kWh | Battery capacity   
charge | double | unit | Current battery state of charge, fraction of capacity.   
charge_at_work | bool | none | Specify if work charging is available.   
charge_throttle | double | unit | Sets fraction of full charge rate.   
demand_profile | char1024 | none | Demand profile of the vehicle.   
  
## Default Evcharger

The minimum definition for an evcharger object is 
    
    
    object evcharger {
    }
    

## Evcharger Schedule

The evcharger object can be put on a schedule to control when it is at home, work, or on a trip. This can be done by using a schedule for the p_go_home and p_go_work properties (only control when it is at home or work) or by using the demand_profile property. 

## Example
    
    
    module residential;
    module tape;
    
    clock {
    	timezone PST+8PDT;
    	timestamp '2001-07-10 00:00:00';
    	stoptime  '2001-07-18 00:00:00'; 
    };
    
    object house {
    	object evcharger {
    		object recorder {
    			file charge.csv;
    			property charge;
    			interval 600;
    		};
    	};				
    }
    

## Bugs

## See Also

  * Residential module
    * User's Guide
    * Appliances
    * house class – Single-family home model.
    * residential_enduse class – Abstract residential end-use class.
    * occupantload – Residential occupants (sensible and latent heat).
    * ZIPload – Generic constant impedance/current/power end-use load.
  * Technical Documents 
    * Requirements
    * Specifications
    * Developer notes
    * Technical support document
    * Validation
