## Pqload

!!! warning
    This page was automatically generated and requires review.

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

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| weather | object | N/A | ✓ |  | Link to the `climate` object used in for parameters of the load. |
| T_nominal | double | degF | ✓ |  | Nominal temperature **Note: Unused at this time** |
| Zp_T | double | ohm/degF | ✓ |  | Coefficient of how resistive impedance varies for temperature. |
| Zp_H | double | ohm/% | ✓ |  | Coefficient for how resistive impedance varies with humidity. |
| Zp_S | double | ohm*h/Btu | ✓ |  | Coefficient for how resistive impedance varies with solar gains. |
| Zp_W | double | ohm/mph | ✓ |  | Coefficient for how resistive impedance varies with wind speed. |
| Zp_R | double | ohm*h/in | ✓ |  | Coefficient for how resistive impedance varies with rain fall. |
| Zp | double | ohm | ✓ |  | Baseline, unvarying resistive impedance value. |
| Zq_T | double | F/degF | ✓ |  | Coefficient of how reactive impedance varies for temperature. |
| Zq_H | double | F/% | ✓ |  | Coefficient for how reactive impedance varies with humidity. |
| Zq_S | double | F*h/Btu | ✓ |  | Coefficient for how reactive impedance varies with solar gains. |
| Zq_W | double | F/mph | ✓ |  | Coefficient for how reactive impedance varies with wind speed. |
| Zq_R | double | F*h/in | ✓ |  | Coefficient for how reactive impedance varies with rain fall. |
| Zq | double | F | ✓ |  | Baseline, unvarying reactive impedance value. |
| Im_T | double | A/degF | ✓ |  | Coefficient of how current magnitude varies for temperature. |
| Im_H | double | A/% | ✓ |  | Coefficient for how current magnitude varies with humidity. |
| Im_S | double | A*h/Btu | ✓ |  | Coefficient for how current magnitude varies with solar gains. |
| Im_W | double | A/mph | ✓ |  | Coefficient for how current magnitude varies with wind speed. |
| Im_R | double | A*h/in | ✓ |  | Coefficient for how current magnitude varies with rain fall. |
| Im | double | A | ✓ |  | Baseline, unvarying current magnitude value. |
| Ia_T | double | deg/degF | ✓ |  | Coefficient of how current angle varies for temperature. |
| Ia_H | double | deg/% | ✓ |  | Coefficient for how current angle varies with humidity. |
| Ia_S | double | deg*h/Btu | ✓ |  | Coefficient for how current angle varies with solar gains. |
| Ia_W | double | deg/mph | ✓ |  | Coefficient for how current angle varies with wind speed. |
| Ia_R | double | deg*h/in | ✓ |  | Coefficient for how current angle varies with rain fall. |
| Ia | double | deg | ✓ |  | Baseline, unvarying current angle value. |
| Pp_T | double | W/degF | ✓ |  | Coefficient of how resistive power varies for temperature. |
| Pp_H | double | W/% | ✓ |  | Coefficient for how resistive power varies with humidity. |
| Pp_S | double | W*h/Btu | ✓ |  | Coefficient for how resistive power varies with solar gains. |
| Pp_W | double | W/mph | ✓ |  | Coefficient for how resistive power varies with wind speed. |
| Pp_R | double | W*h/in | ✓ |  | Coefficient for how resistive power varies with rain fall. |
| Pp | double | W | ✓ |  | Baseline, unvarying resistive power value. |
| Pq_T | double | VAr/degF | ✓ |  | Coefficient of how reactive power varies for temperature. |
| Pq_H | double | VAr/% | ✓ |  | Coefficient for how reactive power varies with humidity. |
| Pq_S | double | VAr*h/Btu | ✓ |  | Coefficient for how reactive power varies with solar gains. |
| Pq_W | double | VAr/mph | ✓ |  | Coefficient for how reactive power varies with wind speed. |
| Pq_R | double | VAr*h/in | ✓ |  | Coefficient for how reactive power varies with rain fall. |
| Pq | double | VAr | ✓ |  | Baseline, unvarying reactive power value. |
| input_temp | double | degF |  | ✓ | Observed temperature. _(read-only)_ |
| input_humid | double | % |  | ✓ | Observed humidity. _(read-only)_ |
| input_solar | double | Btu/h |  | ✓ | Observed solar gains. _(read-only)_ |
| input_wind | double | mph |  | ✓ | Observed wind speed. _(read-only)_ |
| input_rain | double | in/h |  | ✓ | Observed rainfall. _(read-only)_ |
| output_imped_p | double | Ohm |  | ✓ | Observed load resistive impedance value. _(read-only)_ |
| output_imped_q | double | Ohm |  | ✓ | Observed load reactive impedance value. _(read-only)_ |
| output_current_m | double | A |  | ✓ | Observed load current magnitude value. _(read-only)_ |
| output_current_a | double | deg |  | ✓ | Observed load current angular value. _(read-only)_ |
| output_power_p | double | W |  | ✓ | Observed load resistive power value. _(read-only)_ |
| output_power_q | double | VAr |  | ✓ | Observed load reactive power value. _(read-only)_ |
| output_impedance | complex | ohm |  | ✓ | Observed load combined impedance value. _(read-only)_ |
| output_current | complex | A |  | ✓ | Observed load combined current value. _(read-only)_ |
| output_power | complex | VA |  | ✓ | Observed load combined power value. _(read-only)_ |

### PQ Load State of Development

PQ Load is considered an experimental model and has not been validated at this time. 
