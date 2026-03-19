## Bill Dump

Similar to **voltdump**, **billdump** allows users to generate a single file where all customers' bills are written from **triplex_meter** to a single output file in a similar format 

meter_name,  | previous_monthly_bill,  | previous_monthly_energy   
---|---|---  
**triplex_meter_1**  | 154.30 ($)  | 1205 (kWh)   
**triplex_meter_2**  | 105.10 ($)  | 821 (kWh)   
  
### Default Bill Dump

The minimal specifications for billdump are 
    
    
    object billdump {
           filename bill_1.csv;
           }
    

where the previous month's energy and bill for all triplex meters within the system will be placed into `bill_1.csv`. Additional parameters can be added to describe when to run (runtime) and for only meters with a specific groupid: 
    
    
    object billdump {
        group "Residential_tm_solar";
        runtime '2012-04-01 01:00:00';
        filename residential_solar_bill.csv;
    }
    

### Bill Dump Parameters

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**filename**  | char32  | N/A  | Tells the object what file to print all information to. While a *.csv is not necessary, it is recommended as the formatted output is in *.csv format.   
**group**  | char32  | N/A  | Using the `groupid` feature, this allows only triplex meters with the matching `groupid` to be dumped into the output file. If this is not specified, every triplex meter in the system will be recorded.   
**runtime**  | timestamp  | N/A  | Tells the object at what time to output the bills of the system. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution.   
  
### Bill Dump State of Development

Bill Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 

