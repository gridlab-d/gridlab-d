# GridLAB-D™ QSTS Inverter Model

## Overview

The **inverter** object in GridLAB-D™'s generators module is the central component for modeling power electronic interfaces between DC energy resources — such as photovoltaic (PV) arrays and battery storage systems (BSS) — and the AC distribution network. In the QSTS engine the network power flow solver only solves for for voltages and currents at the nodes. Therefore, the **inverter** object, implemented in ```inverter.cpp```, serves as the AC-DC boundary object through which all DER power injections are computed, and communicated to the powerflow solver.

### Design Philosophy: Multi Model Object

The original design of the **inverter** object implementation developed a clear architectural distinction between possible modeling regimes selected by the user through the ```inverter_type``` and ```four_quadrant_control_mode``` properties.

Currently, the feature-complete and recommended modeling approach is the ```FOUR_QUADRANT``` model, which supports a wide range of control modes. The other models, purposely named "legacy" models — ```TWO_PULSE```, ```SIX_PULSE```, ```TWELVE_PULSE```, ```PWM``` — are retained for backward compatibility but are not recommended for new work. The control mode selection is cleanly organized such that only the relevant parameters for the selected mode are active, and the code paths are well modularized to prevent unintended interactions between modes.

The Four-Quadrant Model (FOUR_QUADRANT) is the current, feature-complete representation. Setting ```inverter_type``` to ```FOUR_QUADRANT``` — the four-quadrant model — unlocks the ```four_quadrant_control_mode``` enumeration, which selects from a set of control strategies — from simple constant power dispatch to smart-inverter functions, such as Volt-VAR and Volt-Watt.

### Network Interface

The inverter connects to the distribution network by parenting to a powerflow node, meter, load, or their triplex (split-phase) equivalents. During initialization, the object maps directly to the parent node's voltage, current, and power variables by phase — enabling it to both read terminal conditions and inject current or power back into the network solution.

### Scope of the Model

Across its four-quadrant control operation mode, the inverter object covers:

- DC-to-AC power conversion with efficiency modeling (fixed scalar or multipoint curve based on manufacturer data),
- Four-quadrant real (P) and reactive (Q) power dispatch with rated power and current limiting,
- Power factor regulation with configurable activation thresholds and lockout timers,
- Smart inverter control functions: Volt-VAR, Volt-Watt, and combined Volt-VAR-Frequency-Power modes,
- Load-following and group load-following dispatch for battery-coupled inverters.

The following section describes the functional areas in detail, including the specific parameters that govern their behavior, their default values, and the conditions under which each mode is active.

## Parameters and Functionality

For a comprehensive list of the inverter's parameters and variables, as listed in the source code, including their units, types, descriptions, and any special notes on their usage or activation conditions, please refer to [](#tbl:inverter-parameters) at the end of this document. The table is organized by functional category, with parameters grouped according to their role in the model (e.g., control mode selection, power rating, efficiency modeling, etc.) for ease of reference.

### 1. Model Selection: Four-Quadrant

The inverter's behavior is organized around two distinct modeling tiers, selected through a pair of properties, ```inverter_type``` and ```generator_mode```. However, everything labeled "legacy" is retained for backwards compatibility, but not advised for new simulations <mark style="background-color: lightgreen;">and, hence, not documented (or maybe at the end of this file?????)</mark> . Thus, the recommended model is ```inverter_type FOUR_QUADRANT```, which enables the full suite of control modes and features described in section [2](#2-four-quadrant-control-modes). Consequently, ```generator_mode``` should be set to ```SUPPLY_DRIVEN``` when using the ```FOUR_QUADRANT``` model, and the four-quadrant control mode property takes over as the primary mode selector.

### 2. Four-Quadrant Control Modes

When the inverter type is set to ```FOUR_QUADRANT```, the device can operate anywhere in the P-Q plane within its rated apparent power circle. The ```four_quadrant_control_mode``` is the central dispatch property of the inverter model. In GridLAB-D™ it selects from the following modes:

