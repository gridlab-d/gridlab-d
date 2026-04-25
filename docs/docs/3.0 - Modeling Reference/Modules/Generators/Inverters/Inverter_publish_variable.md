# GridLAB-D Published Variables

Total variables found: 193

## Summary Statistics

### Variable Types

- **double**: 141
- **complex**: 27
- **enumeration**: 10
- **bool**: 10
- **char1024**: 2
- **set**: 1
- **object**: 1
- **int32**: 1

### Access Levels

- **PUBLIC**: 184
- **PA_HIDDEN**: 9

---

## Complete Variable Table

|Published Name|Unit|Type|Description|Access|
|---|---|---|---|---|
|**inverter_type**||enumeration|LEGACY MODEL: Sets efficiencies and other parameters; if using four_quadrant_control_mode, set this to FOUR_QUADRANT [TWO_PULSE, SIX_PULSE, TWELVE_PULSE, PWM, FOUR_QUADRANT]|PUBLIC|
|**four_quadrant_control_mode**||enumeration|FOUR QUADRANT MODEL: Activates various control modes [NONE, CONSTANT_PQ, CONSTANT_PF, VOLT_VAR, VOLT_WATT, VOLT_VAR_FREQ_PWR, LOAD_FOLLOWING, GROUP_LOAD_FOLLOWING, VOLTAGE_SOURCE]|PUBLIC|
|**pf_reg**||enumeration|Activate (or not) power factor regulation in four_quadrant_control_mode [INCLUDED, INCLUDED_ALT, EXCLUDED]|PUBLIC|
|**generator_status**||enumeration|describes whether the generator is online or offline [OFFLINE, ONLINE]|PUBLIC|
|**generator_mode**||enumeration|LEGACY MODEL: Selects generator control mode when using legacy model; in non-legacy models, this should be SUPPLY_DRIVEN. [UNKNOWN, CONSTANT_V, CONSTANT_PQ, CONSTANT_PF, SUPPLY_DRIVEN]|PUBLIC|
|**inverter_convergence_criterion**||double|The maximum change in error threshold for exitting deltamode.|PUBLIC|
|**current_convergence**|A|double|Convergence criterion for current changes on first timestep - basically initialization of system|PUBLIC|
|**V_In**|V|double|DC voltage|PUBLIC|
|**I_In**|A|double|DC current|PUBLIC|
|**P_In**|W|double|DC power|PUBLIC|
|**VA_Out**|VA|complex|AC power|PUBLIC|
|**Vdc**|V|double|LEGACY MODEL: DC voltage|PUBLIC|
|**phaseA_V_Out**|V|complex|AC voltage on A phase in three-phase system; 240-V connection on a triplex system|PUBLIC|
|**phaseB_V_Out**|V|complex|AC voltage on B phase in three-phase system|PUBLIC|
|**phaseC_V_Out**|V|complex|AC voltage on C phase in three-phase system|PUBLIC|
|**phaseA_I_Out**|V|complex|AC current on A phase in three-phase system; 240-V connection on a triplex system|PUBLIC|
|**phaseB_I_Out**|V|complex|AC current on B phase in three-phase system|PUBLIC|
|**phaseC_I_Out**|V|complex|AC current on C phase in three-phase system|PUBLIC|
|**power_A**|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|PUBLIC|
|**power_B**|VA|complex|AC power on B phase in three-phase system|PUBLIC|
|**power_C**|VA|complex|AC power on C phase in three-phase system|PUBLIC|
|**curr_VA_out_A**|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|PUBLIC|
|**curr_VA_out_B**|VA|complex|AC power on B phase in three-phase system|PUBLIC|
|**curr_VA_out_C**|VA|complex|AC power on C phase in three-phase system|PUBLIC|
|**prev_VA_out_A**|VA|complex|AC power on A phase in three-phase system; 240-V connection on a triplex system|PUBLIC|
|**prev_VA_out_B**|VA|complex|AC power on B phase in three-phase system|PUBLIC|
|**prev_VA_out_C**|VA|complex|AC power on C phase in three-phase system|PUBLIC|
|**e_source_A**||double|DELTAMODE: Internal voltage of grid-forming source, phase A|PA_HIDDEN|
|**e_source_B**||double|DELTAMODE: Internal voltage of grid-forming source, phase B|PA_HIDDEN|
|**e_source_C**||double|DELTAMODE: Internal voltage of grid-forming source, phase C|PA_HIDDEN|
|**V_angle_A**||double|DELTAMODE: Internal angle of grid-forming source, phase A|PA_HIDDEN|
|**V_angle_B**||double|DELTAMODE: Internal angle of grid-forming source, phase B|PA_HIDDEN|
|**V_angle_C**||double|DELTAMODE: Internal angle of grid-forming source, phase C|PA_HIDDEN|
|**phaseA_I_Forming**|A|complex|AC current on A phase in three-phase system, only for grid_forming|PUBLIC|
|**phaseB_I_Forming**|A|complex|AC current on B phase in three-phase system, only for grid_forming|PUBLIC|
|**phaseC_I_Forming**|A|complex|AC current on C phase in three-phase system, only for grid_forming|PUBLIC|
|**phaseA_I_Following**|A|complex|AC current on A phase in three-phase system, only for grid_following|PUBLIC|
|**phaseB_I_Following**|A|complex|AC current on B phase in three-phase system, only for grid_following|PUBLIC|
|**phaseC_I_Following**|A|complex|AC current on C phase in three-phase system, only for grid_following|PUBLIC|
|**pCircuit_V_Avg**||double|DELTAMODE: three-phase average value of terminal voltage|PA_HIDDEN|
|**P_Out**|VA|double|FOUR QUADRANT MODEL: Scheduled real power out in CONSTANT_PQ control mode|PUBLIC|
|**Q_Out**|VAr|double|FOUR QUADRANT MODEL: Schedule reactive power out in CONSTANT_PQ control mode|PUBLIC|
|**power_in**|W|double|LEGACY MODEL: No longer used|PUBLIC|
|**rated_power**|VA|double|FOUR QUADRANT MODEL: The rated power of the inverter|PUBLIC|
|**rated_battery_power**|W|double|FOUR QUADRANT MODEL: The rated power of battery when battery is attached|PUBLIC|
|**inverter_efficiency**||double|FOUR QUADRANT MODEL: The efficiency of the inverter|PUBLIC|
|**battery_soc**|pu|double|FOUR QUADRANT MODEL: The state of charge of an attached battery|PUBLIC|
|**soc_reserve**|pu|double|FOUR QUADRANT MODEL: The reserve state of charge of an attached battery for islanding cases|PUBLIC|
|**power_factor**|unit|double|FOUR QUADRANT MODEL: The power factor used for CONSTANT_PF control mode|PUBLIC|
|**islanded_state**||bool|FOUR QUADRANT MODEL: Boolean used to let control modes to act under island conditions|PUBLIC|
|**nominal_frequency**|Hz|double||PUBLIC|
|**Pref**||double|DELTAMODE: The real power reference.|PUBLIC|
|**Qref**||double|DELTAMODE: The reactive power reference.|PUBLIC|
|**kpd**||double|DELTAMODE: The d-axis integration gain for the current modulation PI controller.|PUBLIC|
|**kpq**||double|DELTAMODE: The q-axis integration gain for the current modulation PI controller.|PUBLIC|
|**kid**||double|DELTAMODE: The d-axis proportional gain for the current modulation PI controller.|PUBLIC|
|**kiq**||double|DELTAMODE: The q-axis proportional gain for the current modulation PI controller.|PUBLIC|
|**kdd**||double|DELTAMODE: The d-axis differentiator gain for the current modulation PID controller|PUBLIC|
|**kdq**||double|DELTAMODE: The q-axis differentiator gain for the current modulation PID controller|PUBLIC|
|**epA**||double|DELTAMODE: The real current error for phase A or triplex phase.|PUBLIC|
|**epB**||double|DELTAMODE: The real current error for phase B.|PUBLIC|
|**epC**||double|DELTAMODE: The real current error for phase C.|PUBLIC|
|**eqA**||double|DELTAMODE: The reactive current error for phase A or triplex phase.|PUBLIC|
|**eqB**||double|DELTAMODE: The reactive current error for phase B.|PUBLIC|
|**eqC**||double|DELTAMODE: The reactive current error for phase C.|PUBLIC|
|**delta_epA**||double|DELTAMODE: The change in real current error for phase A or triplex phase.|PUBLIC|
|**delta_epB**||double|DELTAMODE: The change in real current error for phase B.|PUBLIC|
|**delta_epC**||double|DELTAMODE: The change in real current error for phase C.|PUBLIC|
|**delta_eqA**||double|DELTAMODE: The change in reactive current error for phase A or triplex phase.|PUBLIC|
|**delta_eqB**||double|DELTAMODE: The change in reactive current error for phase B.|PUBLIC|
|**delta_eqC**||double|DELTAMODE: The change in reactive current error for phase C.|PUBLIC|
|**mdA**||double|DELTAMODE: The d-axis current modulation for phase A or triplex phase.|PUBLIC|
|**mdB**||double|DELTAMODE: The d-axis current modulation for phase B.|PUBLIC|
|**mdC**||double|DELTAMODE: The d-axis current modulation for phase C.|PUBLIC|
|**mqA**||double|DELTAMODE: The q-axis current modulation for phase A or triplex phase.|PUBLIC|
|**mqB**||double|DELTAMODE: The q-axis current modulation for phase B.|PUBLIC|
|**mqC**||double|DELTAMODE: The q-axis current modulation for phase C.|PUBLIC|
|**delta_mdA**||double|DELTAMODE: The change in d-axis current modulation for phase A or triplex phase.|PUBLIC|
|**delta_mdB**||double|DELTAMODE: The change in d-axis current modulation for phase B.|PUBLIC|
|**delta_mdC**||double|DELTAMODE: The change in d-axis current modulation for phase C.|PUBLIC|
|**delta_mqA**||double|DELTAMODE: The change in q-axis current modulation for phase A or triplex phase.|PUBLIC|
|**delta_mqB**||double|DELTAMODE: The change in q-axis current modulation for phase B.|PUBLIC|
|**delta_mqC**||double|DELTAMODE: The change in q-axis current modulation for phase C.|PUBLIC|
|**IdqA**||complex|DELTAMODE: The dq-axis current for phase A or triplex phase.|PUBLIC|
|**IdqB**||complex|DELTAMODE: The dq-axis current for phase B.|PUBLIC|
|**IdqC**||complex|DELTAMODE: The dq-axis current for phase C.|PUBLIC|
|**Tfreq_delay**||double|DELTAMODE: The time constant for delayed frequency seen by the inverter|PUBLIC|
|**inverter_droop_fp**||bool|DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not|PUBLIC|
|**R_fp**||double|DELTAMODE: The droop parameter of the f/p droop|PUBLIC|
|**kppmax**||double|DELTAMODE: The proportional gain of Pmax controller|PUBLIC|
|**kipmax**||double|DELTAMODE: The integral gain of Pmax controller|PUBLIC|
|**Pmax**||double|DELTAMODE: power limit of grid forming inverter|PUBLIC|
|**Pmin**||double|DELTAMODE: power limit of grid forming inverter|PUBLIC|
|**Pmax_Low_Limit**||double|DELTAMODE: output limit of Pmax controller|PUBLIC|
|**Tvol_delay**||double|DELTAMODE: The time constant for delayed voltage seen by the inverter|PUBLIC|
|**inverter_droop_vq**||bool|DELTAMODE: Boolean used to indicate whether inverter q/v droop is included or not|PUBLIC|
|**R_vq**||double|DELTAMODE: The droop parameter of the v/q droop|PUBLIC|
|**Tp_delay**||double|DELTAMODE: The time constant for delayed real power seen by the VSI droop controller|PUBLIC|
|**Tq_delay**||double|DELTAMODE: The time constant for delayed reactive power seen by the VSI droop controller|PUBLIC|
|**VSI_Rfilter**|pu|complex|VSI filter resistance (p.u.)|PUBLIC|
|**VSI_Xfilter**|pu|complex|VSI filter inductance (p.u.)|PUBLIC|
|**VSI_mode**||enumeration|VSI MODEL: Selects VSI mode for either isochronous or droop one [VSI_ISOCHRONOUS, VSI_DROOP]|PUBLIC|
|**VSI_freq**||double|VSI frequency|PUBLIC|
|**ki_Vterminal**||double|DELTAMODE: The integrator gain for the VSI terminal voltage modulation|PUBLIC|
|**kp_Vterminal**||double|DELTAMODE: The proportional gain for the VSI terminal voltage modulation|PUBLIC|
|**V_set_droop**||double|DELTAMODE: The voltage setpoint of droop control|PUBLIC|
|**V_set0**||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|PUBLIC|
|**V_set1**||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|PUBLIC|
|**V_set2**||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|PUBLIC|
|**enable_ramp_rates_real**||bool|DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not|PUBLIC|
|**max_ramp_up_real**|W/s|double|DELTAMODE: The real power ramp up rate limit|PUBLIC|
|**max_ramp_down_real**|W/s|double|DELTAMODE: The real power ramp down rate limit|PUBLIC|
|**enable_ramp_rates_reactive**||bool|DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not|PUBLIC|
|**max_ramp_up_reactive**|VAr/s|double|DELTAMODE: The reactive power ramp up rate limit|PUBLIC|
|**max_ramp_down_reactive**|VAr/s|double|DELTAMODE: The reactive power ramp down rate limit|PUBLIC|
|**dynamic_model_mode**||enumeration|DELTAMODE: Underlying model to use for deltamode control [PID, PI]|PUBLIC|
|**enable_1547_checks**||bool|DELTAMODE: Enable IEEE 1547-2003 disconnect checking|PUBLIC|
|**reconnect_time**|s|double|DELTAMODE: Time delay after IEEE 1547-2003 violation clears before resuming generation|PUBLIC|
|**inverter_1547_status**||bool|DELTAMODE: Indicator if the inverter is curtailed due to a 1547 violation or not|PUBLIC|
|**IEEE_1547_version**||enumeration|DELTAMODE: Version of IEEE 1547 to use to populate defaults [NONE, IEEE1547, IEEE1547A, IEEE1547_2003, IEEE1547A_2014]|PUBLIC|
|**over_freq_high_cutout**|Hz|double|DELTAMODE: OF2 set point for IEEE 1547a|PUBLIC|
|**over_freq_high_disconnect_time**|s|double|DELTAMODE: OF2 clearing time for IEEE1547a|PUBLIC|
|**over_freq_low_cutout**|Hz|double|DELTAMODE: OF1 set point for IEEE 1547a|PUBLIC|
|**over_freq_low_disconnect_time**|s|double|DELTAMODE: OF1 clearing time for IEEE 1547a|PUBLIC|
|**under_freq_high_cutout**|Hz|double|DELTAMODE: UF2 set point for IEEE 1547a|PUBLIC|
|**under_freq_high_disconnect_time**|s|double|DELTAMODE: UF2 clearing time for IEEE1547a|PUBLIC|
|**under_freq_low_cutout**|Hz|double|DELTAMODE: UF1 set point for IEEE 1547a|PUBLIC|
|**under_freq_low_disconnect_time**|s|double|DELTAMODE: UF1 clearing time for IEEE 1547a|PUBLIC|
|**under_voltage_low_cutout**|pu|double|Lowest voltage threshold for undervoltage|PUBLIC|
|**under_voltage_middle_cutout**|pu|double|Middle-lowest voltage threshold for undervoltage|PUBLIC|
|**under_voltage_high_cutout**|pu|double|High value of low voltage threshold for undervoltage|PUBLIC|
|**over_voltage_low_cutout**|pu|double|Lowest voltage value for overvoltage|PUBLIC|
|**over_voltage_high_cutout**|pu|double|High voltage value for overvoltage|PUBLIC|
|**under_voltage_low_disconnect_time**|s|double|Lowest voltage clearing time for undervoltage|PUBLIC|
|**under_voltage_middle_disconnect_time**|s|double|Middle-lowest voltage clearing time for undervoltage|PUBLIC|
|**under_voltage_high_disconnect_time**|s|double|Highest voltage clearing time for undervoltage|PUBLIC|
|**over_voltage_low_disconnect_time**|s|double|Lowest voltage clearing time for overvoltage|PUBLIC|
|**over_voltage_high_disconnect_time**|s|double|Highest voltage clearing time for overvoltage|PUBLIC|
|**IEEE_1547_trip_method**||enumeration|DELTAMODE: Reason for IEEE 1547 disconnect - which threshold was hit [NONE, OVER_FREQUENCY_HIGH, OVER_FREQUENCY_LOW, UNDER_FREQUENCY_HIGH, UNDER_FREQUENCY_LOW, UNDER_VOLTAGE_LOW, UNDER_VOLTAGE_MID, UNDER_VOLTAGE_HIGH, OVER_VOLTAGE_LOW, OVER_VOLTAGE_HIGH]|PUBLIC|
|**phases**||set|The phases the inverter is attached to [A, B, C, N, S]|PUBLIC|
|**use_multipoint_efficiency**||bool|FOUR QUADRANT MODEL: boolean to used the multipoint efficiency curve for the inverter when solar is attached|PUBLIC|
|**inverter_manufacturer**||enumeration|MULTIPOINT EFFICIENCY MODEL: the manufacturer of the inverter to setup up pre-existing efficiency curves [NONE, FRONIUS, SMA, XANTREX]|PUBLIC|
|**maximum_dc_power**||double|MULTIPOINT EFFICIENCY MODEL: the maximum dc power point for the efficiency curve|PUBLIC|
|**maximum_dc_voltage**||double|MULTIPOINT EFFICIENCY MODEL: the maximum dc voltage point for the efficiency curve|PUBLIC|
|**minimum_dc_power**||double|MULTIPOINT EFFICIENCY MODEL: the minimum dc power point for the efficiency curve|PUBLIC|
|**c_0**||double|MULTIPOINT EFFICIENCY MODEL: the first coefficient in the efficiency curve|PUBLIC|
|**c_1**||double|MULTIPOINT EFFICIENCY MODEL: the second coefficient in the efficiency curve|PUBLIC|
|**c_2**||double|MULTIPOINT EFFICIENCY MODEL: the third coefficient in the efficiency curve|PUBLIC|
|**c_3**||double|MULTIPOINT EFFICIENCY MODEL: the fourth coefficient in the efficiency curve|PUBLIC|
|**sense_object**||object|FOUR QUADRANT MODEL: name of the object the inverter is trying to mitigate the load on (node/link) in LOAD_FOLLOWING|PUBLIC|
|**max_charge_rate**|W|double|FOUR QUADRANT MODEL: maximum rate the battery can be charged in LOAD_FOLLOWING|PUBLIC|
|**max_discharge_rate**|W|double|FOUR QUADRANT MODEL: maximum rate the battery can be discharged in LOAD_FOLLOWING|PUBLIC|
|**charge_on_threshold**|W|double|FOUR QUADRANT MODEL: power level at which the inverter should try charging the battery in LOAD_FOLLOWING|PUBLIC|
|**charge_off_threshold**|W|double|FOUR QUADRANT MODEL: power level at which the inverter should cease charging the battery in LOAD_FOLLOWING|PUBLIC|
|**discharge_on_threshold**|W|double|FOUR QUADRANT MODEL: power level at which the inverter should try discharging the battery in LOAD_FOLLOWING|PUBLIC|
|**discharge_off_threshold**|W|double|FOUR QUADRANT MODEL: power level at which the inverter should cease discharging the battery in LOAD_FOLLOWING|PUBLIC|
|**excess_input_power**|W|double|FOUR QUADRANT MODEL: Excess power at the input of the inverter that is otherwise just lost, or could be shunted to a battery|PUBLIC|
|**charge_lockout_time**|s|double|FOUR QUADRANT MODEL: Lockout time when a charging operation occurs before another LOAD_FOLLOWING dispatch operation can occur|PUBLIC|
|**discharge_lockout_time**|s|double|FOUR QUADRANT MODEL: Lockout time when a discharging operation occurs before another LOAD_FOLLOWING dispatch operation can occur|PUBLIC|
|**pf_reg_activate**||double|FOUR QUADRANT MODEL: Lowest acceptable power-factor level below which power-factor regulation will activate.|PUBLIC|
|**pf_reg_deactivate**||double|FOUR QUADRANT MODEL: Lowest acceptable power-factor above which no power-factor regulation is needed.|PUBLIC|
|**pf_target**||double|FOUR QUADRANT MODEL: Desired power-factor to maintain (signed) positive is inductive|PUBLIC|
|**pf_reg_high**||double|FOUR QUADRANT MODEL: Upper limit for power-factor - if exceeds, go full reverse reactive|PUBLIC|
|**pf_reg_low**||double|FOUR QUADRANT MODEL: Lower limit for power-factor - if exceeds, stop regulating - pf_target_var is below this|PUBLIC|
|**pf_reg_activate_lockout_time**|s|double|FOUR QUADRANT MODEL: Mandatory pause between the deactivation of power-factor regulation and it reactivation|PUBLIC|
|**disable_volt_var_if_no_input_power**||bool||PUBLIC|
|**delay_time**|s|double||PUBLIC|
|**max_var_slew_rate**|VAr/s|double||PUBLIC|
|**max_pwr_slew_rate**|W/s|double||PUBLIC|
|**volt_var_sched**||char1024||PUBLIC|
|**freq_pwr_sched**||char1024||PUBLIC|
|**charge_threshold**|W|double|FOUR QUADRANT MODEL: Level at which all inverters in the group will begin charging attached batteries. Regulated minimum load level.|PUBLIC|
|**discharge_threshold**|W|double|FOUR QUADRANT MODEL: Level at which all inverters in the group will begin discharging attached batteries. Regulated maximum load level.|PUBLIC|
|**group_max_charge_rate**|W|double|FOUR QUADRANT MODEL: Sum of the charge rates of the batteries involved in the group load-following.|PUBLIC|
|**group_max_discharge_rate**|W|double|FOUR QUADRANT MODEL: Sum of the discharge rates of the batteries involved in the group load-following.|PUBLIC|
|**group_rated_power**|W|double|FOUR QUADRANT MODEL: Sum of the inverter power ratings of the inverters involved in the group power-factor regulation.|PUBLIC|
|**V_base**|V|double|FOUR QUADRANT MODEL: The base voltage on the grid side of the inverter. Used in VOLT_VAR control mode.|PUBLIC|
|**V1**|pu|double|FOUR QUADRANT MODEL: voltage point 1 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**Q1**|pu|double|FOUR QUADRANT MODEL: VAR point 1 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**V2**|pu|double|FOUR QUADRANT MODEL: voltage point 2 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**Q2**|pu|double|FOUR QUADRANT MODEL: VAR point 2 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**V3**|pu|double|FOUR QUADRANT MODEL: voltage point 3 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**Q3**|pu|double|FOUR QUADRANT MODEL: VAR point 3 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**V4**|pu|double|FOUR QUADRANT MODEL: voltage point 4 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**Q4**|pu|double|FOUR QUADRANT MODEL: VAR point 4 in volt/var curve. Used in VOLT_VAR control mode.|PUBLIC|
|**volt_var_control_lockout**|s|double|FOUR QUADRANT QUADRANT MODEL: the lockout time between volt/var actions.|PUBLIC|
|**number_of_phases_out**||int32|Exposed variable - hidden for solar|PA_HIDDEN|
|**efficiency_value**||double|Exposed variable - hidden for solar|PA_HIDDEN|
|**VW_V1**|pu|double|FOUR QUADRANT MODEL: Voltage at which power limiting begins (e.g. 1.0583). Used in VOLT_WATT control mode.|PUBLIC|
|**VW_V2**|pu|double|FOUR QUADRANT MODEL: Voltage at which power limiting ends. (e.g. 1.1000). Used in VOLT_WATT control mode.|PUBLIC|
|**VW_P1**|pu|double|FOUR QUADRANT MODEL: Power limit at VW_P1 (e.g. 1). Used in VOLT_WATT control mode.|PUBLIC|
|**VW_P2**|pu|double|FOUR QUADRANT MODEL: Power limit at VW_P2 (e.g. 0). Used in VOLT_WATT control mode.|PUBLIC|
|**WT_is_connected**||bool|Internal flag that indicates a wind turbine child is connected|PUBLIC|


---

*Generated from inverter.cpp source code*
