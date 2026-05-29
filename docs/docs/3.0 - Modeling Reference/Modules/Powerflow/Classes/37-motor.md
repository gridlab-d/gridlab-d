## Motor

The **motor** object models a three-phase or single phase induction motor. For detailed technical information on motor modeling approaches, see the additional documentation: [Tech_DeltaSPIM](../Motor/Tech_DeltaSPIM.md) (single-phase dynamic phasor model), [Tech_DeltaTPIM](../Motor/Tech_DeltaTPIM.md) (three-phase dynamic phasor model), and [Tech_CompositionMotor](../Motor/Tech_CompositionMotor.md) (composite motor models for heat pumps and refrigerators).

### Motor Parameters

#### Properties

**motor** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).



##### Operational and Control Parameters

Table: 37-motor table 1 { #tbl:37-motor-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| capacitor_speed | double | % | I | Percentage speed of nominal when starting capacitor kicks in |
| time_step_divider | int32 | N/A | I | Divide time step by n - single- and three- phase models |
| mechanical_torque | double | pu | IO | Mechanical torque applied to the motor |
| torque_usage_method | enumeration | N/A | I | Approach for using Tmech on both types. Valid values: `DIRECT`, `SPEEDFOUR`. |
| iteration_count | int32 | N/A | I | Maximum number of iterations for steady state model |
| maximum_speed_error | double | N/A | I | Maximum speed error for transitioning modes |
| delta_mode_voltage_trigger | double | % | I | Percentage voltage of nominal when transient mode is triggered |
| delta_mode_rotor_speed_trigger | double | % | I | Percentage speed of nominal when transient mode is triggered |
| delta_mode_voltage_exit | double | % | I | Percentage voltage of nominal to exit transient mode |
| delta_mode_rotor_speed_exit | double | % | I | Percentage speed of nominal to exit transient mode |
| rotor_speed | double | rad/s | IO | Rotor speed |
| motor_status | enumeration | N/A | O | The current status of the motor. Valid values: `RUNNING`, `STALLED`, `TRIPPED`, `OFF`. |
| motor_status_number | int32 | N/A | O | The current status of the motor as an integer |
| desired_motor_state | enumeration | N/A | IO | Should the motor be on or off. Valid values: `ON`, `OFF`. |
| connected_house | object | N/A | I | House object to monitor the XXX property to determine if the motor is running |
| connected_house_assumed_mode | enumeration | N/A | I | Assumed operation mode of connected_house object. Valid values: `NONE`, `COOLING`, `HEATING`. |
| motor_operation_type | enumeration | N/A | I | Current operation type of the motor - transient mode related. Valid values: `SINGLE-PHASE`, `THREE-PHASE`. |
| triplex_connection_type | enumeration | N/A | I | Describes how the motor will connect to the triplex devices. Valid values: `TRIPLEX_1N`, `TRIPLEX_2N`, `TRIPLEX_12`. |



##### Core Motor Parameters

These are the fundamental electromagnetic and mechanical parameters of the motor.

Table: 37-motor table 2 { #tbl:37-motor-2 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| base_power | double | W | I | Base power |
| n | double | N/A | I | Ratio of stator auxiliary windings to stator main windings |
| Rr | double | ohm | I | Rotor resistance |
| Xm | double | ohm | I | Magnetizing reactance |
| Xr | double | ohm | I | Rotor reactance |
| H | double | s | I | Inertia constant |
| J | double | kg*m^2 | I | Moment of inertia |
| number_of_poles | double | N/A | I | Number of poles |
| To_prime | double | s | I | Rotor time constant |

#### Single-Phase Model Properties

These properties are specific to the single-phase motor model.

Table: 37-motor table 3 { #tbl:37-motor-3 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| Rds | double | ohm | I | Stator d-axis resistance  |
| Rqs | double | ohm | I | Stator q-asis resistance  |
| Xd_prime | double | ohm | I | Stator d-axis reactance |
| Xq_prime | double | ohm | I | Stator q-axis reactance |
| Xc_run | double | ohm | I | Running capacitor reactance |
| Xc_start | double | ohm | I | Starting capacitor reactance |
| A_sat | double | N/A | I | flux saturation parameter, A |
| b_sat | double | N/A | I | flux saturation parameter, b |

#### Three-Phase Model Properties

These properties are specific to the three-phase motor model.

Table: 37-motor table 4 { #tbl:37-motor-4 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| Rs | double | ohm | I | Stator resistance |
| Xs | double | ohm | I | Stator leakage reactance |


#### Protection, relay and contactor Properties
These properties are related to motor protection features.

Table: 37-motor table 5 { #tbl:37-motor-5 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| contactor_state | enumeration | N/A | IO | The current status of the motor. Valid values: `OPEN`, `CLOSED`. |
| contactor_open_Vmin | double | pu | I | Per unit voltage at which motor contactor opens |
| contactor_close_Vmax | double | pu | I | Per unit voltage at which motor contactor recloses |
| trip_time | double | s | I | Time motor can stay stalled before tripping off |
| uv_relay_install | enumeration | N/A | I | Is under-voltage relay protection installed on this motor. Valid values: `INSTALLED`, `UNINSTALLED`. |
| uv_relay_trip_time | double | s | I | Time low-voltage condition must exist for under-voltage protection to trip |
| uv_relay_trip_V | double | pu | I | Per unit minimum voltage before under-voltage relay trips |
| motor_trip | bool | N/A | IO | Boolean variable to check if motor is tripped |
| reconnect_time | double | s | I | Time before tripped motor reconnects |


??? note "Internal Properties"

	#### Internal Properties
	These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

Table: 37-motor table 6 { #tbl:37-motor-6 }

	| Property Name | Type | Unit | I/O | Description |
	| --- | --- | --- | --- | --- |
	| mechanical_torque_state_var | double | pu | O | Internal state variable torque - three-phase model |
	| wb | double | rad/s | — | base speed |
	| ws | double | rad/s | O | system speed |
	| psi_b | complex | N/A | O | backward rotating flux |
	| psi_f | complex | N/A | O | forward rotating flux |
	| psi_dr | complex | N/A | O | Rotor d axis flux |
	| psi_qr | complex | N/A | O | Rotor q axis flux |
	| Ids | complex | N/A | O | time before tripped motor reconnects |
	| Iqs | complex | N/A | O | time before tripped motor reconnects |
	| If | complex | N/A | O | forward current |
	| Ib | complex | N/A | O | backward current |
	| Is | complex | N/A | O | motor current |
	| electrical_power | complex | VA | O | motor power |
	| electrical_torque | double | pu | O | electrical torque |
	| Vs | complex | N/A | O | motor voltage |
	| trip | double | N/A | O | current time in tripped state |
	| reconnect | double | N/A | O | current time since motor was tripped |
	| phips | complex | N/A | O | positive sequence stator flux |
	| phins_cj | complex | N/A | O | conjugate of negative sequence stator flux |
	| phipr | complex | N/A | O | positive sequence rotor flux |
	| phinr_cj | complex | N/A | O | conjugate of negative sequence rotor flux |
	| per_unit_rotor_speed | double | pu | O | rotor speed in per-unit |
	| Ias | complex | pu | O | motor phase-a stator current |
	| Ibs | complex | pu | O | motor phase-b stator current |
	| Ics | complex | pu | O | motor phase-c stator current |
	| Vas | complex | pu | O | motor phase-a stator-to-ground voltage |
	| Vbs | complex | pu | O | motor phase-b stator-to-ground voltage |
	| Vcs | complex | pu | O | motor phase-c stator-to-ground voltage |
	| Ips | complex | N/A | O | positive sequence stator current |
	| Ipr | complex | N/A | O | positive sequence rotor current |
	| Ins_cj | complex | N/A | O | conjugate of negative sequence stator current |
	| Inr_cj | complex | N/A | O | conjugate of negative sequence rotor current |
	| Ls | double | N/A | — | stator synchronous reactance |
	| Lr | double | N/A | — | rotor synchronous reactance |
	| sigma1 | double | N/A | — | intermediate variable 1 associated with synch. react. |
	| sigma2 | double | N/A | — | intermediate variable 2 associated with synch. react. |

### Motor State of Development

In Development. 