- ```NONE```: The inverter object is present but performs no active control. It allows performing simples testing or integrating with externally developed controllers.
- ```CONSTANT_PQ```: The inverter outputs fixed real and reactive power as specified by ```P_Out``` (W) and ```Q_Out``` (VAr). This is the simplest grid-connected dispatch mode. The output is not voltage-sensitive and does not adjust unless ```P_Out``` or ```Q_Out``` are externally modified (e.g., via a player or controller object).
- ```CONSTANT_PF```: The inverter operates at a fixed power factor specified by ```power_factor```, emulating equation $Q = P \cdot \tan(\cos^{-1}(\text{power\_factor}))$. Real power is determined by the attached resource (solar irradiance, battery dispatch), and reactive power is computed as the corresponding lagging or leading component at the declared power factor. This is the default mode when ```four_quadrant_control_mode``` is not explicitly set.
- ```VOLT_VAR```: The inverter implements a piecewise-linear Volt-VAR curve $Q = f(V_{terminal})$ defined by four voltage-reactive power breakpoints: (```V1```, ```Q1```), (```V2```, ```Q2```), (```V3```, ```Q3```), (```V4```, ```Q4```). Voltages are specified in per-unit relative to ```V_base```, and reactive power values are in per-unit of rated power. The inverter reads its terminal voltage each timestep and interpolates the appropriate Q injection from the curve. A lockout timer (```volt_var_control_lockout```, in seconds) prevents rapid successive changes.
- ```VOLT_WATT```: The inverter limits its real power output as a function of terminal voltage $P = f(V_{terminal})$, using a two-point linear ramp defined by (```VW_V1```, ```VW_P1```) and (```VW_V2```, ```VW_P2```). Below ```VW_V1``` the inverter operates at full available power; above ```VW_V2``` it curtails to ```VW_P2```. This implements the Volt-Watt curtailment function for over-voltage mitigation.
- ```VOLT_VAR_FREQ_PWR```: This is a combined operational mode that simultaneously applies a Volt-VAR schedule and a frequency-power schedule. The VAR response is driven by terminal voltage via ```volt_var_sched```, and real power is modified as a function of measured frequency via ```freq_pwr_sched```. Slew rate limits (```max_var_slew_rate``` in VAr/s, ```max_pwr_slew_rate``` in W/s) prevent instantaneous jumps. A delay_time parameter introduces a response delay between observing a voltage or frequency deviation and applying the corresponding output change. The ```disable_volt_var_if_no_input_power``` flag allows the Volt-VAR function to be suppressed when the DC source (e.g., PV) is not producing power.
- ```LOAD_FOLLOWING```: The inverter monitors the power flow on a designated ```sense_object``` (a node or link) and dispatches the attached battery to charge or discharge in order to keep that power within defined thresholds. Charging begins when the sensed power falls below ```charge_on_threshold``` and stops at ```charge_off_threshold```. Discharging begins above ```discharge_on_threshold``` and stops at ```discharge_off_threshold```. Rates are bounded by ```max_charge_rate``` and ```max_discharge_rate```. Lockout timers (```charge_lockout_time```, ```discharge_lockout_time```) prevent rapid mode cycling.
- ```GROUP_LOAD_FOLLOWING```: This mode is an extension of load-following where multiple inverters coordinate their battery dispatch collectively. The group shares a common set of thresholds (```charge_threshold```, ```discharge_threshold```) and aggregate rate limits (```group_max_charge_rate```, ```group_max_discharge_rate```, ```group_rated_power```). Individual inverters in the group scale their dispatch proportionally.
- ```VOLTAGE_SOURCE (VSI)``` Activates the voltage source inverter model for grid-forming operation. In this mode the inverter synthesizes its own internal voltage and acts as a controlled voltage source rather than a current source. <mark style="background-color: lightgreen;">This mode is only active in deltamode and is described further in Section 7, so should techinically be taken out from here.</mark>.

### 3. Core Identification and Status Parameters

These parameters govern what the inverter is and whether it is active in the simulation.

