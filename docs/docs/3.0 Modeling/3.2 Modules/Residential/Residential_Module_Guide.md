# Residential module user's guide

The thermal performance of a home in the House_e module is based on a simple thermal heat flow circuit, shown in Figure 1. Here, the complexity of much more detailed thermal models, as used in most building simulations, is reduced to an equivalent thermal parameter (ETP) model in which parallel or nearly parallel heat flow paths and series thermal mass elements are lumped into a few parameters and portrayed as a simple DC electric circuit. This reduces the number of details of the building design that must be specified by the user of House_e , greatly reduces memory requirements, and speeds execution (all critical when simulating populations of buildings, especially relevant where the thermal details of the population are somewhat uncertain in any event). 

[![Equivalent Thermal Parameters Circuit Modeled by House-e.](../../../../../images/300px-Residential_module_users_guide_figure_1.png)](/wiki/File:Residential_module_users_guide_figure_1.png)

Figure 1. Equivalent Thermal Parameters Circuit Modeled by House-e.

In the laws of physics, temperatures, conductances, thermal masses and heat flows are entirely equivalent to voltages, conductances, capacitors and current flows in the electric circuit analog. That is, the differential equations expressing conservation for energy are the same. In practice, when applied to represent heat flow in a building, this circuit is always over-damped. That is, it exhibits exponential decays and approaches toward steady-state (not oscillatory) conditions. 

The ETP circuit captures the essence of the response of the home under most circumstances of importance to analysis of a smart grid: heat gains and losses and the effects of thermal mass, as a function of weather (temperature and solar radiation), occupant behavior (thermostat settings and internal heat gains from appliances), and heating/cooling system efficiencies. How these are modeled using this framework is explained in this document. 

In its essence, the thermal envelope of the home has a conductance ($U_A$) through which heat flows from the room air temperature ($T_A$) to the outdoor air temperature ($T_O$). The $U_A$ is the sum of all parallel heat flow paths through the envelope of the building (walls, windows, doors, ceilings, floors, and infiltration air flows). The primary simplifying assumption here is that the masses of these elements of the building envelope are relatively insignificant compared to their conductances, so their masses can be lumped inside the home. 

The bulk of the mass in the home is summed to form the lumped mass $C_M$, which is coupled to the room air through a conductance that represents the sum of the products of the mass surface area and the heat transfer coefficient. The mass of the air in the interior volume of the house is represented by the much smaller mass $C_A$, which is directly couple to the room air. The primary effect of $C_A$ is to realistically dampen the effect of heat delivered to the air ($Q_A$) from the heating/cooling (HVAC) system turning on and off, which would otherwise result in an instantaneous change in room air temperature. 

Heat gains from solar radiation and from appliances are combined with that from the heating/cooling system to form the heat gains to the air, $Q_A$. The House_e model allows a specified fraction for each of the heat gains from heating/cooling, solar radiation, and internal appliances to allow them to bypass the air node and be delivered directly to the mass to form $Q_M$. This can be used to represent solid interior objects absorbing heat from solar radiation shining through windows, for example. This is a reasonable approximation for the wood frame construction predominant in U.S. homes. It becomes an increasingly poor assumption for buildings with massive masonry or brick exterior. Future versions of GridLab-D will have the capability to model these effects explicitly with a modified approach. 

Finally, a time-series solution of the ETP circuit must be solved, with a thermostat controlling the HVAC system to maintain heating and cooling setpoints specified by the occupants. This requires modeling the output of the HVAC system, and the electric input to it, as a function of the type, capacity, and efficiency of the equipment under varying conditions such as the outdoor temperature. 

The details of how these are modeled from user-specified inputs are described in the sections that follow. 

# Synopsis
    
    
    module residential;
    module residential {
      default_outdoor_temperature 74.0 [degF];
      default_humidity 75.0 [%];
      default_etp_iterations 100;
      implicit_enduses LIGHTS|PLUGS|OCCUPANCY|DISHWASHER|MICROWAVE|FREEZER|REFRIGERATOR|RANGE|EVCHARGER|WATERHEATER|CLOTHESWASHER|DRYER;
      house_low_temperature_warning 55 [degF];
      house_high_temperature_warning 95 [degF];
      thermostat_control_warning TRUE;
      system_dwell_time 1 [s];
      aux_cutin_temperature 10 [degF];
    }
    
# Classes

As of Four Corners (Version 2.2)

  * house – Single-family home model.
  * residential_enduse – Abstract residential end-use class.
  * waterheater – Typical residential water heating appliance.
  * ZIPload – Generic constant impedance/current/power end-use load.
As of Hassayampa (Version 3.0)
    These may be available in earlier versions but they have not been validated and are not supported.

  * lights – Typical residential lights.
  * occupantload – Residential occupants (sensible and latent heat).
  * plugload – Typical residential plug loads.
Unsupported
    These may be available in many versions but they have not been validated and are not supported.

  * clotheswasher – Typical residential clothes washing appliance.
  * dishwasher – Typical residential dish washing appliance.
  * dryer – Typical residential clothes drying appliance.
  * evcharger – Standard electric vehicle charger.
  * freezer – Typical residential freezing appliance.
  * microwave – Typical residential microwave appliance.
  * range – Typical residential cooking appliance.
  * refrigerator – Typical residential refrigeration appliance.

# Variables

  * default_line_voltage (complex3) Incoming line voltage to use when no power objects are defined (default is 240V+0j,120V+0j,120V+0j).
  * default_line_current (complex3) Line current across the outside energy meter (default is 0A+0j,0A+0j,0A+0j).
  * default_outdoor_temperature (double) Used when no climate/weather data is available (default is 74 degF).
  * default_humidity (double) Used when no climate/weather data is available (default is 75%).
  * default_solar (double9) Used when no climate/weather data is available (default is 0,0,0,0,0,0,0,0,0).
  * default_etp_iterations (int64) Limits the number of iterations the ETP solver will perform before stopping (default is 100).

# Bugs

Due to parsing limitations on arrays default_line_voltage, default_line_current, and default_solar cannot be set from a GLM file. 

# Envelope

## Solution to the ETP Heat Balance Equations

For the thermal circuit in Figure 1, a heat balance (conservation of energy) can be written for the air temperature node ($T_A$) as: 

$Q_A - U_A (T_A-T_O) - H_M (T_A-T_M) - C_A \frac{dT_A}{dt} = 0 \qquad\qquad(1)$

The heat balance for the mass temperature node ($T_M$) can be written as: 

$Q_M - H_M (T_M-T_A) - C_M \frac{dT_M}{dt} = 0\qquad\qquad(2)$

As shown in [ETP closed form solution], Equation (1) can be solved for $T_M$, differentiated with respect to time to provide $dT_M/dt$, and both of these substituted into (2) to form a second order linear differential equation in $T_A$ of the form 

$a \frac{d^2T_A}{dt^2} + b \frac{dT_A}{dt} + c\ T_A = d\qquad\qquad(3)$

where: 

  * $a = \frac{C_M C_A}{H_M}$
  * $b = \frac{C_M (U_A + H_M)}{H_M} + C_A$
  * $c = U_A \\!$
  * $d = Q_M + Q_A + U_A T_O \\!$

which has the solution with known, constant boundary conditions $T_O$, $Q_A$, and $Q_M$ and initial conditions at time $t=0$ of $T_{A_o}$ and $dT_{A_o}/dt$

$T_A = A_1 e^{r_1 t} + A_2 e^{r_2 t} + \frac{d}{c}\qquad\qquad(4)$

