## Series Compensator

!!! warning
    This page was automatically generated and requires review.

### Series Compensator Parameters

#### Properties

**series_compensator** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| vset_A | double | pu | ✓ | ✓ | ⚠️ Voltage magnitude reference for phase A |
| vset_B | double | pu | ✓ | ✓ | ⚠️ Voltage magnitude reference for phase B |
| vset_C | double | pu | ✓ | ✓ | ⚠️ Voltage magnitude reference for phase C |
| vset_A_0 | double | pu | ✓ |  | ⚠️ Voltage magnitude set point for phase A, changed by the player |
| vset_B_0 | double | pu | ✓ |  | ⚠️ Voltage magnitude set point for phase B, changed by the player |
| vset_C_0 | double | pu | ✓ |  | ⚠️ Voltage magnitude set point for phase C, changed by the player |
| vset_1 | double | pu | ✓ | ✓ | ⚠️ Voltage magnitude reference for phase 1 of a triplex system |
| vset_2 | double | pu | ✓ | ✓ | ⚠️ Voltage magnitude reference for phase 2 of a tryplex system |
| vset_1_0 | double | pu | ✓ |  | ⚠️ Voltage magnitude reference for phase 1 of a triplex system |
| vset_2_0 | double | pu | ✓ |  | ⚠️ Voltage magnitude reference for phase 2 of a tryplex system |
| frequency_regulation | bool | N/A | ✓ |  | ⚠️ DELTAMODE: Boolean value indicating whether the frequency regulation of the series compensator is enabled or not |
| frequency_open_loop_control | bool | N/A | ✓ |  | ⚠️ DELTAMODE: Boolean value indicating whether the frequency open loop control of the series compensator is enabled or not |
| t_delay | double | N/A | ✓ |  | ⚠️ the controller will wait for t_delay to take actions |
| t_hold | double | N/A | ✓ |  | ⚠️ Once the controller changes the voltage set point, it will stay there for t_hold time |
| recover_rate | double | N/A | ✓ |  | ⚠️ The rate that the voltage goes back to nominal, unit: pu/s |
| frequency_low | double | N/A | ✓ |  | ⚠️ The low frequency that activates the controller |
| frequency_high | double | N/A | ✓ |  | ⚠️ The high frequency that activates the controller |
| V_error | double | N/A | ✓ |  | ⚠️ Make sure the voltage can go back to nominal |
| voltage_update_tolerance | double | pu | ✓ |  | ⚠️ Largest absolute between vset_X and measured voltage that won&#x27;t force a reiteration |
| n_max_ext_A | double | N/A | ✓ |  | ⚠️ maximum Turn ratio for phase A |
| n_max_ext_B | double | N/A | ✓ |  | ⚠️ maximum Turn ratio for phase B |
| n_max_ext_C | double | N/A | ✓ |  | ⚠️ maximum Turn ratio for phase C |
| n_min_ext_A | double | N/A | ✓ |  | ⚠️ minimum Turn ratio for phase A |
| n_min_ext_B | double | N/A | ✓ |  | ⚠️ minimum Turn ratio for phase B |
| n_min_ext_C | double | N/A | ✓ |  | ⚠️ minimum Turn ratio for phase C |
| n_max_ext_1 | double | N/A | ✓ |  | ⚠️ maximum Turn ratio for phase 1 (triplex) |
| n_max_ext_2 | double | N/A | ✓ |  | ⚠️ maximum Turn ratio for phase 2 (triplex) |
| n_min_ext_1 | double | N/A | ✓ |  | ⚠️ minimum Turn ratio for phase 1 (triplex) |
| n_min_ext_2 | double | N/A | ✓ |  | ⚠️ minimum Turn ratio for phase 2 (triplex) |
| kp | double | N/A | ✓ |  | ⚠️ proportional gain |
| ki | double | N/A | ✓ |  | ⚠️ integrator gain |
| kpf | double | N/A | ✓ |  | ⚠️ proportional gain of frequency regulation |
| f_db_max | double | N/A | ✓ |  | ⚠️ frequency dead band max |
| f_db_min | double | N/A | ✓ |  | ⚠️ frequency dead band max |
| delta_Vmax | double | N/A | ✓ |  | ⚠️ upper limit of the frequency regulation output |
| delta_Vmin | double | N/A | ✓ |  | ⚠️ lower limit of the frequency regulation output |
| delta_V | double | N/A |  | ✓ | ⚠️ frequency regulation output |
| V_bypass_max_pu | double | N/A | ✓ |  | ⚠️ the upper limit voltage to bypass compensator |
| V_bypass_min_pu | double | N/A | ✓ |  | ⚠️ the lower limit voltage to bypass compensator |
| phase_A_state | enumeration | N/A | ✓ | ✓ | ⚠️ Defines if phase A is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| phase_B_state | enumeration | N/A | ✓ | ✓ | ⚠️ Defines if phase B is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| phase_C_state | enumeration | N/A | ✓ | ✓ | ⚠️ Defines if phase C is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| phase_1_state | enumeration | N/A | ✓ | ✓ | ⚠️ Defines if phase 1 is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| phase_2_state | enumeration | N/A | ✓ | ✓ | ⚠️ Defines if phase 2 is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| series_compensator_resistance | double | Ohm | ✓ |  | ⚠️ Baseline resistance for the series compensator device - needed for NR |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| turns_ratio_A | double | N/A |  | ✓ | ⚠️ Debug variable - Turns ratio for phase A series compensator equivalent |
| turns_ratio_B | double | N/A |  | ✓ | ⚠️ Debug variable - Turns ratio for phase B series compensator equivalent |
| turns_ratio_C | double | N/A |  | ✓ | ⚠️ Debug variable - Turns ratio for phase C series compensator equivalent |
| turns_ratio_1 | double | N/A |  | ✓ | ⚠️ Debug variable - Turns ratio for phase 1 (triplex) series compensator equivalent |
| turns_ratio_2 | double | N/A |  | ✓ | ⚠️ Debug variable - Turns ratio for phase 2 (triplex) series compensator equivalent |