- ```generator_status``` can be either ```OFFLINE``` or ```ONLINE``` and controls whether the inverter participates in the power flow solution at all. When set to ```OFFLINE```, the object exists in the model but contributes no injection. It defaults to ```ONLINE``` at runtime. <mark style="background-color: lightgreen;">CAN WE VERIFY THIS IS ACTUALLY WHAT HAPPENS?????.</mark>
- ```phases``` (A, B, C, N, S) declares which AC phases the inverter is connected to. This drives the phase-specific variable mapping performed during the initialization stage. For example, a single-phase rooftop PV on phase B will only inject into that phase's current and power accumulators. The S flag denotes split-phase (triplex) connection.
- ```islanded_state``` (boolean) signals to all control modes that the inverter is operating in an islanded network. When true, several control modes alter their behavior. For example, load-following modes account for the ```soc_reserve``` floor to ensure the battery retains capacity for sustained island operation.

### 4. Power Rating and Efficiency Parameters

These parameters define the fundamental physical limits of the inverter and its conversion efficiency, which are critical for ensuring that the simulated behavior reflects real-world constraints.

- ```rated_power``` (VA): The rated apparent power capacity of the inverter is the primary sizing parameter and sets the ceiling on total VA output. It also serves as the base for per-unit calculations in Volt-VAR and Volt-Watt curves.
- ```rated_battery_power``` (W): When a battery is attached, this separately declares the battery's rated real power, which may differ from the inverter's total VA rating.
- ```inverter_efficiency```: This scalar between 0 and 1 applies to the DC input power to compute available AC output in the four-quadrant model. For example, a value of 0.95 means 5% of input power is lost in conversion. This is a flat efficiency — for more accurate representation across operating points, the multipoint efficiency model should be used instead.
- ```battery_soc``` (pu): This parameter represents the current state of charge (SoC) of an attached battery, ranging from 0 to 1. The inverter reads this to determine whether charging or discharging is permissible. It does not write this value directly — the battery object owns the SoC state.
- ```soc_reserve``` (pu): The minimum SOC fraction that the battery must retain in islanded operation. The inverter will not discharge below this level when ```islanded_state``` is true.

### 5. Multipoint Efficiency Model

When ```use_multipoint_efficiency``` is set to true and a solar object is the child resource, the inverter replaces the flat ```inverter_efficiency``` scalar with a California Energy Commission (CEC) model efficiency curve. This model is parameterized by:

|Parameter|Description|
|---------|-----------|
|```maximum_dc_power```|DC power at which the inverter reaches rated output|
|```maximum_dc_voltage```|DC voltage at the maximum power point|
|```minimum_dc_power```|Minimum DC power required for the inverter to start|
|```c_0```, ```c_1```, ```c_2```, ```c_3```|Polynomial coefficients of the efficiency curve|

Pre-configured coefficient sets are available for three manufacturers via the ```inverter_manufacturer```property: ```FRONIUS```, ```SMA```, and ```XANTREX```. Setting one of these populates the coefficients automatically.

### 6. Power Factor Regulation

Power factor regulation is an auxiliary function that can be layered on top of load-following or group load-following modes. It is enabled by setting ```pf_reg``` (```EXCLUDED``` by default) to ```INCLUDED``` or ```INCLUDED_ALT```. The following parameters drive the behavior of the power factor regulation:

- ```pf_reg_activate```: This threshold represents the power factor magnitude below which regulation activates. When the power factor at the ```sense_object``` drops below this threshold, the inverter begins injecting reactive power to correct it.
- ```pf_reg_deactivate```: This threshold represents the power factor magnitude above which regulation is no longer needed and the inverter returns to its baseline dispatch.
- ```pf_target```: The desired power factor to maintain. In GridLAB-D™ the sign convention is
  - positive if inductive (lagging), 
  - negative if capacitive (leading).
