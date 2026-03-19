## Parametric Load

Parametric loads provide a **load**-like object that allows the load to vary based on other conditions in the system. This may be things such as weather conditions or even time-of-day scheduling. Further details on parametric loads can be found in the [Industrial and agricultural loads](../../Loads/3.0%20-%20Industrial_and_agricultural_loads.md) page. A typical parametric load would be called as 
    
    
    object pqload {
    	name pqload1;
    	phases ABC;
    	Zp 200 ohm;
    	Zq_T 250 F;
    	Im 300;
    	Ia 45;
    	Pp 100;
    	Pp_T 3;
    	Pq_T 1;
    	nominal_voltage 2400;
    	}
    

### Parametric Load Parameters

The **pqload** object is directly derived from the **load** object and thus derived from the **node** object as well. As such, parameters of those two objects are also available for use, but most **load** parameters will probably be overwritten. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**weather**  | object  | N/A  | Link to the `climate` object used in for parameters of the load.   
**T_nominal**  | double  | Fahrenheit  | Nominal temperature **Note: Unused at this time**  
Zp_T  | double  | Ohm/degree Fahrenheit  | Coefficient of how resistive impedance varies for temperature.   
**Zp_H**  | double  | Ohm  | Coefficient for how resistive impedance varies with humidity.   
**Zp_S**  | double  | Ohm-Hour/BTU  | Coefficient for how resistive impedance varies with solar gains.   
**Zp_W**  | double  | Ohm-Hour/Mile  | Coefficient for how resistive impedance varies with wind speed.   
**Zp_R**  | double  | Ohm-Hour/Inch  | Coefficient for how resistive impedance varies with rain fall.   
**Zp**  | double  | Ohm  | Baseline, unvarying resistive impedance value.   
**Zq_T**  | double  | Farad/degree Fahrenheit  | Coefficient of how reactive impedance varies for temperature.   
**Zq_H**  | double  | Farad  | Coefficient for how reactive impedance varies with humidity.   
**Zq_S**  | double  | Farad-Hour/BTU  | Coefficient for how reactive impedance varies with solar gains.   
**Zq_W**  | double  | Farad-Hour/Mile  | Coefficient for how reactive impedance varies with wind speed.   
**Zq_R**  | double  | Farad-Hour/Inch  | Coefficient for how reactive impedance varies with rain fall.   
**Zq**  | double  | Farads  | Baseline, unvarying reactive impedance value.   
**Im_T**  | double  | Ampere/degree Fahrenheit  | Coefficient of how current magnitude varies for temperature.   
**Im_H**  | double  | Ampere  | Coefficient for how current magnitude varies with humidity.   
**Im_S**  | double  | Ampere-Hour/BTU  | Coefficient for how current magnitude varies with solar gains.   
**Im_W**  | double  | Ampere-Hour/Mile  | Coefficient for how current magnitude varies with wind speed.   
**Im_R**  | double  | Ampere-Hour/Inch  | Coefficient for how current magnitude varies with rain fall.   
**Im**  | double  | Ampere  | Baseline, unvarying current magnitude value.   
**Ia_T**  | double  | degrees/degree Fahrenheit  | Coefficient of how current angle varies for temperature.   
**Ia_H**  | double  | degrees  | Coefficient for how current angle varies with humidity.   
**Ia_S**  | double  | degree-Hour/BTU  | Coefficient for how current angle varies with solar gains.   
**Ia_W**  | double  | degree-Hour/Mile  | Coefficient for how current angle varies with wind speed.   
**Ia_R**  | double  | degree-Hour/Inch  | Coefficient for how current angle varies with rain fall.   
**Ia**  | double  | degrees  | Baseline, unvarying current angle value.   
**Pp_T**  | double  | Watts/degree Fahrenheit  | Coefficient of how resistive power varies for temperature.   
**Pp_H**  | double  | Watts  | Coefficient for how resistive power varies with humidity.   
**Pp_S**  | double  | Watt-Hour/BTU  | Coefficient for how resistive power varies with solar gains.   
**Pp_W**  | double  | Watt-Hour/Mile  | Coefficient for how resistive power varies with wind speed.   
**Pp_R**  | double  | Watt-Hour/Inch  | Coefficient for how resistive power varies with rain fall.   
**Pp**  | double  | Watts  | Baseline, unvarying resistive power value.   
**Pq_T**  | double  | Volt-Amperes reactive/degree Fahrenheit  | Coefficient of how reactive power varies for temperature.   
**Pq_H**  | double  | Volt-Amperes reactive  | Coefficient for how reactive power varies with humidity.   
**Pq_S**  | double  | Volt-Amperes reactive-Hour/BTU  | Coefficient for how reactive power varies with solar gains.   
**Pq_W**  | double  | Volt-Amperes reactive-Hour/Mile  | Coefficient for how reactive power varies with wind speed.   
**Pq_R**  | double  | Volt-Amperes reactive-Hour/Inch  | Coefficient for how reactive power varies with rain fall.   
**Pq**  | double  | Volt-Amperes reactive  | Baseline, unvarying reactive power value.   
**input_temp**  | double  | degrees Fahrenheit  | Observed temperature. _(read-only)_  
**input_humid**  | double  | Percentage  | Observed humidity. _(read-only)_  
**input_solar**  | double  | BTU/hour  | Observed solar gains. _(read-only)_  
**input_wind**  | double  | Miles/hour  | Observed wind speed. _(read-only)_  
**input_rain**  | double  | inches/hour  | Observed rainfall. _(read-only)_  
**output_imped_p**  | double  | Ohms  | Observed load resistive impedance value. _(read-only)_  
**output_imped_q**  | double  | Ohms  | Observed load reactive impedance value. _(read-only)_  
**output_current_m**  | double  | Amperes  | Observed load current magnitude value. _(read-only)_  
**output_current_a**  | double  | degrees  | Observed load current angular value. _(read-only)_  
**output_power_p**  | double  | Watts  | Observed load resistive power value. _(read-only)_  
**output_power_q**  | double  | Volt-Amperes  | Observed load reactive power value. _(read-only)_  
**output_impedance**  | complex  | Ohms  | Observed load combined impedance value. _(read-only)_  
**output_current**  | complex  | Amperes  | Observed load combined current value. _(read-only)_  
**output_power**  | complex  | Volt-Amperes  | Observed load combined power value. _(read-only)_  
  
### PQ Load State of Development

PQ Load is considered an experimental model and has not been validated at this time. 

