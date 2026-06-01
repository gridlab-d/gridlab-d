## Series Compensator



### Series Compensator Parameters

#### Properties

**series_compensator** objects are derived from **[link](04-link.md)** objects, so any parameters of the **[link](04-link.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: series_compensator table 1 { #tbl:47-series-compensator-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **vset_A** | double | pu | IO |  Voltage magnitude reference for phase A |
| **vset_B** | double | pu | IO |  Voltage magnitude reference for phase B |
| **vset_C** | double | pu | IO |  Voltage magnitude reference for phase C |
| **vset_A_0** | double | pu | I |  Voltage magnitude set point for phase A, changed by the player |
| **vset_B_0** | double | pu | I |  Voltage magnitude set point for phase B, changed by the player |
| **vset_C_0** | double | pu | I |  Voltage magnitude set point for phase C, changed by the player |
| **vset_1** | double | pu | IO |  Voltage magnitude reference for phase 1 of a triplex system |
| **vset_2** | double | pu | IO |  Voltage magnitude reference for phase 2 of a tryplex system |
| **vset_1_0** | double | pu | I |  Voltage magnitude reference for phase 1 of a triplex system |
| **vset_2_0** | double | pu | I |  Voltage magnitude reference for phase 2 of a tryplex system |
| **frequency_regulation** | bool | N/A | I |  DELTAMODE: Boolean value indicating whether the frequency regulation of the series compensator is enabled or not |
| **frequency_open_loop_control** | bool | N/A | I |  DELTAMODE: Boolean value indicating whether the frequency open loop control of the series compensator is enabled or not |
| **t_delay** | double | N/A | I |  The controller will wait for t_delay to take actions |
| **t_hold** | double | N/A | I |  Once the controller changes the voltage set point, it will stay there for t_hold time |
| **recover_rate** | double | N/A | I |  The rate that the voltage goes back to nominal, unit: pu/s |
| **frequency_low** | double | N/A | I |  The low frequency that activates the controller |
| **frequency_high** | double | N/A | I |  The high frequency that activates the controller |
| **V_error** | double | N/A | I |  Make sure the voltage can go back to nominal |
| **voltage_update_tolerance** | double | pu | I |  Largest absolute between vset_X and measured voltage that won't force a reiteration |
| **n_max_ext_A** | double | N/A | I |  Maximum Turn ratio for phase A |
| **n_max_ext_B** | double | N/A | I |  Maximum Turn ratio for phase B |
| **n_max_ext_C** | double | N/A | I |  Maximum Turn ratio for phase C |
| **n_min_ext_A** | double | N/A | I |  Minimum Turn ratio for phase A |
| **n_min_ext_B** | double | N/A | I |  Minimum Turn ratio for phase B |
| **n_min_ext_C** | double | N/A | I |  Minimum Turn ratio for phase C |
| **n_max_ext_1** | double | N/A | I |  Maximum Turn ratio for phase 1 (triplex) |
| **n_max_ext_2** | double | N/A | I |  Maximum Turn ratio for phase 2 (triplex) |
| **n_min_ext_1** | double | N/A | I |  Minimum Turn ratio for phase 1 (triplex) |
| **n_min_ext_2** | double | N/A | I |  Minimum Turn ratio for phase 2 (triplex) |
| **kp** | double | N/A | I |  Proportional gain |
| **ki** | double | N/A | I |  Integrator gain |
| **kpf** | double | N/A | I |  Proportional gain of frequency regulation |
| **f_db_max** | double | N/A | I |  Frequency dead band max |
| **f_db_min** | double | N/A | I |  Frequency dead band min |
| **delta_Vmax** | double | N/A | I |  Upper limit of the frequency regulation output |
| **delta_Vmin** | double | N/A | I |  Lower limit of the frequency regulation output |
| **delta_V** | double | N/A | O |  Frequency regulation output |
| **V_bypass_max_pu** | double | N/A | I |  The upper limit voltage to bypass compensator |
| **V_bypass_min_pu** | double | N/A | I |  The lower limit voltage to bypass compensator |
| **phase_A_state** | enumeration | N/A | IO |  Defines if phase A is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| **phase_B_state** | enumeration | N/A | IO |  Defines if phase B is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| **phase_C_state** | enumeration | N/A | IO |  Defines if phase C is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| **phase_1_state** | enumeration | N/A | IO |  Defines if phase 1 is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| **phase_2_state** | enumeration | N/A | IO |  Defines if phase 2 is in bypass or not Valid values: `NORMAL`, `BYPASS`. |
| **series_compensator_resistance** | double | Ohm | I |  Baseline resistance for the series compensator device - needed for NR |

??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

Table: series_compensator table 2 { #tbl:47-series-compensator-2 }

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| turns_ratio_A | double | N/A | O | Debug variable - Turns ratio for phase A series compensator equivalent |
	| turns_ratio_B | double | N/A | O | Debug variable - Turns ratio for phase B series compensator equivalent |
	| turns_ratio_C | double | N/A | O | Debug variable - Turns ratio for phase C series compensator equivalent |
	| turns_ratio_1 | double | N/A | O | Debug variable - Turns ratio for phase 1 (triplex) series compensator equivalent |
	| turns_ratio_2 | double | N/A | O | Debug variable - Turns ratio for phase 2 (triplex) series compensator equivalent |