- ```pf_reg_high``` / ```pf_reg_low```: Outer bounds for the power factor regulation band. If the measured power factor exceeds ```pf_reg_high```, the inverter goes to full reverse reactive injection. If it falls below ```pf_reg_low```, regulation ceases entirely.
- ```pf_reg_activate_lockout_time```(s): A mandatory pause between the deactivation of power factor regulation and its reactivation, preventing rapid oscillation.


### 7. DC Interface Parameters

- ```V_In``` (V), ```I_In``` (A), ```P_In``` (W): The DC-side voltage, current, and power. For solar-connected inverters, ```P_In``` is driven by the child solar object's computed output. For battery-connected inverters, the battery object populates this based on its dispatch state. While ```P_In``` is the primary input in most configurations, representing the DC power delivered by the attached resource, ```V_In``` and ```I_In``` are available for configurations where the DC voltage or current are independently meaningful.
- ```Vdc``` (V): A legacy DC voltage parameter retained for backward compatibility with older models that used the ```TWO/SIX/TWELVE_PULSE``` inverter types.

### 8. AC Output Observable Variables

These are the primary output quantities applied at the network connection node and that can be recorded in output players or recorders:

|Property|Description|
|---|---|
|```VA_Out```|Total AC apparent power output (complex, VA). This is the aggregate across all active phases.|
|```power_A/B/C```|Per-phase apparent power (complex, VA)|
|```phaseA/B/C_V_Out```|Per-phase AC terminal voltage (complex, V)|
|```phaseA/B/C_I_Out```|Per-phase AC current (grid-following)|
|```curr_VA_out_A/B/C```|Current-timestep per-phase power (used for convergence)|
|```prev_VA_out_A/B/C```|Previous-timestep per-phase power (used for convergence)|


## Summary of Key Parameters