where: 

  * $r_1 = \frac{-b + \sqrt{b^2-4ac}}{2a}$
  * $r_2 = \frac{-b - \sqrt{b^2-4ac}}{2a}$
  * $A_1 = \frac{r_2 T_{A_o} - \frac{dT_{A_o}}{dt} - r_2 \frac{d}{c}}{( r_2 - r_1 )}$
  * $A_2 = T_{A_o} - \frac{d}{c} - \frac{r_2 T_{A_o} - \frac{dT_{A_o}}{dt} - r_2 \frac{d}{c}}{( r_2 - r_1 )}$

The initial condition $T_{A_o}$ is known as the final condition of $T_A$ from previous time step. However, at any time step at which the boundary conditions $T_O$, $Q_A$, or $Q_M$ have changed (i.e. the weather, internal gains, heating/cooling output) from the previous time interval, then the new air temperature trajectory at the beginning of the time step can be derived from Equation (1) as 

$$\frac{dT_{A_o}}{dt} = \frac{H_M}{C_A T_{M_o}} - \frac{U_A + H_M}{C_A T_{A_o}} + \frac{U_A}{C_A T_O} + \frac{Q_A}{C_A}\qquad\qquad(5)$$

Then, differentiating Equation (4) and substituting it and Equation (4) into Equation (1) yields a solution for $T_M$ of the form: 

$$T_M = A_1 A_3 e^{r_1 t} + A_2 A_4 e^{r_2 t} + g + \frac{d}{c}\qquad\qquad(6)$$

where: 

  * $g = \frac{Q_M}{H_M}$
  * $A_3 = \frac{r_1 C_A}{H_M} + \frac{U_A + H_M}{H_M}$
  * $A_3 = \frac{r_2 C_A}{H_M} + \frac{U_A + H_M}{H_M}$
## Initial Room and Mass Air Temperature

When initializing a House_E simulation, the temperature and weather history prior to the first time step is unknown. First assume the house is at the equilibrium air temperature (at steady state with the heating and cooling system off, i.e. in balance). Equilibrium is defined by $dT_A/dt_o = 0$. Then, by differentiating (4), at the beginning of the initial time step, it can be shown that 

$$T_{A_{eq}} = \frac{d}{c} = T_O + \frac{Q_M + Q_A}{U_A}\qquad\qquad(7)$$

and the corresponding mass temperature at equilibrium is 

$$T_{M_{eq}} = T_{A_{eq}} + \frac{Q_M}{H_M}\qquad\qquad(8)$$

If the heating system would be “on” (based on the thermostat heating set point; see the section Heating/Cooling Thermostat Operations) at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{M_o} = T_{set\ heat}\qquad\qquad(9)$ _# Heating system “on” at_ $T_A = T_{A_{eq}}$

If the cooling system would be “on” (based on the thermostat cooling set point) at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{M_o} = T_{set\ cool}\qquad\qquad(10)$ _# Cooling system “on” at_ $T_A = T_{A_{eq}}$

The time of day when either of these approximate initial conditions is most correct is when the conditions in the house have been stable for a long period of time. So, starting the simulation at midnight may be a good choice. An often better choice would be the earlier of sunrise or just prior to a morning thermostat change. 

If neither the heating or cooling system would be “on” at the condition $T_A = T_{A_{eq}}$, then the initial conditions are best approximated as 

$T_{A_o} = T_{A_{eq}}\qquad\qquad(11)$ _# Heating/cooling system “off” at_ $T_A = T_{A_{eq}}$
$T_{M_o} = T_{M_{eq}}\qquad\qquad(12)$

## Predicting the Time of the Next Heating/Cooling State Change

[**TODO**] 

# Primary Inputs

Parameter (symbol; selections) | Default Value   
---|---  
Floor area (A) | 2,500 ft²   
Floor aspect ratio (R) | 1.5 -   
No. Stories (n) | 1 -   
Ceiling height (h) | 8 ft   
Exterior ceiling, fraction of total (ECR) | 100%   
Exterior floor, fraction of total (EFR) | 100%   
Exterior wall, fraction of total (EWR) | 100%   
Window/exterior wall area ratio (WWR) | 7%   
Doors ($n_d$) 4 -   
Area of 1 door ($A_1d$) | 19.5 ft²   
Glazing layers (GL; 1, 2, 3,integer) | 2   
Glazing material (GM; glass, low-e glass) | low-e glass   
Window frame (WF; none,aluminium, thermal break,wood,insulated) | TB   
Glazing treatment (GT; clear, abs, refl, low-s, high-s) | clear   
Window, exterior transmission coefficient (WET)  
\--_For exterior shading effects None=1.0, insect screen=0.6 (ASHRAE)_ | 60%   
R-value, walls ($R_w$) | 19 °F.ft².hr/Btu   
R-value, ceilings ($R_c$) | 30 °F.ft².hr/Btu   
R-value, floors ($R_f$) | 22 °F.ft².hr/Btu   
R-value, doors ($R_d$) | 5 °F.ft².hr/Btu   
Infiltration volumetric air exchange rate (I) | 0.5 1/hr   
Interior/exterior wall surface ratio (IWR)  
\--_Based partitions for six rooms per floor_ | 1.5 -   
Interior surface heat transfer coefficient ($h_s$) | 1.46 Btu/hr.°F.ft²   
Total thermal mass, per unit floor area ($m_f$)  
\--_Rule of thumb (Pratt), residential wood-frame construction: 2 Btu/°F/ft² including furnishings_ | 2.0 Btu/°F.ft²   
Interior surface thermal mass, subtotal   
\--_Typical wood frame residential (Pratt), excludes furnishings_ | 1.5 Btu/°F.ft²   
Solar gain fraction to mass ($f_s$) | 50%   
Internal gain fraction to mass ($f_i$) | 50%   
HVAC delivered fraction to mass ($f_m$) | 0%   
Include solar quadrant (none, H(horizontal), N(north), S(south), E(east), W(west)) | N,S,E,W   
  
## Derived Defaults

### **Glazing**

