# GridLAB-D™ QSTS Inverter Model - DELTAMODE (Legacy)

<mark style="background-color: lightgreen;">Undocumented Parameters and Variables as they are only used in deltamode and not relevant for this model. Should we even include them here? Maybe at the end of the file or in a separate section?</mark>

|Published Name|Unit|Type|Description|
|---|---|---|---|
|```inverter_convergence_criterion```||double|The maximum change in error threshold for exitting deltamode.|
|```current_convergence```|A|double|Convergence criterion for current changes on first timestep - basically initialization of system|
|```e_source_A```||double|DELTAMODE: Internal voltage of grid-forming source, phase A|PA_HIDDEN|
|```e_source_B```||double|DELTAMODE: Internal voltage of grid-forming source, phase B|PA_HIDDEN|
|```e_source_C```||double|DELTAMODE: Internal voltage of grid-forming source, phase C|PA_HIDDEN|
|```V_angle_A```||double|DELTAMODE: Internal angle of grid-forming source, phase A|PA_HIDDEN|
|```V_angle_B```||double|DELTAMODE: Internal angle of grid-forming source, phase B|PA_HIDDEN|
|```V_angle_C```||double|DELTAMODE: Internal angle of grid-forming source, phase C|PA_HIDDEN|
|```phaseA_I_Forming```|A|complex|AC current on A phase in three-phase system, only for grid_forming|
|```phaseB_I_Forming```|A|complex|AC current on B phase in three-phase system, only for grid_forming|
|```phaseC_I_Forming```|A|complex|AC current on C phase in three-phase system, only for grid_forming|
|```phaseA_I_Following```|A|complex|AC current on A phase in three-phase system, only for grid_following|
|```phaseB_I_Following```|A|complex|AC current on B phase in three-phase system, only for grid_following|
|```phaseC_I_Following```|A|complex|AC current on C phase in three-phase system, only for grid_following|
|```pCircuit_V_Avg```||double|DELTAMODE: three-phase average value of terminal voltage|PA_HIDDEN|
|```power_in```|W|double|LEGACY MODEL: No longer used|
|```nominal_frequency```|Hz|double||
|```Pref```||double|DELTAMODE: The real power reference.|
|```Qref```||double|DELTAMODE: The reactive power reference.|
|```kpd```||double|DELTAMODE: The d-axis integration gain for the current modulation PI controller.|
|```kpq```||double|DELTAMODE: The q-axis integration gain for the current modulation PI controller.|
|```kid```||double|DELTAMODE: The d-axis proportional gain for the current modulation PI controller.|
|```kiq```||double|DELTAMODE: The q-axis proportional gain for the current modulation PI controller.|
|```kdd```||double|DELTAMODE: The d-axis differentiator gain for the current modulation PID controller|
|```kdq```||double|DELTAMODE: The q-axis differentiator gain for the current modulation PID controller|
|```epA```||double|DELTAMODE: The real current error for phase A or triplex phase.|
|```epB```||double|DELTAMODE: The real current error for phase B.|
|```epC```||double|DELTAMODE: The real current error for phase C.|
|```eqA```||double|DELTAMODE: The reactive current error for phase A or triplex phase.|
|```eqB```||double|DELTAMODE: The reactive current error for phase B.|
|```eqC```||double|DELTAMODE: The reactive current error for phase C.|
|```delta_epA```||double|DELTAMODE: The change in real current error for phase A or triplex phase.|
|```delta_epB```||double|DELTAMODE: The change in real current error for phase B.|
|```delta_epC```||double|DELTAMODE: The change in real current error for phase C.|
|```delta_eqA```||double|DELTAMODE: The change in reactive current error for phase A or triplex phase.|
|```delta_eqB```||double|DELTAMODE: The change in reactive current error for phase B.|
|```delta_eqC```||double|DELTAMODE: The change in reactive current error for phase C.|
|```mdA```||double|DELTAMODE: The d-axis current modulation for phase A or triplex phase.|
|```mdB```||double|DELTAMODE: The d-axis current modulation for phase B.|
|```mdC```||double|DELTAMODE: The d-axis current modulation for phase C.|
|```mqA```||double|DELTAMODE: The q-axis current modulation for phase A or triplex phase.|
|```mqB```||double|DELTAMODE: The q-axis current modulation for phase B.|
|```mqC```||double|DELTAMODE: The q-axis current modulation for phase C.|
|```delta_mdA```||double|DELTAMODE: The change in d-axis current modulation for phase A or triplex phase.|
|```delta_mdB```||double|DELTAMODE: The change in d-axis current modulation for phase B.|
|```delta_mdC```||double|DELTAMODE: The change in d-axis current modulation for phase C.|
|```delta_mqA```||double|DELTAMODE: The change in q-axis current modulation for phase A or triplex phase.|
|```delta_mqB```||double|DELTAMODE: The change in q-axis current modulation for phase B.|
|```delta_mqC```||double|DELTAMODE: The change in q-axis current modulation for phase C.|
|```IdqA```||complex|DELTAMODE: The dq-axis current for phase A or triplex phase.|
|```IdqB```||complex|DELTAMODE: The dq-axis current for phase B.|
|```IdqC```||complex|DELTAMODE: The dq-axis current for phase C.|
|```Tfreq_delay```||double|DELTAMODE: The time constant for delayed frequency seen by the inverter|
|```inverter_droop_fp```||bool|DELTAMODE: Boolean used to indicate whether inverter f/p droop is included or not|
|```R_fp```||double|DELTAMODE: The droop parameter of the f/p droop|
|```kppmax```||double|DELTAMODE: The proportional gain of Pmax controller|
|```kipmax```||double|DELTAMODE: The integral gain of Pmax controller|
|```Pmax```||double|DELTAMODE: power limit of grid forming inverter|
|```Pmin```||double|DELTAMODE: power limit of grid forming inverter|
|```Pmax_Low_Limit```||double|DELTAMODE: output limit of Pmax controller|
|```Tvol_delay```||double|DELTAMODE: The time constant for delayed voltage seen by the inverter|
|```inverter_droop_vq```||bool|DELTAMODE: Boolean used to indicate whether inverter q/v droop is included or not|
|```R_vq```||double|DELTAMODE: The droop parameter of the v/q droop|
|```Tp_delay```||double|DELTAMODE: The time constant for delayed real power seen by the VSI droop controller|
|```Tq_delay```||double|DELTAMODE: The time constant for delayed reactive power seen by the VSI droop controller|
|```VSI_Rfilter```|pu|complex|VSI filter resistance (p.u.)|
|```VSI_Xfilter```|pu|complex|VSI filter inductance (p.u.)|
|```VSI_mode```||enumeration|VSI MODEL: Selects VSI mode for either isochronous or droop one [VSI_ISOCHRONOUS, VSI_DROOP]|
|```VSI_freq```||double|VSI frequency|
|```ki_Vterminal```||double|DELTAMODE: The integrator gain for the VSI terminal voltage modulation|
|```kp_Vterminal```||double|DELTAMODE: The proportional gain for the VSI terminal voltage modulation|
|```V_set_droop```||double|DELTAMODE: The voltage setpoint of droop control|
|```V_set0```||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|
|```V_set1```||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|
|```V_set2```||double|DELTAMODE: The voltage setpoint of grid-following Q-V droop control|
|```enable_ramp_rates_real```||bool|DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not|
|```max_ramp_up_real```|W/s|double|DELTAMODE: The real power ramp up rate limit|
|```max_ramp_down_real```|W/s|double|DELTAMODE: The real power ramp down rate limit|
|```enable_ramp_rates_reactive```||bool|DELTAMODE: Boolean used to indicate whether inverter ramp rate is enforced or not|
|```max_ramp_up_reactive```|VAr/s|double|DELTAMODE: The reactive power ramp up rate limit|
|```max_ramp_down_reactive```|VAr/s|double|DELTAMODE: The reactive power ramp down rate limit|
|```dynamic_model_mode```||enumeration|DELTAMODE: Underlying model to use for deltamode control [PID, PI]|
|```enable_1547_checks```||bool|DELTAMODE: Enable IEEE 1547-2003 disconnect checking|
|```reconnect_time```|s|double|DELTAMODE: Time delay after IEEE 1547-2003 violation clears before resuming generation|
|```inverter_1547_status```||bool|DELTAMODE: Indicator if the inverter is curtailed due to a 1547 violation or not|
|```IEEE_1547_version```||enumeration|DELTAMODE: Version of IEEE 1547 to use to populate defaults [NONE, IEEE1547, IEEE1547A, IEEE1547_2003, IEEE1547A_2014]|
|```over_freq_high_cutout```|Hz|double|DELTAMODE: OF2 set point for IEEE 1547a|
|```over_freq_high_disconnect_time```|s|double|DELTAMODE: OF2 clearing time for IEEE1547a|
|```over_freq_low_cutout```|Hz|double|DELTAMODE: OF1 set point for IEEE 1547a|
|```over_freq_low_disconnect_time```|s|double|DELTAMODE: OF1 clearing time for IEEE 1547a|
|```under_freq_high_cutout```|Hz|double|DELTAMODE: UF2 set point for IEEE 1547a|
|```under_freq_high_disconnect_time```|s|double|DELTAMODE: UF2 clearing time for IEEE1547a|
|```under_freq_low_cutout```|Hz|double|DELTAMODE: UF1 set point for IEEE 1547a|
|```under_freq_low_disconnect_time```|s|double|DELTAMODE: UF1 clearing time for IEEE 1547a|
|```under_voltage_low_cutout```|pu|double|Lowest voltage threshold for undervoltage|
|```under_voltage_middle_cutout```|pu|double|Middle-lowest voltage threshold for undervoltage|
|```under_voltage_high_cutout```|pu|double|High value of low voltage threshold for undervoltage|
|```over_voltage_low_cutout```|pu|double|Lowest voltage value for overvoltage|
|```over_voltage_high_cutout```|pu|double|High voltage value for overvoltage|
|```under_voltage_low_disconnect_time```|s|double|Lowest voltage clearing time for undervoltage|
|```under_voltage_middle_disconnect_time```|s|double|Middle-lowest voltage clearing time for undervoltage|
|```under_voltage_high_disconnect_time```|s|double|Highest voltage clearing time for undervoltage|
|```over_voltage_low_disconnect_time```|s|double|Lowest voltage clearing time for overvoltage|
|```over_voltage_high_disconnect_time```|s|double|Highest voltage clearing time for overvoltage|
|```IEEE_1547_trip_method```||enumeration|DELTAMODE: Reason for IEEE 1547 disconnect - which threshold was hit [NONE, OVER_FREQUENCY_HIGH, OVER_FREQUENCY_LOW, UNDER_FREQUENCY_HIGH, UNDER_FREQUENCY_LOW, UNDER_VOLTAGE_LOW, UNDER_VOLTAGE_MID, UNDER_VOLTAGE_HIGH, OVER_VOLTAGE_LOW, OVER_VOLTAGE_HIGH]|
|```excess_input_power```|W|double|FOUR QUADRANT MODEL: Excess power at the input of the inverter that is otherwise just lost, or could be shunted to a battery|
|```number_of_phases_out```||int32|Exposed variable - hidden for solar|PA_HIDDEN|
|```efficiency_value```||double|Exposed variable - hidden for solar|PA_HIDDEN|
|```WT_is_connected```||bool|Internal flag that indicates a wind turbine child is connected|