Table: Key Parameters and Variables of the GridLAB-D™ Inverter Object { #tbl:inverter-parameters }

|Parameter Name|Unit|Type|Description|
|---|---|---|---|
|```inverter_type```||enumeration|**LEGACY MODEL:** Sets efficiencies and other parameters; if using four_quadrant_control_mode, set this to FOUR_QUADRANT [TWO_PULSE, SIX_PULSE, TWELVE_PULSE, PWM, FOUR_QUADRANT]|
|```generator_mode```||enumeration|LEGACY MODEL: Selects generator control mode when using legacy model; in non-legacy models, this should be SUPPLY_DRIVEN. [UNKNOWN, CONSTANT_V, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN]|
|```four_quadrant_control_mode```||enumeration|FOUR QUADRANT MODEL: Activates various control modes [NONE, CONSTANT_PQ, CONSTANT_PF, VOLT_VAR, VOLT_WATT, VOLT_VAR_FREQ_PWR, LOAD_FOLLOWING, GROUP_LOAD_FOLLOWING, VOLTAGE_SOURCE]|
|```generator_status```||enumeration|describes whether the generator is online or offline [OFFLINE, ONLINE]|
|```phases```||set|The phases the inverter is attached to [A, B, C, N, S]|
|```islanded_state```||bool|FOUR QUADRANT MODEL: Boolean used to let control modes to act under island conditions|
|```rated_power```|VA|double|FOUR QUADRANT MODEL: The rated power of the inverter|
|```rated_battery_power```|W|double|FOUR QUADRANT MODEL: The rated power of battery when battery is attached|
|```inverter_efficiency```||double|FOUR QUADRANT MODEL: The efficiency of the inverter|
|```battery_soc```|pu|double|FOUR QUADRANT MODEL: The state of charge of an attached battery|
|```soc_reserve```|pu|double|FOUR QUADRANT MODEL: The reserve state of charge of an attached battery for islanding cases|
|```P_Out```|VA|double|FOUR QUADRANT MODEL: Scheduled real power out in CONSTANT_PQ control mode|
|```Q_Out```|VAr|double|FOUR QUADRANT MODEL: Schedule reactive power out in CONSTANT_PQ control mode|
|```power_factor```|unit|double|FOUR QUADRANT MODEL: The power factor used for CONSTANT_PF control mode|
|```V_base```|V|double|FOUR QUADRANT MODEL: The base voltage on the grid side of the inverter. Used in VOLT_VAR control mode.|
|```V1```|pu|double|FOUR QUADRANT MODEL: voltage point 1 in volt/var curve. Used in VOLT_VAR control mode.|
|```Q1```|pu|double|FOUR QUADRANT MODEL: VAR point 1 in volt/var curve. Used in VOLT_VAR control mode.|
|```V2```|pu|double|FOUR QUADRANT MODEL: voltage point 2 in volt/var curve. Used in VOLT_VAR control mode.|
|```Q2```|pu|double|FOUR QUADRANT MODEL: VAR point 2 in volt/var curve. Used in VOLT_VAR control mode.|
|```V3```|pu|double|FOUR QUADRANT MODEL: voltage point 3 in volt/var curve. Used in VOLT_VAR control mode.|
|```Q3```|pu|double|FOUR QUADRANT MODEL: VAR point 3 in volt/var curve. Used in VOLT_VAR control mode.|
|```V4```|pu|double|FOUR QUADRANT MODEL: voltage point 4 in volt/var curve. Used in VOLT_VAR control mode.|
|```Q4```|pu|double|FOUR QUADRANT MODEL: VAR point 4 in volt/var curve. Used in VOLT_VAR control mode.|
|```volt_var_control_lockout```|s|double|FOUR QUADRANT QUADRANT MODEL: the lockout time between volt/var actions.|
|```VW_V1```|pu|double|FOUR QUADRANT MODEL: Voltage at which power limiting begins (e.g. 1.0583). Used in VOLT_WATT control mode.|
|```VW_V2```|pu|double|FOUR QUADRANT MODEL: Voltage at which power limiting ends. (e.g. 1.1000). Used in VOLT_WATT control mode.|
|```VW_P1```|pu|double|FOUR QUADRANT MODEL: Power limit at VW_P1 (e.g. 1). Used in VOLT_WATT control mode.|
|```VW_P2```|pu|double|FOUR QUADRANT MODEL: Power limit at VW_P2 (e.g. 0). Used in VOLT_WATT control mode.|
|```volt_var_sched```||char1024||
|```freq_pwr_sched```||char1024||
|```max_var_slew_rate```|VAr/s|double||
|```max_pwr_slew_rate```|W/s|double||
|```disable_volt_var_if_no_input_power```||bool||
|```delay_time```|s|double||
|```sense_object```||object|FOUR QUADRANT MODEL: name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING|
|```max_charge_rate```|W|double|FOUR QUADRANT MODEL: maximum rate the battery can be charged in LOAD_FOLLOWING|
|```max_discharge_rate```|W|double|FOUR QUADRANT MODEL: maximum rate the battery can be discharged in LOAD_FOLLOWING|
|```charge_on_threshold```|W|double|FOUR QUADRANT MODEL: power level at which the inverter should try charging the battery in LOAD_FOLLOWING|
|```charge_off_threshold```|W|double|FOUR QUADRANT MODEL: power level at which the inverter should cease charging the battery in LOAD_FOLLOWING|
|```discharge_on_threshold```|W|double|FOUR QUADRANT MODEL: power level at which the inverter should try discharging the battery in LOAD_FOLLOWING|
|```discharge_off_threshold```|W|double|FOUR QUADRANT MODEL: power level at which the inverter should cease discharging the battery in LOAD_FOLLOWING|
|```charge_lockout_time```|s|double|FOUR QUADRANT MODEL: Lockout time when a charging operation occurs before another LOAD_FOLLOWING dispatch operation can occur|
|```discharge_lockout_time```|s|double|FOUR QUADRANT MODEL: Lockout time when a discharging operation occurs before another LOAD_FOLLOWING dispatch operation can occur|
|```charge_threshold```|W|double|FOUR QUADRANT MODEL: Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level.|
|```discharge_threshold```|W|double|FOUR QUADRANT MODEL: Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level.|
|```group_max_charge_rate```|W|double|FOUR QUADRANT MODEL: Sum of the charge rates of the batteries involved in the group load-following.|
|```group_max_discharge_rate```|W|double|FOUR QUADRANT MODEL: Sum of the discharge rates of the batteries involved in the group load-following.|
|```group_rated_power```|W|double|FOUR QUADRANT MODEL: Sum of the inverter power ratings of the inverters involved in the group power-factor regulation.|
|```use_multipoint_efficiency```||bool|FOUR QUADRANT MODEL: boolean to used the multipoint efficiency curve for the inverter when solar is attached|
|```inverter_manufacturer```||enumeration|MULTIPOINT EFFICIENCY MODEL: the manufacturer of the inverter to setup up pre-existing efficiency curves [NONE, FRONIUS, SMA, XANTREX]|
|```maximum_dc_power```||double|MULTIPOINT EFFICIENCY MODEL: the maximum dc power point for the efficiency curve|
|```maximum_dc_voltage```||double|MULTIPOINT EFFICIENCY MODEL: the maximum dc voltage point for the efficiency curve|
|```minimum_dc_power```||double|MULTIPOINT EFFICIENCY MODEL: the minimum dc power point for the efficiency curve|
|```c_0```||double|MULTIPOINT EFFICIENCY MODEL: the first coefficient in the efficiency curve|
|```c_1```||double|MULTIPOINT EFFICIENCY MODEL: the second coefficient in the efficiency curve|
|```c_2```||double|MULTIPOINT EFFICIENCY MODEL: the third coefficient in the efficiency curve|
|```c_3```||double|MULTIPOINT EFFICIENCY MODEL: the fourth coefficient in the efficiency curve|
|```pf_reg```||enumeration|Activate (or not) power factor regulation in four_quadrant_control_mode [INCLUDED, INCLUDED_ALT, EXCLUDED]|
|```pf_reg_activate```||double|FOUR QUADRANT MODEL: Lowest acceptable power-factor level below which power-factor regulation will activate.|
|```pf_reg_deactivate```||double|FOUR QUADRANT MODEL: Lowest acceptable power-factor above which no power-factor regulation is needed.|
|```pf_target```||double|FOUR QUADRANT MODEL: Desired power-factor to maintain (signed) positive is inductive|
|```pf_reg_high```||double|FOUR QUADRANT MODEL: Upper limit for power-factor - if exceeds, go full reverse reactive|
|```pf_reg_low```||double|FOUR QUADRANT MODEL: Lower limit for power-factor - if exceeds, stop regulating - pf_target_var is below this|
|```pf_reg_activate_lockout_time```|s|double|FOUR QUADRANT MODEL: Mandatory pause between the deactivation of power-factor regulation and it reactivation|
|```V_In```|V|double|DC voltage|
|```I_In```|A|double|DC current|
|```P_In```|W|double|DC power|
|```VA_Out```|VA|complex|AC power|
|```Vdc```|V|double|LEGACY MODEL: DC voltage|
|```phaseA_V_Out```|V|complex|AC voltage on A phase in three-phase system; 240-V connection on a triplex system|
|```phaseB_V_Out```|V|complex|AC voltage on B phase in three-phase system|
|```phaseC_V_Out```|V|complex|AC voltage on C phase in three-phase system|
|```phaseA_I_Out```|V|complex|AC current on A phase in three-phase system; 240-V connection on a triplex system|
|```phaseB_I_Out```|V|complex|AC current on B phase in three-phase system|
|```phaseC_I_Out```|V|complex|AC current on C phase in three-phase system|
|```power_A```|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|
|```power_B```|VA|complex|AC power on B phase in three-phase system|
|```power_C```|VA|complex|AC power on C phase in three-phase system|
|```curr_VA_out_A```|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|
|```curr_VA_out_B```|VA|complex|AC power on B phase in three-phase system|
|```curr_VA_out_C```|VA|complex|AC power on C phase in three-phase system|
|```prev_VA_out_A```|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|
|```prev_VA_out_B```|VA|complex|AC power on B phase in three-phase system|
|```prev_VA_out_C```|VA|complex|AC power on C phase in three-phase system|