Translate glazing material into glazing material type (gmt) for U-value with a look-up table 

    IF( OR(GM="glass", GM="low-e glass"), IF(GM="glass", gmt="G", gmt="L"), “Error: unrecognized glazing material" )

Based on Type = GL & gmt, and on WF, look-up the window U-value ($U_g$) in Table 1 (produces the default Window U-value = 0.47 Btu/hr-°F-ft2) 

  
**Table 1. Window U-Values as a Function of Window Characteristics**

**Window U-values, AHSRAE Handbook 2005, Table 29.2, Operable**  
---  
**Type** | **Layers** | **Glazing** | **U-value (Btu / hr.°F.ft²) by Frame Type**  
**None** | **Aluminum** | **Tb** | **Wood** | **In**  
1G  | 1  | Glass  | 1.04  | 1.27  | 1.08  | 0.90  | 0.81   
2G  | 2  | Glass  | 0.48  | 0.81  | 0.60  | 0.53  | 0.44   
2L  | 2  | Low-e glass  | 0.30  | 0.67  | 0.47  | 0.41  | 0.33   
3G  | 3  | Glass  | 0.31  | 0.67  | 0.46  | 0.40  | 0.34   
3L  | 3  | Low-e glass  | 0.27  | 0.64  | 0.43  | 0.37  | 0.31   
  
  
**Translate window frame into window frame type (wft) for Solar Heat Gain Coefficient (SHGC) look-up table**

    IF( OR(WF="AL", WF="TB"), wft="AL/TB", IF( OR(WF="WD", WF="IN"), wft="WD/IN", IF(WF="none", wft="none", "Error: unrecognized window frame") ) )

Based on Type = GL & GT, and on wft, lookup the nominal window SHGC in the Table 2: (produces the default Window Nominal SHGC = 0.67) 

**Table 2. Window Solar Heat Gain Coefficients as a Function of Window Characteristics** >

**Window SHGCs, AHSRAE Handbook 2005, Table 29.2, Operable**  
---  
**Type** | **Layers** | **Glazing  
Treatment** | **SHGC by Frame Type**  
**None** | **Aluminum/Tb** | **Wood/In**  
1clear  | 1  | Clear  | 0.86  | 0.75  | 0.64   
1abs  | 1  | Heat-absorbing  | 0.73  | 0.64  | 0.54   
1refl  | 1  | Reflective  | 0.31  | 0.28  | 0.24   
2clear  | 2  | Clear  | 0.76  | 0.67  | 0.57   
2abs  | 2  | Heat-absorbing  | 0.62  | 0.55  | 0.46   
2refl  | 2  | Reflective  | 0.29  | 0.27  | 0.22   
2low-s  | 2  | Low-solar  | 0.41  | 0.37  | 0.31   
2high-s  | 2  | High-solar  | 0.70  | 0.62  | 0.52   
3clear  | 3  | Clear  | 0.68  | 0.60  | 0.51   
3abs  | 3  | Heat-absorbing  | 0.34  | 0.31  | 0.26   
3refl  | 3  | Reflective  | 0.34  | 0.31  | 0.26   
3low-s  | 3  | Low-solar  | 0.27  | 0.25  | 0.21   
3high-s  | 3  | High-solar  | 0.62  | 0.55  | 0.46   
  
    (a) low-e only

The SHGC is the product of the nominal SHGC and the window exterior transmission coefficient 

$SHGC = SHGC_{nom}\ WET\ A_g$ _# See next section for derivation of $A_g$ (area of glazing)_

## **Thermal Integrity Table Inputs and Defaults**

For the convenience of the user in describing a population of buildings, we allow the thermal properties of the envelope construction to be entered as a table. This is useful because the insulation level of ceilings, walls, floors, and windows tend to be highly correlated, rather than independent, because of construction practices and/or building codes that are a function of the vintage of construction. That is, it is extremely unlikely to find a house with heavily insulated walls, but to ceiling insulation. 

So, the user is allowed to enter the following primary inputs as a table, of the form shown in Table 3, with defaults as shown. 

**Table 3. Thermal Integrity Table**

**Default Thermal Integrity by Component**  
---  
**Integrity  
Level** | **Description** | **Ceilings  
°F.ft².hr/Btu** | **Walls  
°F.ft².hr/Btu** | **Floors  
°F.ft².hr/Btu** | **Windows  
°F.ft².hr/Btu** | **Doors  
°F.ft².hr/Btu** | **Infiltration Air  
Exchange Rate (1/hr)**  
**Layers** | **Glazing** | **Treatment** | **Frame**  
0  | old, uninsulated  | 11  | 4  | 4  | 1  | Glass  | Clear  | Al  | 3  | 1.5   
1  | old, insulated  | 19  | 11  | 4  | 2  | Glass  | Clear  | Al  | 3  | 1.5   
2  | old, weatherized  | 19  | 11  | 11  | 2  | Glass  | Clear  | Al  | 3  | 1.0   
3  | old, retrofit upgraded  | 30  | 11  | 19  | 2  | Glass  | Clear  | Tb  | 3  | 1.0   
4  | moderately insulated  | 30  | 19  | 11  | 2  | Glass  | Clear  | Tb  | 3  | 1.0   
5  | very well insulated  | 30  | 19  | 22  | 2  | Low-e glass  | Clear  | Tb  | 5  | 0.5   
6  | extremely well insulated  | 48  | 22  | 30  | 3  | Low-e glass  | Heat-absorbing  | In  | 11  | 0.5   
  
To override the defaults, the user defines a thermal integrity table defining the following primary inputs (see the **Primary Inputs** section above), in the following order: 

**Paramater (symbol)** | **Selections** | **Units**  
---|---|---  
Integrity level | (integer value) | \-   
R-value, ceilings ($R_c$) | (value) | °F.ft².hr/Btu   
R-value, walls ($R_w$) | (value) | °F.ft².hr/Btu   
R-value, floors ($R_f$) | (value) | °F.ft².hr/Btu   
Glazing layers (GL) | (integer value: 1, 2, 3) | \-   
Glazing material (GM) | (glass, low-e glass) | \-   
Window frame (WF) | (none,aluminium,thermal break,wood,insulated) | \-   
R-value, doors ($R_d$) | (value) | °F.ft².hr/Btu   
Infiltration volumetric air exchange rate (I) | (value) | 1/hr   
  
    _Note: Any set of (column-wise) values in an integrity table will be overridden by entry of a primary input._

### **Heat Loss Coefficient ( $U_A$)**

Compute exterior surface areas: 

    Define the following, based on rectangular geometry. Let x = width, y = depth.
    Then the aspect ratio is $R = y / x$
    The floor area is $A = x y n$

    and the volume is $V = A h$

The gross exterior wall area ($A_{wt}$) can be derived by introducing the perimeter (p), as follows: 

$y\ x = A / n$
$y = R\ x$
$R\ x^2 = A / n$
$x^2 = \frac{A}{n R}$
$x = \sqrt{\frac{A}{nR}}$
$p\ = 2x + 2y = 2x + 2Rx = 2x (1 + R)$
$p\ = 2 (1 + R)\sqrt{\frac{A}{nR}} $
$A_{wt} = n\ h\ p $

Then   
  
---  
| $A_{wt} = 2 n h (1 + R) \sqrt{\frac{A}{nR}} $ | The gross exterior wall area ($A_{wt}$)   
| $A_g = WWR\ A_{wt}\ EWR$ | The gross window area ($A_g$)   
| $A_d = n_d\ A_{1d}$ | The total door area ($A_d$)   
| $A_w = (A_{wt}-(A_g + A_d))\ EWR$ | The net exterior wall area ($A_w$)   
| $A_c = \frac{A}{n} ECR$ | The net exterior ceiling area ($A_c$)   
| $A_f = \frac{A}{n} EFR$ | The net exterior floor area ($A_f$)   
  
The total heat loss coefficient (conductance), $U_A$, for the house (the last term is for air infiltration); the defaults produce $U_A$ = 522.1 Btu/°F.hr 

$$U_A = A_g U_g + \frac{A_d}{R_d} + \frac{A_w}{R_w} + \frac{A_c}{R_c} + \frac{A_f}{R_f} + 0.018 A h I $$

    

    Note: 0.018 is the volumetric heat capacity of air at standard conditions (Btu/°F.ft³std-air-pressure)

### **Interior Mass Surface Conductance ( $H_m$)**

Surface area is estimates as total exterior walls (less doors and windows) + interior walls + ceilings 

$$H_m = h_s\ (\frac{A_w}{EWR}) + A_{wt} IWR + \frac{A_c n}{ECR}$$

### **Total “Air” Mass ( $C_a$)**

Based on tuning to typical home heating system cycling times, the “air mass” seems to be well approximated as 3 times the volumetric capacitance of the interior air volume. 

$C_a = 3\ (0.018 A\ h)$ _# Short-cycle thermal mass_

### **Total Thermal Mass ( $C_m$)**

$C_m = A\ m_f - 2\ (0.018 A\ h)$ # Thermal mass (daily cycle), less that added to the “air” mass

# HVAC Systems

## Primary Inputs

**Parameter (symbol; selections)** | **Default Value**  
---|---  
Heat system type (gas, heat pump, resistance, none) | heat pump | \-   
Cool system type (electric, none) | none | \-   
Cooling COP, standard conditionse | 3.50 | \-   
Heating COP, standard conditionsa | 3.50 | \-   
Latent cooling, fraction, of sensible cooling | 35% | \-   
Thermostat set point, heat (Tset_heat; value, or a schedule)b | 70 | °F   
Thermostat set point, cool (Tset_cool; value, or a schedule)c | 75 | °F   
Thermostat deadband (dTdeadband)d | 2.0 | °F   
Thermostat cycle time, minimum (tmin) | 2.0 | min   
Auxiliary heat (electric, none)e | electric | \-   
Auxiliary heat deadband (dTaux; value, none)e,f,g | 2.0 | °F   
Auxiliary heat outdoor lockout temperature (Taux; value, none)e,h | none | °F   
Auxiliary heat time delay (taux)e,h | none | min   
Fan type (1-speed, 2-speed, none)h,i | 1-speed | \-   
Fan power, low-speed, fraction of hi-speedj | % | \-   
Heating COP curve (default, flat, linear, curved) | default | \-   
Cooling COP curve (default, flat, linear, curved) | default | \-   
Heating capacity curve (default, flat, linear, curved) | default | \-   
Cooling capacity curve (default, flat, linear, curved) | default | \-   
Use latent heat (true, false) | true | \-   
Include fan heat gain (true, false) | true | \-   
_a_ For resistance heat system type, the default COP is 1.0 and not settable by the user.  
---  
_b_ Applicable to all heat system types, other than _none_.  
_c_ Applicable to _electric_ cool system type only.  
_d_ Temperature range of a thermostat from "on" to "off" (centered on thermostat setpoint). To prevent simultaneous heating and cooling, throw a warning if Tset_cool \- Tset_heat < dTdeadband  
_e_ Applicable to _heat pump_ heat system type only.  
_f_ Applicable to _heat pump_ heat system type with _electric_ auxiliary heat only.  
_g_ To control auxiliary heat, at least one of $dT_{aux}$, $T_{aux on}$ and $t_{aux}$ must be given a value; otherwise throw a warning.  
_h_ Cool system type _electric_ and heat system types _heat pump_ and _gas_ require a 1-speed or 2-speed fan, so _none_ is not a valid selection in cases.  
_i_ _1-speed_ or _2-speed_ indicates a forced air system. _1-speed_ fan is off when heating and cooling are off. _2-speed_ fan operates continually at low power when heating and cooling are off.  
_j_ Applicable to _2-speed_ fan types only.  
  
## How to Specify Common HVAC Systems

The types of equipment that form a residential heating/ventilating/air conditioning (HVAC)system is defined in House_e by the input parameters Heat_system_type, the Cool_system_type, and the Fan_type. 

  


### **Air Conditioning (Cooling)**

House_e only supports electrically-powered forced-air vapor-compression cooling, which can represent either a central air conditioner or a window/wall unit that cycles on and off to maintain the air temperature below the cooling thermostat setpoint. This is defined as an _electric_ Cool_system_type. If no air conditioning is provided, input _none_ for this parameter. 

An air conditioner uses an electrically-powered refrigerant pump to move heat from the cooled space and reject it outdoors. Hence, it can remove more heat from the house than the electricity input to the pump, and the COP is thus typically much greater than 1.0 (conservation of energy requires that the heat rejected outdoors is equal to the heat removed from the house plus the energy input to the pump). 

The laws of thermodynamics governing the vapor-compression cycle show that the COP decreases as the outdoor temperature increases, because of the increased difficulty of rejecting the heat outdoors. House_e models this phenomena. Similarly, the COP decreases as the supply air temperature decreases, and as the temperature and humidity of the air from the house increase, but these variations are assumed to be relatively small and House_e does not model them. 

Default values for the other input parameters are characteristic of a central air conditioner or heat pump. Window/wall air conditioners generally are less efficient, so the Cooling COP under standard conditions is likely to be lower than the default value. A house may utilize multiple window/wall air conditioners in different locations, each with its own thermostat. House_e simulates them together as a single equivalent unit controlled by a single thermostat. It is not uncommon for these units to be too small to keep a house at the cooling setpoint during very hot weather. If this is the case, the user can specify a negative oversizing factor (see the next section, **Design Loads and HVAC System Sizing**) 

  


### **Heating**

House_e supports a variety of heating system types: 

    

  * The absence of a heating system is indicated by a Heat_system_type parameter of value _none_.
    

  * In a _resistance_ Heat_system_type, heat is provided by an electric resistance heating coil (either in a forced-air furnace, or electric baseboard radiators) that cycles on and off to try to maintain the air temperature above the heating thermostat setpoint. The COP of a resistance heating system is a constant 1.0, being unaffected by the outdoor air temperature. The capacity is likewise unaffected.
    

  * In a _gas_ Heat_system_type, heat is provided by a gas-powered furnace or a boiler that cycles on and off to try to maintain the air temperature above the heating thermostat setpoint, but places no load on the electricity distribution system (except for its fan; see the next section). Its capacity is a constant unaffected by the outdoor air temperature. This can be used to model heating systems supplied by natural gas, propane, wood/biomass, and other non-electric sources.
    

  * In a _heat pump_ heat_system_type, heat is provided by a heat pump that is a reversible vapor compression cycle, i.e. and air conditioner running in "reverse" that pumps heat from outdoors and rejects it indoors. It cycles on and off to try to maintain the air temperature above the heating thermostat setpoint, but places no load on the electricity distribution system( excpet for its fan; see the next section). Its capacity is a constant unaffected by the outdoor air temperature.
Like an air conditioner, a heat pump's heating COP is not a constant, but instead decreases as the outdoor temperature decreases, because of the increased difficulty of pumping heat from outdoors. House_e models this phenomena. Similarly, the COP and capacity decrease as the supply air temperature increases and as the temperature of the air from the house increases, but these variations are assumed to be relatively small and House_e does not model them. 

### **Auxiliary heating(For Heat Pumps Only)**

An auxiliary heating system can be specified for a heat pump Heat_system_type by providing a value of _electric_ for the input parameter Auxiliary_heat. A heat pump is generally sized to meet the peak cooling requirement, and its heating capacity under standard conditions is generally equal to its cooling capacity. Given its reduced output at very cold outdoor temperatures, it may not have enough capacity to maintain the house at the desired heating setpoint. An auxiliary heat system serves this function. A value of _none_ for the Auxilairy_heat parameter indicates no such auxiliary heat is provided. 

If auxiliary heat is specified (the default), then a control strategy for it must also be specified. Three types of control can be modeled by House_e. Atleast one must be specified; two or all three can be specified to work in combination. Each auxiliary heat control strategy can call for auxiliary heat to turn on and the heat pump heating cycle to turn off. The auxiliary heat control strategies supported by House_e are: 

    

  * If a _value_ is provided for the Auxiliary_heat_deadband input parameter, then the auxiliary heat comes "on" when the indoor air temperature drops by more than the _value_ below the heating setpoint and remains "on" until the thermostat is satisfied (i.e., when the heating system would normally go "off"). This is the default control strategy (_value_ =2°F) and the most common one in usage in the U.S in the decades 1980-2010. The problem with it is that if the thermostat is set back at night , it will trigger auxiliary heat when the thermostat is set up in the morning, even under relatively warm conditions.
    

  * If a _value_ is provided for the Auxiliary_heat_lockout_temperature input parameter, then the auxiliary heat comes "on" whenever the thermostat calls for heating and the outdoor air temperature is below _value_. It remains "on" until the thermostat is satisfied (i.e.,when the heating system would normally go "off"). The default _value_ = _none_. This is an older control strategy particularly common to early heat pump systems in the U.S.
(NOTE: If used, this parameter should be diversified to represent a realistic range of values in the population by using a distribution as the input. Otherwise an entire population of homes will shift to auxilairy heat at the same time in a GridLAB-D simulation) 

    

  * If a _value_ is provided for the Auxiliary_time_delay input parameter, then the auxiliary heat comes "on" whenever the heating system has been "on" for more than _value_ minutes but the heating thermostat remains unsatisfied. The auxilairy heat remains "on" until the thermostat is satisfied (i.e.,when the heating system would normally go "off"). The default _value_ is _none_. This is the most advanced control strategy and is designed to minimize auxiliary heating by giving the heat pump a chance to satisfy the heating requirement before turning to auxiliary heat.
  


### **Circulation Fan**

Air conditioners and heat pumps require a fan to be specified (Fan_type parameter equal to /-_speed_ or _2-speed_). For resistance heating systems without an air conditioning, a fan is optional. If fan_type is _none_ , then the heating system is implicitly a baseboard/radiator system. otherwise 9and by default) a fan is assumed to be present, and will be sized to meet the larger of the heating or cooling air flow rate required to meet the heating and cooling design loads, respectively. 

