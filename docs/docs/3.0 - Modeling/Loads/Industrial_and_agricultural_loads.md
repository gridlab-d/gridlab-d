# Industrial and agricultural loads

Industrial and agricultural loads are implemented using a parametric load model, calculating the real and imaginary current, power, and resistance load components based on the time and existing weather data. 

# Parametric Loads

Parametric loads can be defined using the **pqload** object, which is derived from **powerflow::load**. A **pqload** object has varying contributions to constant impedance, current, and power depending on prevailing time and weather conditions, such as sunlight, temperature, humidity, wind speed, and rainfall. Each load component (Z, I, P) is nominally computed based on the prevailing weather and adjusted based on the time. 

The weather load is computed from a linear transformation as a function of weather as follows: 

$$\begin{bmatrix} Z_p \\\ Z_q \\\ I_m \\\ I_a \\\ P_p \\\ P_q \end{bmatrix} = \begin{bmatrix} Zp_T & Zp_H & Zp_S & Zp_W & Zp_R & Zp \\\ Zq_T & Zq_H & Zq_S & Zq_W & Zq_R & Zq \\\ Im_T & Im_H & Im_S & Im_W & Im_R & Im \\\ Ia_T & Ia_H & Ia_S & Ia_W & Ia_R & Ia \\\ Pp_T & Pp_H & Pp_S & Pp_W & Pp_R & Pp \\\ Pq_T & Pq_H & Pq_S & Pq_W & Pq_R & Pq \end{bmatrix} \ \begin{bmatrix} T_t \\\ H_t \\\ S_t \\\ W_t \\\ R_t \\\ 1 \end{bmatrix} 
$$

where 

  * $Z$, $I$, $P$ are the constant impedance, current and power components of the load, with $_p$ and $_q$ indicated the real and reactive parts and $m$ and $a$ are the magnitude and angle, respectively;
  * $T$ is the outdoor temperature at the time $t$ in °F;
  * $R$ is the relative humidity at the time $t$ in %;
  * $S$ is the solar gains at the time $t$ in Btu/h;
  * $W$ is the wind speed at the time $t$ in mph;
  * $R$ is the rainfall at the time $t$ in inch/h;
## Properties

The **pqload** object publishes the following variables, in addition to those published by **powerflow::load**. 

Table 1 - _pqload_ properties  Property | Type | Unit | Default | Description   
---|---|---|---|---  
schedule | char1024 | - | * * * * *:1.0; | The load schedule (default is always 1.0)   
weather | object | - | NULL | The climate object to use for this load. If NULL, the temperature coefficients will be ignored.   
Zp_T | double | ohm/degF | 0 | Resistance response to temperature   
Zp_H | double | ohm/% | 0 | Resistance response to humidity   
Zp_S | double | ohm.h/Btu | 0 | Resistance response to solar gains   
Zp_W | double | ohm/mph | 0 | Resistance response to wind   
Zp_R | double | ohm.h/in | 0 | Resistance response to rainfall   
Zp | double | ohm | ∞ | Invariant resistance   
Zq_T | double | F/degF | 0 | Capacitance response to temperature   
Zq_H | double | F/% | 0 | Capacitance response to humidity   
Zq_S | double | F.h/Btu | 0 | Capacitance response to solar gains   
Zq_W | double | F/mph | 0 | Capacitance response to wind   
Zq_R | double | F.h/in | 0 | Capacitance response to rainfall   
Zq | double | F | 0 | Invariant capacitance   
Im_T | double | A/degF | 0 | Current magnitude response to temperature   
Im_H | double | A/% | 0 | Current magnitude response to humidity   
Im_S | double | A.h/Btu | 0 | Current magnitude response to solar gains   
Im_W | double | A/mph | 0 | Current magnitude response to wind   
Im_R | double | A.h/in | 0 | Current magnitude response to rainfall   
Im | double | A | 0 | Invariant current magnitude   
Ia_T | double | deg/degF | 0 | Current angle response to temperature   
Ia_H | double | deg/% | 0 | Current angle response to humidity   
Ia_S | double | deg.h/Btu | 0 | Current angle response to solar gains   
Ia_W | double | deg/mph | 0 | Current angle response to wind   
Ia_R | double | deg.h/in | 0 | Current angle response to rainfall   
Ia | double | deg | 0 | Invariant current angle   
Pp_T | double | W/degF | 0 | Real power response to temperature   
Pp_H | double | W/% | 0 | Real power response to humidity   
Pp_S | double | W.h/Btu | 0 | Real power response to solar gains   
Pp_W | double | W/mph | 0 | Real power response to wind   
Pp_R | double | W.h/in | 0 | Real power response to rainfall   
Pp | double | W | 0 | Invariant real power   
Pq_T | double | VAr/degF | 0 | Reactive power response to temperature   
Pq_H | double | VAr/% | 0 | Reactive power response to humidity   
Pq_S | double | VAr.h/Btu | 0 | Reactive power response to solar gains   
Pq_W | double | VAr/mph | 0 | Reactive power response to wind   
Pq_R | double | VAr.h/in | 0 | Reactive power response to rainfall   
Pq | double | VAr | 0 | Invariant Reactive power   
input_temp | double | degF | \-- | Observed temperature. Read only.   
input_humid | double | % | \-- | Observed humidity. Read only.   
input_solar | double | Btu/h | \-- | Observed solar gains. Read only.   
input_wind | double | mph | \-- | Observed wind speed. Read only.   
input_rain | double | in/h | \-- | Observed rainfall. Read only.   
output_admit_p | double | Ohm | \-- | Observed resistance. Read only.   
output_admit_q | double | Ohm | \-- | Observed capacitance. Read only.   
output_current_m | double | A | \-- | Observed current magnitude. Read only.   
output_current_a | double | degrees | \-- | Observed current angle. Read only.   
output_power_p | double | W | \-- | Observed real power. Read only.   
output_power_q | double | VAr | \-- | Observed reactive power. Read only.   
output_admittence | complex | Ohm | \-- | Observed impedance. Read only.   
output_current | complex | A | \-- | Observed complex current. Read only.   
output_power | complex | W | \-- | Observed complex power. Read only.   
  
The schedule is defined based the POSIX standard for crontab. The general syntax is 


    moh hod dom moy dow[ lpu][; ...]
    

where 

  * `moh` is the minute of hour;
  * `hod` is the hour of day;
  * `dom` is the day of month;
  * `moy` is the month of year;
  * `dow` is the day of week;
  * `lpu` is the load per unit (optional, 1.0 if omitted);
  
The block may be repeated as many times as needed to complete the schedule. Any unscheduled time interval is assumed to be 0. 


