## Pqload

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

### Pqload Parameters

#### Properties

**pqload** objects are derived from **[load](17-load.md)** objects, so any parameters of the **[load](17-load.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| weather | object | N/A | I | Link to the `climate` object used in for parameters of the load. |
| T_nominal | double | degF | I | Nominal temperature **Note: Unused at this time** |
| Zp_T | double | ohm/degF | I | Coefficient of how resistive impedance varies for temperature. |
| Zp_H | double | ohm/% | I | Coefficient for how resistive impedance varies with humidity. |
| Zp_S | double | ohm*h/Btu | I | Coefficient for how resistive impedance varies with solar gains. |
| Zp_W | double | ohm/mph | I | Coefficient for how resistive impedance varies with wind speed. |
| Zp_R | double | ohm*h/in | I | Coefficient for how resistive impedance varies with rain fall. |
| Zp | double | ohm | I | Baseline, unvarying resistive impedance value. |
| Zq_T | double | F/degF | I | Coefficient of how reactive impedance varies for temperature. |
| Zq_H | double | F/% | I | Coefficient for how reactive impedance varies with humidity. |
| Zq_S | double | F*h/Btu | I | Coefficient for how reactive impedance varies with solar gains. |
| Zq_W | double | F/mph | I | Coefficient for how reactive impedance varies with wind speed. |
| Zq_R | double | F*h/in | I | Coefficient for how reactive impedance varies with rain fall. |
| Zq | double | F | I | Baseline, unvarying reactive impedance value. |
| Im_T | double | A/degF | I | Coefficient of how current magnitude varies for temperature. |
| Im_H | double | A/% | I | Coefficient for how current magnitude varies with humidity. |
| Im_S | double | A*h/Btu | I | Coefficient for how current magnitude varies with solar gains. |
| Im_W | double | A/mph | I | Coefficient for how current magnitude varies with wind speed. |
| Im_R | double | A*h/in | I | Coefficient for how current magnitude varies with rain fall. |
| Im | double | A | I | Baseline, unvarying current magnitude value. |
| Ia_T | double | deg/degF | I | Coefficient of how current angle varies for temperature. |
| Ia_H | double | deg/% | I | Coefficient for how current angle varies with humidity. |
| Ia_S | double | deg*h/Btu | I | Coefficient for how current angle varies with solar gains. |
| Ia_W | double | deg/mph | I | Coefficient for how current angle varies with wind speed. |
| Ia_R | double | deg*h/in | I | Coefficient for how current angle varies with rain fall. |
| Ia | double | deg | I | Baseline, unvarying current angle value. |
| Pp_T | double | W/degF | I | Coefficient of how resistive power varies for temperature. |
| Pp_H | double | W/% | I | Coefficient for how resistive power varies with humidity. |
| Pp_S | double | W*h/Btu | I | Coefficient for how resistive power varies with solar gains. |
| Pp_W | double | W/mph | I | Coefficient for how resistive power varies with wind speed. |
| Pp_R | double | W*h/in | I | Coefficient for how resistive power varies with rain fall. |
| Pp | double | W | I | Baseline, unvarying resistive power value. |
| Pq_T | double | VAr/degF | I | Coefficient of how reactive power varies for temperature. |
| Pq_H | double | VAr/% | I | Coefficient for how reactive power varies with humidity. |
| Pq_S | double | VAr*h/Btu | I | Coefficient for how reactive power varies with solar gains. |
| Pq_W | double | VAr/mph | I | Coefficient for how reactive power varies with wind speed. |
| Pq_R | double | VAr*h/in | I | Coefficient for how reactive power varies with rain fall. |
| Pq | double | VAr | I | Baseline, unvarying reactive power value. |
| input_temp | double | degF | O | Observed temperature. _(read-only)_ |
| input_humid | double | % | O | Observed humidity. _(read-only)_ |
| input_solar | double | Btu/h | O | Observed solar gains. _(read-only)_ |
| input_wind | double | mph | O | Observed wind speed. _(read-only)_ |
| input_rain | double | in/h | O | Observed rainfall. _(read-only)_ |
| output_imped_p | double | Ohm | O | Observed load resistive impedance value. _(read-only)_ |
| output_imped_q | double | Ohm | O | Observed load reactive impedance value. _(read-only)_ |
| output_current_m | double | A | O | Observed load current magnitude value. _(read-only)_ |
| output_current_a | double | deg | O | Observed load current angular value. _(read-only)_ |
| output_power_p | double | W | O | Observed load resistive power value. _(read-only)_ |
| output_power_q | double | VAr | O | Observed load reactive power value. _(read-only)_ |
| output_impedance | complex | ohm | O | Observed load combined impedance value. _(read-only)_ |
| output_current | complex | A | O | Observed load combined current value. _(read-only)_ |
| output_power | complex | VA | O | Observed load combined power value. _(read-only)_ |

### PQ Load State of Development

PQ Load is considered an experimental model and has not been validated at this time. 