A _1-speed_ Fan_type is "on" when the cooling, heating, or auxiliary heating system is "on". This is the default, and the most common case in the u.S. 

The heat from the power to the fan is added to the output of the HVAC system, and the power is add to the electrical load placed by the HVAC system on the electric distribution system. 

A _2-speed_ Fan_type is "on" continually, at full speed when the cooling, heating, or auxiliary heating system is "on", and at low speed to circulate air when the cooling,heating, and auxiliary heating system are each "off" or not present. At low speed, the power of the fan is reduced by the factor of the input parameter Fan_power_low_speed_fraction_of_high_speed. Note that fan power is generally proportional to the square of the flow rate; if the air flow is 50% at low-speed, then the power at low speed is (50%)2 , i.e. 25%. 

  


### **Example HVAC System Specification**

[TO DO] 

## **Design Loads and HVAC System Sizing**

### **User Inputs (and Defaults)**

**Parameter** | **Default Value** | **Units**  
---|---|---  
Design indoor temperature, heating | 70 | °F   
Design indoor temperature, cooling | 75 | °F   
Design outdoor temperature, heating | (a) | °F   
Design outdoor temperature, cooling | (b) | °F   
Design solar radiation | 195c | Btu/hr.ft2  
Design internal gains | (d) | Btu/hr   
Oversizing factor | 0% | \-   
Cooling supply air temperature | 50 | °F   
Heating supply air temperature | 150 | °F   
Duct pressure drop | $\frac{1}{2}$ | in. water   
_a_ If a weather file is provided, the default is the minimum hourly outdoor temperature in the weather file. If no file provided, then the default is 0°F.  
---  
_b_ If a weather file is provided, the default is the maximum hourly outdoor temperature in the weather file. If no file provided, then the default is 95°F.  
_c_ Typical clear day incident radiation for 35° latitiude and equal window areas in each of the eight cardinal directions (N, NE/NW, E/W, SE/SW, and S respectively) is 1/8 * (60+2*201+2*261+2*213+153) = 195 Btu/hr.ft2. (ASHRAE Handbook of Fundamentals, 2005).  
_d_ A function of floor area (see next section **Design Internal Gains**): 167.09*(floor_area)0.442  
  
