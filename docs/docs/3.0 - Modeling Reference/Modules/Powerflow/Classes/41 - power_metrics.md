## Power Metrics

The **power_metrics** object is used by the **reliability** module to calculate relevant **powerflow** metrics. The **power_metrics** object calculates the IEEE 1366-2003 metrics for evaluating the reliability indices of a power system. 

A minimalist **power_metrics** implementation is 
    
    
    object power_metrics {
    	name PwrMetrics;
    	}
    

with an equivalent of 
    
    
    object power_metrics {
    	name PwrMetrics;
    	base_time_value 60.0;
    	}
    

**power_metrics** objects are primarily output objects. 

### Power Metrics Parameters

**power_metrics** objects do not inherit properties from any module. Individual metrics are described in the **reliability** user's guide and in the IEEE 1366-2003 standard. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**SAIDI**  | double  | Customer interruption duration over total customers served  | The simulation-long computed value of the System Average Interruption Duration Index   
**SAIDI_int**  | double  | Customer interruption duration over total customers served  | The interval-long computed value of the System Average Interruption Duration Index. The interval is defined by the `base_time_Value` property.   
**SAIFI**  | double  | Customers interrupted over customers served  | The simulation-long computed value of the System Average Interruption Frequency Index   
**SAIFI_int**  | double  | Customers interrupted over customers served  | The interval-long computed value of the System Average Interruption Frequency Index. The interval is defined by the `base_time_Value` property.   
**ASAI**  | double  | Customer hours availability over customer hours demand  | The simulation-long computed value of the Average Service Availability Index.   
**ASAI_int**  | double  | Customer hours availability over customer hours demand  | The interval-long computed value of the Average Service Availability Index. The interval is defined by the `base_time_Value` property.   
**CAIDI**  | double  | Customer interruption duration over total customer interrupted  | The simulation-long computed value of the Customer Average Interruption Duration Index.   
**CAIDI_int**  | double  | Customer interruption duration over total customer interrupted  | The interval-long computed value of the Customer Average Interruption Duration Index. The interval is defined by the `base_time_Value` property.   
**MAIFI**  | double  | Customer momentary interruptions over total customers served  | The simulation-long computed value of the Momentary Average Interruption Frequency Index   
*8*  | double  | Customer momentary interruptions over total customers served  | The interval-long computed value of the Momentary Average Interruption Frequency Index. The interval is defined by the `base_time_Value` property.   
**base_time_value**  | double  | seconds  | Interval duration for IEEE 1366-2003 statistics to be computed. This information is the basis for any time calculations. For example, the interruption duration for a CAIDI calculation can be interruptions per hour, interruptions per minute, or any other time base. `base_time_value` dictates this base for the calculations. The value defaults to 1 minute.   
  
### Power Metrics State of Development

The **power_metrics** object is tested and validated with the **reliability** module. However, it has not been fully validated and is considered experimental at this time. 

