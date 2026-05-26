## Billdump

!!! warning
    This page was automatically generated and requires review.

Similar to **voltdump**, **billdump** allows users to generate a single file where all customers' bills are written from **triplex_meter** to a single output file in a similar format 

meter_name,  | previous_monthly_bill,  | previous_monthly_energy   
---|---|---  
**triplex_meter_1**  | 154.30 ($)  | 1205 (kWh)   
**triplex_meter_2**  | 105.10 ($)  | 821 (kWh)   

### Billdump Parameters

#### Properties

**billdump** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| group | char32 | N/A | I | Using the `groupid` feature, this allows only triplex meters with the matching `groupid` to be dumped into the output file. If this is not specified, every triplex meter in the system will be recorded. |
| runtime | timestamp | N/A | IO | Tells the object at what time to output the bills of the system. Can be in either seconds from epoch (Unix time) or with a timestamp ('2006-01-01 00:00:00'). If not specified, the default is immediately after the first time step solution. |
| filename | char256 | N/A | I | Tells the object what file to print all information to. While a *.csv is not necessary, it is recommended as the formatted output is in *.csv format. |
| runcount | int32 | N/A | — | ⚠️ the number of times the file has been written to |
| meter_type | enumeration | N/A | I | ⚠️ describes whether to collect from 3-phase or S-phase meters Valid values: `TRIPLEX_METER`, `METER`. |

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

### Bill Dump State of Development

Bill Dump is considered a well developed, but unvalidated model. Additional features may be included as needed. 