### **Design Internal Gains**

The design internal gains as a function of floor area are approximated as a regression against mean annual ELCAP consumption data for the “Other” end use by floor area categories, as follows (from spreadsheets ELCAP Load Shapes_Q3.xls and Internal Gains Default.xls): 

**Table 4. Regression of ELCAP “Other” Annual End Use Load vs. Floor Area**

**End Use** | **Size of Home (ft 2)** | **Regression: ln(Other) = ln(a) + b ln(x)**  
---|---|---  
**850** | **1350** | **2100** | **2475** | **Parameter** | **Value** | **Std. Error**  
Other  | 6730  | 7298  | 9066  | 11079  | **ln(a)** | 5.7834  | 0.8017   
**ln( End Use )** | **ln( Size of Home (ft 2) )** | **b** | 0.4420  | 0.1088   
**6.745** | **7.208** | **7.650** | **7.814** | **r2** | 0.8918   
ln( Other )  | 8.814  | 8.895  | 9.112  | 9.313  | **a** | 324.9  | 45.0   
**Predicted End Use** | **Size of Home (ft 2)** | EU = a xb  
**850** | **1350** | **2100** | **2475** | ln(EU) = ln(a) + ln(xb  
Other  | 6403  | 7856  | 9550  | 10269  | ln(EU) = ln(a) + b ln(x)   
  
For the “Other” end use (excludes heating and water heating) in the ELCAP metered end use data project, the results of a linear regression of the average annual energy consumption as a function of floor area of the form Other = a xb can be converted by an axis transformation into a linear regression of the form 

$$log_e (kWh/yr) = log_e(a) + b\ log_e(floor area, ft^2)$$

with the resulting coefficients a and b shown in the table above. 

Also from ELCAP, the ratio of 1) the maximum hourly load of the summer average load shape to 2) the mean hourly load for the year is 1.32 (hour 18). Combined with the regression results, and converting the units from kWh/yr to Btu/hr, the design internal gains as a function of floor area is 

    Design_internal_gains = 324.9 * (floor_area)0.442 * 1.32 * 3413 / 8760

### **Sizing Calculations**

This section describes how sensibly sized HVAC units are created. 

    Design heating load (Btu/hr) = UA * (Design indoor temperature heating – Design outdoor temperature heating)

    Design sensible cooling load (Btu/hr) = UA * (Design outdoor temperature cooling - Design indoor temperature cooling) + Design internal gains * 3.413 (Btu/hr-kW) +

Design solar_radiation * Ag * SHGC * WET 

    Design total cooling load (Btu/hr) = Design_sensible_cooling load * (1 + Latent_cooling_fraction)

For cool system types other than _none_ , the design cooling capacity is nearest 6,000 Btu/hr increment above design cooling load (otherwise the Design_cooling_capacity = 0) 

    Design_cooling_capacity (Btu/hr) = Round( (Design_total_cooling_load * (1 + Oversizing_ factor) + 3000) / 6000 ) * 6000

Other than for heat pumps, the design heating capacity is nearest 10,000 Btu/hr increment above design heating load (otherwise the Design_heating_capacity (Btu/hr) = 0) 

    Design_heating_capacity (Btu/hr) = Round( (Design_heating_load * (1 + Oversizing_factor) + 5000) / 10000 ) * 10000 # Heat system types other than heat pump

For heat pumps (only), a rule of thumb is that the heating capacity is equal to the cooling capacity, and the auxiliary heating capacity is equal to the 

    Design_heating_capacity (Btu/hr) = Design_cooling_capacity # _heat pump_ Heat system type only

