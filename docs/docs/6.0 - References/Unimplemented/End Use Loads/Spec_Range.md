# Spec:Range


!!! warning

	This page contains features that are unfinished, were never implemented, or have since been deprecated. We preserve these pages for archival purposes, and also as a foundational resource for prospective developers who may wish to implement the same or similar feature. Many of these pages provide robust explanations of the theory behind a particular module or feature that we hope readers will find useful. 
  
	**This page does not reflect the current state of GridLAB-D™**
  
The purpose of the range model in GridLAB-D™ is to facilitate the real representation of the oven and cooktop energy consumptions profile. 

## General Description

Electric range has oven and an electronic control for the cooktop. It converts electrical energy into heat to cook and bake. 

### Oven

The food in the oven is heated by an electrical element and is controlled by a thermostat. Oven element capacity (wattage) ranges from about 500 W to 2500 W, with 1000 W being common. It heats to the temperature that the user sets it to. Thermostatic controls have a deadband associated with the setpoint to prevent rapid cycling of power to the elements, which would result if the turn-on temperature equaled the turn-off temperature. The deadband is typically a few degrees above and below the nominal setpoint. 

The oven GridLAB-D™ model is similar to that of GridLAB-D™ waterheater one-node model. 

### Cooktop

The cooktop has burners on the top and is usually installed into a countertop. These are essentially perfectly resistive loads. Each burner on a cooktop can be controlled by the user-controlled knob settings. 

# Modeling Assumptions

  * The temperature inside the oven is considered to be uniform throughout.
  * Three cooktop settings are considered for the cooktop model.
  * The cooktop is a timer-based model. This implies that the operating time of the cooktop depends on time settings rather than on system voltage.

# Equations

## Oven Equations