For _heat pump_ Heat system types with _electric_ auxiliary heat only: 

    Auxiliary_capacity (Btu/hr) = Round( (Design_heating_load * (1 + Oversizing_factor) + 5000) / 10000 ) * 10000 # heat pump Heat system type only

For all other cases, the Auxiliary capacity = 0. 

  
For Fan types other than _none_ and Heat system types other than _none_ , determine the volumetric flow rate of air at standard conditions and 150 °F to deliver the greater of the Design_heating_capacity and the Auxiliary_capacity: 

    Design_heating_airflow = Max(Design_heating_capacity and the Auxiliary_capacity)/( 0.018 * (Design_heating_supply_temperature - Design_indoor_temperature_heating))/ 60

For Cool system types other than _none_ , size a fan to deliver a sufficient volume of air at 50 °F to deliver the Design_cooling _cfm: 

    Design_cooling_airflow = Design_cooling_capacity /(1 + Latent_fraction)/(0.018 * (Design_indoor_temperature_cooling - Design_cooling_supply_temperature)/60

The power input to the fan is based on the greater of the Design_cooling_airflow and the Design_heating_airflow, assuming a ½ in. of water pressure drop, a 42% efficient fan, and an 88% efficient motor sized to the nearest 1/8 HP (for Fan type _none_ , Fan power = 0) 

    Fan_power = Round(0.117 * Duct_pressure_drop * Max(Design_cooling_airflow, Design_heating_airflow)/$\frac {0.42}{745.7} + \frac {1}{16})/ \frac{1}{8}))*\frac{1}{8} * \frac{745.7}{0.88}$$

  
## **Heating/Cooling Thermostat Operations**

For convenience, define a set of HVAC functionality indicators, F, which define the capabilities of the HVAC system and whether they are enabled at a given time: 

  * Fcool = Boolean(Cool_system_type = _electric_ & cooling system is enabled )

  * Fheat = Boolean(Heat_system_type $\neq$ none & heating system is enabled)

  * Faux = Boolean(Heat_system_type = _heat pump_ & Auxiliary_heat = _electric_ & heating system is enabled )

  * Ffan = Boolean(Fan type$\neq$none)

Also define a set of state variables: 

  * Coolon = Boolean(Cooling system is “on” )
  * Cooloff = Boolean(Cooling system is “off” )=$1-\text{Cool}_\text{on}$
  * Heaton = Boolean(Cooling system is “on” )
  * Heatoff = Boolean( Heating system is “off” )=$1-\text{Heat}_\text{on}$

For heat pumps with electric auxiliary heat, define additional functionality indicators and state variables: 

  * Faux_deadband = Boolean(Faux & $dT_\text{aux}\neq$none)
  * Faux_lockout=Boolean(Faux & Taux_on$\neq$none)
  * Faux_delay=Boolean(Faux & taux_on $\neq$none )
  * Auxon = Boolean( Auxiliary heat is “on” )
  * Auxoff = Boolean(Auxiliary heat is “off” ) = 1 - Auxon

Further, define the time (in minutes) from the last state change as thvac. 

To initialize the time-series, heating, cooling (and auxiliary heat) are “off”: Coolon = Heaton = Auxon = 0 Cooloff = Heatoff = Auxoff = 1 

### **Cooling Thermostat**

  * Coolon = Boolean(Cooloff $t_\text{hvac}$ > $t_\text{min}$ & $F_\text{cool}$ & $T_\text{air}$ > (Tset_cool + ½ dTdeadband))

  * Cooloff = Boolean(Coolon&thvac > tmin& Fcool & Tair $\leq$ (Tset_cool \- ½ dTdeadband))

### **Heating Thermostat**

  * Heaton = Boolean( Heatoff & thvac > tmin & Fheat & Tair $\leq$ (Tset_heat - ½ dTdeadband))

  * Heatoff = Boolean( Heaton & thvac > tmin & Fheat & Tair > (Tset_heat \+ ½ dTdeadband) )

### **Auxiliary Heating Control (Heat Pumps Only)**

To allow a state change from heating to auxiliary to occur in a single time step, the heating and cooling state change evaluations should be followed by: 

  * Auxon = Boolean( Heaton & Auxoff & OR(NOT(Faux_deadband) * ( Tair $\leq$( Tset_heat \- ½ dTaux) ) & OR (NOT(Faux_lockout * (Tair $\leq$ Taux) & OR(NOT (Faux_delay,(thvac > taux) )

  * Auxoff = Boolean( Auxon & T Faux_deadband * ( Tair > (Tset_heat + dTdeadband) ) & thvac > tmin )

If Auxon then Heatoff = 1 and Heaton = 0 

### **Band control**

Under **band control** regime (e.g., `object house {thermostat_control BAND;}`), the setpoint and deadband settings are ignore, and instead the HVAC control uses the band control variables: 

* TauxOn - 
    The indoor temperature at which auxiliary heating is turned on.

* TheatOn -
    The indoor temperature at which normal heating is turned on.

* TheatOff -
    The indoor temperature at which heating is turned off.

* TcoolOff -
    The indoor temperature at which cooling is turned off.

* TcoolOn -
    The indoor temperature at which cooling is turned on.

The control regime is used for external controllers that wish to directly control the actual temperatures at which the HVAC system changes state. 

### **No control**

Under **no control** regime, i.e., `object house { thermostat_control NONE;}`, all control variables are ignored and the HVAC system mode is not changed by any internal logic. The variable `system_mode` determines the state of the HVAC system and must be controlled directly from the external controller. 

### **Outdoor Temperature Adjustments to Capacity and COP**

The DOE-2 building stimulation program provides curves that adjust nameplate COPs and capacities for heat pumps and air conditioners as a function of the wet-bulb temperature (Twb) of the return air and/or the outdoor air temperature (Tout). The curves relevant to the House_e model (from DOE-2 Reference Manual, Part 1, Version 1.2, pg IV.194-199) are shown in Table 5, below. Variables involving CAP refer to capacity and variables involving EIR refer to DOE-2’s energy input ratio, which is the inverse of COP. 

**Table 5. DOE-2 System-Equipment Default Curves**

DOE-2 System-Equipment Default Curves  
Extracted from Table IV.11, DOE-2 Reference Manual, Part 1, Version 1.2 (pg IV.72-73)


Keyword | Curve | Variables | Curve Type  | Applicable  SYSTEM-TYPE(s) | Default Curve Coefficients
---|--- | ---|--- |---|---   
COOL-CAP-FT  | SDL-C1  | Twb,Tout  | bi-linear  | RESYS  | 0.59815404  | 0.01329987  | 0.0  | -0.00514995  | 0.0  | 0.0   
COOL-EIR-FT  | SDL-C11  | Twb,Tout  | bi-linear  | RESYS  | 0.49957503  | -0.00765992  | 0.0  | 0.01066989  | 0.0  | 0.0   
HEAT-CAP-FT  | SDL-C51  | Tout  | quadratic  | RESYS  | 0.34148808  | 0.00894102  | 0.00010787  | 0.0  | 0.0  | 0.0   
HEAT-EIR-FT  | SDL-C56  | Tout  | cubic  | RESYS  | 2.03914613  | -0.03906753  | 0.00045617  | -0.00000203  | 0.0  | 0.0   
  
  
House_e does not explicitly model moisture in the home, so the Twb is assumed to be at the standard test condition 67°F. Eliminating Twb as a variable and inverting the EIR equations to produce equivalent COP equations gives the correction factor equations of the forms: 

  F_COP_T = $\frac{1}{EIR-FT} = \frac{1}{(K_{0} + K_{1} * T_{out} + K_{2} * T_{out}^{2} + K_{3} * T_{out}^{3})}$

  F_Capacity_Tout = CAP-FT = $K_{0} + K_{1} * T_{out} + K_{2} * T_{out}^{2} + K_{3} * T_{out}^{3}$

The resulting coefficients used by GridLAB-D are shown in the Table 6, below. 

**Table 6. GridLAB-D Equipment COP Factors**

**HVAC Equipment COP Factors**  
---  
**COP Factor** | **K 0** | **K 1** | **K 2** | **K 3** | **Limit**  
---|--- | ---|--- |---|---   
F_Cool_COP_Tout | -0.01363961  | 0.01066989  | 0.0  | 0.0  | 40   
F_Heat_COP_Tout | 2.03914613  | -0.03906753  | 0.00045617  | -0.00000203  | 80   
F_Cooling_Capacity_Tout | 1.48924533  | -0.00514995  | 0.0  | 0.0  | \-   
F_Heating_Capacity_Tout | 0.34148808  | 0.00894102  | 0.00010787  | 0.0  | \-   
  
These are then used to compute the actual COP and capacity as a function of outdoor temperature, as follows: 

Note that part-load effects (the effect of starting a heating/cooling cycle are not yet accounted for in GridLAB-D). They will be added in a future release. 

### **HVAC and Electrical Loads**

  1. If the Heat system type is not a heat pump, then the heating capacity is
    Heating_capacity = Design_heating_capacity

  1. If the Heat system type is a heat pump, then the heating capacity is determined using Table 6 and the previous section to evaluate F_Heating_Capacity_Tout

    Heating_capacity = Design_heating_capacity * F_Heating_Capacity_Tout

  1. The actual capacity at operating conditions for air conditioning is determined using Table 6 and the previous section to evaluate F_Cooling_Capacity_Tout

    Cooling_capacity = Design_cooling_capacity * F_Cooling_Capacity_Tout

  1. The electrical load of the fan is

    Pfan = Ffan * (HVACon * Fan_power +F2-speed* HVACoff * Fan_power_low_speed_fraction

  1. The sensible heat provided by the HVAC system to the air (Qhvac), with a sign convention of heating positive and cooling negative, is

    Qhvac = Heaton * Heating_capacity + Auxon * Auxiliary_capacity –Coolon * Cooling_capacity / (1 + Latent_cooling_fraction) + Pfan

  1. If use_latent_heat is set to TRUE the latent heat is

    Latent_heat_load = Cooling_capacity * (1 - 1 / Latent_cooling_fraction)

  1. if use_latent_heat is set to FALSE, the latent heat is

    Latent_heat_load = 0;

  1. If the Heat system type is not a heat pump, then the heating COP is

    Heat_COP = 1

  1. If the Heat system type is not a heat pump, then the heating COP is determined using Table 6 and the previous section to evaluate F_Heating_Capacity_Tout

    Heat_COP = Heat_COP_std * F_Heat_COP_Tout

  1. The cooling capacity is determined using Table 6 and the previous section to evaluate F_Cool_COP_Tout

    Cool_COP = Cool_COP_std * F_Cool_COP_Tout

  
  1. Define additional HVAC functionality indicators, indicating electricity as the source for heating and the presence of a two-speed fan

    Felectric = Boolean(Heat_system_type = _heat pump_ <OR>
        
    Heat_system_type = _resistance_ )
    
    F2-speed = Boolean(Fan_type = _2-speed_ )

  1. Define additional state variables indicating that heating/cooling is “on” or “off”

    Hvacon = Boolean( Heaton <OR> Coolon )

    Hvacoff = Boolean( Heatoff & Cooloff )

  
  1. The electrical power drawn by the HVAC system (Phvac, kW) is the sum of the heating, cooling, and fan electricity consumption

    Phvac = FelectricHeaton * ( Heating_Capacity / 3.413 (Btu/hr- kW) ) / Heating_COP + (Cooling_Capacity / 3.413 (Btu/hr-kW) ) * 1 + Latent_Cooling_Fraction) / Cooling_COP + Pfan

## Using House_E

The following section explains how to use house_e inside GLM files. The code snippets are meant as file excerpts and samples. 

### HVAC Settings

Two methods exist for describing the HVAC system presence. 

The first method exclusively uses the "system_type" property for describing a house's HVAC system. It contains a set of values joined by pipes to describe the system. The valid options are 

  * GAS for natural gas heating
  * AIRCONDITIONING for electric cooling units
  * FORCEDAIR for the presence of a central ventilation system (including duct fans)
  * TWOSTAGE for two-speed central ventilation fan systems
  * RESISTIVE for purely resistive heating

If neither GAS nor RESISTIVE are set, the model will assume that a given house has a heat pump, a one-speed central ventilation fan, auxiliary heating with an auxiliary deadband for heating. 

The second method uses a combination of "heating_system_type", "cooling_system_type", "auxiliary_system_type", "auxiliary_strategy", and "fan_type" to describe the HVAC system more explicitly. 

The three system types controls what mechanism, if any, is used. The fan_type controls the presence of central ventilation and whether or not it has a low-power setting. Auxiliary heat strategies can be put together with pipes (with the exception of "NONE", which has adds nothing) to combine the control options. 

The valid settings are: 

heating_system_type 

  * NONE
  * GAS
  * HEAT_PUMP
  * RESISTANCE

| cooling_system_type 

  * NONE
  * ELECTRIC
  * HEAT_PUMP

| fan_type 

  * NONE
  * ONE_SPEED
  * TWO_SPEED

| auxiliary_system_type 

  * NONE
  * ELECTRIC

| auxiliary_strategy 

  * NONE
  * DEADBAND
  * TIMER
  * LOCKOUT

  
It is assumed that any system with a heat pump will have electric auxiliary heating, and that any system with central heating or cooling will include a fan for circulation. 

### Thermal Envelope Settings

The primary values for the thermal envelope of a modeled house are "Rroof", "Rwall", "Rfloor", "Rwindows", and "Rdoors". The R-value of a surface is the inverse of its U-value. If one or more of these values are set explicitly, they will be used over the values set by the following options for that field. 

#### Thermal Integrity Option

"thermal_integrity_level" is usable to provide unambiguous insulation settings. It will override any values not already set, but will override that objects initial values. The valid options are "VERY_LITTLE", "LITTLE", "BELOW_NORMAL", "NORMAL", "ABOVE_NORMAL", "GOOD", "VERY_GOOD", and "UNKNOWN". The default value is "UNKNOWN", which will not use the thermal integrity level value lookup table. 

#### Window Options

The type of glass, the window frame material, the window glazing, and the number of window pane layers can all be set, and will be used to fill in the window R-value and the window solar transmission coefficient. 

Both "aluminum" and "aluminium" are accepted keyword spellings for the window frame type. 

glass_type 

  * OTHER
  * GLASS
  * LOW_E_GLASS

| window_frame 

  * NONE
  * ALUMINUM/ALUMINIUM
  * THERMAL_BREAK
  * WOOD
  * INSULATED

| glazing_treatment 

  * OTHER
  * CLEAR
  * ABS
  * REFL
  * LOW_S
  * HIGH_S

| glazing_layers 

  * ONE
  * TWO
  * THREE
  * OTHER

    
### Example HVAC Configurations

The following snippets can be used to define the HVAC system within a house. The two methods will results in the same system in both cases. The values for the houses with the separate system types are being set explicitly for clarity, even if the defaults would apply the same value. 

Only one method is listed for the auxiliary heating strategies because the older style of describing the HVAC system did not include options for auxiliary heating types, presence, or strategies. A combination with system_type, auxiliary_system_type, and auxiliary_strategy, will work without options colliding. 

Gas heating 
    
    
    object house{
     system_type GAS;
    }
       
    object house{
     heating_system_type GAS;
     cooling_system_type NONE;
     auxiliary_system_type NONE;
    }
    

Heat pump 
    
    
    object house{
     system_type AIRCONDITIONING|FORCEDAIR;
    }
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type HEAT_PUMP;
     auxiliary_system_type ELECTRIC;
     auxiliary_system_type DEADBAND;
     fan_type ONE_SPEED;
    }
    

Heat pump with two-speed fan 
    
    
    object house {
     system_type AIRCONDITIONING|TWOSTAGE;
    }
        
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     fan_type TWO_SPEED;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy DEADBAND;
    }
    

Baseboard heating 
    
    
    object house{
     system_type RESISTIVE;
    }   
    
    object house{
     heating_system_type RESISTANCE;
     cooling_system_type NONE;
     auxiliary_system_type NONE;
     fan_type NONE;
    }
    

Electric heat and central air 
    
    
    object house{
     system_type RESISTIVE|FORCEDAIR;
    }
    
    object house{
     heating_system_type RESISTANCE;
     cooling_system_type NONE;
     auxiliary_system_type NONE;
     fan_type ONE_SPEED;
    }
    

Auxiliary heat with timer 
    
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy TIMER;
    }
    

Auxiliary heat with deadband 
    
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy DEADBAND;
    }
    

Auxiliary heating with Timer and Lockout 
    
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy TIMER|LOCKOUT;
    }
    