Table: Equation Notation { #tbl:equation-notation }

Variable | Definition   
---|---  
$T_{on}$ | Lower setpoint temperature of oven [degF]   
$T_{off}$ | Upper setpoint temperature of oven [degF]   
$\dot{m}$ | mass flow [lb/hr]   
$T_{amb}$ | Ambient temperature [degF]  
$GALPCF$ | Gallons to cubic foot conversion factor   
$BTUPHPKW$ | [Btu/hr] to [kw] conversion factor   
$\Delta_{t}$ | The time required to change the oven's temperature from an intial temperature ($T_0$) to a new temperature ($T_1$)   
$C_w$ | Thermal capacitance   
ovenUA | Thermal Conductance   
$P_l$ | Load power   
$I_l$ | Load current   
$Z_l$ | Load admittance   
$V_l$ | Load voltage factor   
$ C_{heat}$ | Heating element capacity   
$p_l$ | Load power fraction   
$i_l$ | Load current fraction   
$z_l$ | Load impedance fraction   
$P_{total}$ | Total power   
$E_{total}$ | Energy used   
$T_{set}$ | Oven setpoint   
$db$ | Thermostat deadband   
ovenDemand | oven demand in [gal/min]  
$\rho$ | density of food in pounds per cubic feet [lb/cf]
$C_p$ |   
$c_{food}$ | Specific heat of the food   
  
### Total Power Calculation

$$\begin{align}P_l &= C_{heat} \cdot p_l\\

I_l &= C_{heat} \cdot i_l\\ Z_l &= C_{heat} \cdot z_l\end{align}$$

$$ P_{total} = (P_l + (I_l + Z_lV_l) \cdot V_l) \cdot 1000$$

$$ E_{total} = \frac{P_{total}}{1000} \cdot \frac{\Delta t}{3600}$$

### Thermostat Setpoint Temperatures

$$\begin{align}T_{on}&=T_{set}-\frac{db}{2}\\

T_{off}&=T_{set}-\frac{db}{2}\end{align}$$

### New Time Calculation

Estimate mass flow: 

$$ \dot{m}= \text{ovenDemand} \cdot 60 \cdot \frac{\rho}{GALPCF}$$

Calculate new time 

$$\begin{align}\Delta t &= \frac{\text{log}(c_1+c_2T_1)-\text{log}(c_1+c_2T_0)}{c_2}\\

c_{11}&=\frac{\text{ovenUA} + \dot{m}_{C_p}}{C_w}\\ c_{22}&=\frac{(P_{total}\cdot BTUPHPKW) + (\dot{m}\cdot c_{food})+(\text{ovenUA} \cdot T_{amb})}{\text{ovenUA}+(\dot{m} \cdot c_{food})}\\ T_{new}&=c_{22}-(c_{22}-T_0) \cdot exp(-c_{11} \cdot \Delta t)\end{align}$$

where 

$$\begin{align}c_1 &= \frac{(P_{total} \cdot BTUPHPKW + ovenUA \cdot T_{amb} + \dot{m} \cdot C_p \cdot T_{inlet})}{C_w}\\

c_2&=\frac{-(ovenUA+\dot{m}\cdot c_{food})}{C_w}\\ C_w &= \frac{v_{oven}}{GALPCF} \cdot \rho \cdot c_{food}\end{align}$$

# Solver

## Interfacing Overview

### Object Inclusion

### Published Inputs

The user may input values for the following variables related to the oven model. 

Table: Oven inputs { #tbl:oven-inputs }

Variable | Type | Units | Default | Definition   
---|---|---|---|---  
oven_volume | double | gal | 5 | Volume of oven   
heating_element_capacity | double | kw | 1 | Power rating of heating element   
oven_setpoint | double | degF | 100 | Setpoint temperature of oven   
temperature | double | degF | 70 | Initial temperature   
thermostat_deadband | double | degF | 8 | Deadband around oven_setpoint temperature (half above, half below)   
location | bool | n/a | INSIDE | location of oven   
oven_UA | double | BTU/hr.F | 2.9 | Thermal conductance of the oven   
food_density | double | lb/cf | 5 | Density of the food   
specificheat_food | double | Btu/lb.degF | 1 | Specific heat of the food   
time_oven_setting | double | s | 3600 | Cycle time of oven   
load_impedence_fraction | double | n/a | 1 | Constant impedance component fraction   
load_current_fraction | double | n/a | 0 | Constant current component fraction   
load_power_fraction | double | n/a | 0 | Constant power component fraction   
queue_oven | double | n/a | 0.85 | Oven is placed in its 'queue' and awaiting to be turned on   
demand_oven | double | n/a | RANGE * 20 | The probability that a given oven is turned on depends on demand_oven, and the value of the normalized oven load shape at any given time. The higher these quantities are, the higher the probability of the given appliance turning on (GE CRADA report)   
Table: Cooktop inputs { #tbl:cooktop-inputs }

Variable | Type | Units | Default | Definition   
---|---|---|---|---  
cooktop_energy_baseline | double | kwh | 0.5 | The amount of energy needed for a cooktop event   
cooktop_coil_setting_1 | double | W | 2 | Power rating of the cooktop's high level setting   
cooktop_coil_setting_2 | double | W | 1 | Power rating of the cooktop's low leven setting   
cooktop_coil_setting_3 | double | W | 1.7 | Power rating of the cooktop's medium level setting   
cooktop_interval_setting_1 | double | s | 240 | Cook time of setting 1   
cooktop_interval_setting_2 | double | s | 900 | Cook time of setting 2   
cooktop_interval_setting_3 | double | s | 120 | The amount of energy needed for a cooktop event   
time_cooktop_setting | double | s | 2000 | Cycle time of cooktop   
demand_cooktop | double | n/a | RANGE* 35 | The probability that a given cooktop is turned on depends on demand_cooktop and the value of the normalized appliance load shape at any given time. The higher these quantities are, the highter the probability of the given appliance turning on (GE CRADA report)   
queue_cooktop | double | n/a | 0.99 | Cooktop is placed in its 'queue' and awaiting its turn to be ON   
queue_min | double | n/a | 0 | Minimum 'queue' value considered   
queue_max | double | n/a | 2 | Maximum 'queue' value considered   
  
### Published Outputs

Table: Range Outputs { #tbl:range-outputs }

Variable | Type | Units | Definition   
---|---|---|---  
total_power_oven | double | kw | Total power required during the oven cycle   
total_power_cooktop | double | kw | Total power required during the cooktop cycle   
cooktop_energy_used | double | kwh | Total energy consumed for cooktop cycle   
time_cooktop_operation | double | s | Duration of cooktop in each setting   
Toff | double | degF | Upper setpoint temperature   
Ton | double | degF | Lower setpoint temperature   
time_oven_operation | double | s | Incremental change of oven operation time when it is ON   
time_oven_setting | double | s | Total ON time for electric oven   
  
### Data Structure

To facilitate data operations between the individual objects and the dynamic solver capability, a common data structure will be used to pass information back and forth. This data structure should contain information and pointers to the following elements. 

Table: Range interface elements { #tbl:range-interface-elements }

Variable | Definition   
---|---  
timestamp | Pointer to current timestamp of the solution   
timestamp_change | Pointer to the difference between the last and current timestamp  
energy | Pointer to accumulated energy consumption of the system   
voltage | Pointer to complex voltage values of the object  
current | Pointer to complex current values of the object  
power | Pointer to complex power contributions of the object  
impedance | Pointer to complex impedance contributions of the object  

## Solver Timing

The range model will need to be properly timed with the powerflow solution, as well as the requirements of the individual range components. 

### Solver Passes

The oven model follows these steps: 

  1. Solve the time required to change the oven's temperature if the oven's inside temperature is lower than the lower setpoint temperature.
  2. Solve the oven interface components based on its settings.
  3. Update the energy calculation.

After these steps are complete, the simulation advances to the next timestamp. This sequence will repeat until the next GridLAB-D™ overall timestamp is encountered. At that point, the changes will be reflected into the quasi-steady state powerflow solution, and the process will repeat until the given energy consumption is elapsed. 

The cooktop model follows these steps: 

  1. Solve the cooktop interface conponents based on its settings.
  2. Update energy calculation.

### Solution Timesteps

Add description like in Spec:Microgrids. 

### Solver Call Timing

Add description like in Spec:Microgrids. 

# Testing And Validation

Include finalized testing and validation. 

# References

  1. _IEEE power & energy magazine_; May/June 2010
  
# Related Concepts:

  * Range User Manual
  * Residential Module