Auxiliary heating with Deadband and Lockout 
    
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy DEADBAND|LOCKOUT;
    }
    

Auxiliary heating with Deadband, Lockout, and Timer 
    
    
    object house{
     heating_system_type HEAT_PUMP;
     cooling_system_type ELECTRIC;
     auxiliary_system_type ELECTRIC;
     auxiliary_strategy DEADBAND|LOCKOUT|TIMER;
    }
    

Solar heat gain from the east, south, west, and horizontal with no latent heat, no fan heat, and linear COP and capacity curves 
    
    
    object house{
     include_solar_quadrant H|E|S|W;
     use_latent_heat FALSE;
     include_fan_heatgain FALSE;
     heating_cop_curve LINEAR;
     cooling_cop_curve LINEAR;
     heating_cap_curve LINEAR;
     cooling_cap_curve LINEAR;
    }
    

# New Features

This section will document changes from the original model (as specified above). Most of these are optional features that have been requested by specific projects. 

## Window Openings

TODO: Confirm bug fix, consider adding example with comparison plots

This model is designed to represent the effects of people opening their windows during shoulder temperature periods. This is a complicated human interaction to model, as it is often related to current temperature, forecast of the temperature and other weather conditions, history of temperature, humidity, etc. This model is designed to be a brute force approach to representing this impact. 

Basically, you activate the model (simulate_window_openings is FALSE by default), then describe the upper and lower cutoffs of when the window WILL absolutely be open versus absolutely WILL NOT be open (window_low_temperature_cutoff and window_high_temperature_cutoff). 

The three coefficients listed below then describe a probability curve between the two cutoff points – the probability of opening or closing a window is now a function of the outside air temperature. The “delta” variable is “how often” to update the model as a function of outdoor air temperature, i.e., if outside air changes by 5 degrees, let's see if the human has changed their mind. 

The effect is that if windows are closed, normal operation. If open, the HVAC is overridden (to OFF) and UA is raised by a factor of 10 (which makes heat transfer very fast). 

What we don’t have a great model for is the probability curve or what the cutoff values should be! For previous work, we found that a simple linear between the cutoffs was a pretty good representation. I think for cutoffs, we used -2 to +8 around the setpoint. 

**Parameter** | **Default Value** | **Units**  
---|---|---  
simulate_window_openings | FALSE | Boolean | activates a representation of an occupant opening a window and de-activating the HVAC system   
is_window_open | FALSE | double | (output only) defines the state of the window opening, 1=open, 2=closed   
window_low_temperature_cutoff | 60 | °F | lowest temperature at which the window opening might occur   
window_high_temperature_cutoff | 80 | °F | highest temperature at which the window opening might occur   
window_quadratic_coefficient | 0 | none | quadratic coefficient for describing function between low and high temperature cutoffs   
window_linear_coefficient | 0 | none | linear coefficient for describing function between low and high temperature cutoffs   
window_constant_coefficient | 1 | none | constant coefficient for describing function between low and high temperature cutoffs   
window_temperature_delta | 5 | °F | change in outdoor temperature required to update the window opening model   

# Synopsis
    
    module residential;
    module residential {
      default_outdoor_temperature 74.0 [degF];
      default_humidity 75.0 [%];
      default_etp_iterations 100;
      implicit_enduses LIGHTS|PLUGS|OCCUPANCY|DISHWASHER|MICROWAVE|FREEZER|REFRIGERATOR|RANGE|EVCHARGER|WATERHEATER|CLOTHESWASHER|DRYER;
      house_low_temperature_warning 55 [degF];
      house_high_temperature_warning 95 [degF];
      thermostat_control_warning [TRUE];
      system_dwell_time 1 [s];
      aux_cutin_temperature 10 [degF];
    }
    

# Classes

TODO: Review this list, deprecate or update where needed

As of Four Corners (Version 2.2)

  * house – Single-family home model.
  * residential_enduse – Abstract residential end-use class.
  * waterheater – Typical residential water heating appliance.
  * ZIPload – Generic constant impedance/current/power end-use load.

As of Hassayampa (Version 3.0) - 
    These may be available in earlier versions but they have not been validated and are not supported.

  * lights – Typical residential lights.
  * occupantload – Residential occupants (sensible and latent heat).
  * plugload – Typical residential plug loads.

Unsupported - 
    These may be available in many versions but they have not been validated and are not supported.

  * clotheswasher – Typical residential clothes washing appliance.
  * dishwasher – Typical residential dish washing appliance.
  * dryer – Typical residential clothes drying appliance.
  * evcharger – Standard electric vehicle charger.
  * freezer – Typical residential freezing appliance.
  * microwave – Typical residential microwave appliance.
  * range – Typical residential cooking appliance.
  * refrigerator – Typical residential refrigeration appliance.

# Variables

  * default_line_voltage (complex3) Incoming line voltage to use when no power objects are defined (default is 240V+0j,120V+0j,120V+0j).
  * default_line_current (complex3) Line current across the outside energy meter (default is 0A+0j,0A+0j,0A+0j).
  * default_outdoor_temperature (double) Used when no climate/weather data is available (default is 74 degF).
  * default_humidity (double) Used when no climate/weather data is available (default is 75%).
  * default_solar (double9) Used when no climate/weather data is available (default is 0,0,0,0,0,0,0,0,0).
  * default_etp_iterations (int64) Limits the number of iterations the ETP solver will perform before stopping (default is 100).
  
# Bugs

Due to parsing limitations on arrays default_line_voltage, default_line_current, and default_solar cannot be set from a GLM file. 


# See Also

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

